#include <io/pgn_parser.hpp>
#include <io/san_parser.hpp>
#include <core/game_context.hpp>
#include <diagnostics/exceptions.hpp>

#include <io/fen_parser.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <format>

namespace io {
namespace pgn_parser {
namespace {
    bool isSpace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f'; }
    bool isDigit(char c) { return c >= '0' && c <= '9'; }

    // a word of movetext ends at whitespace or at anything that starts another token.
    bool endsWord(char c)
    {
        return isSpace(c) || c == '{' || c == '}' || c == '(' || c == ')' || c == ';' || c == '[' || c == '$';
    }

    bool isResult(std::string_view word) { return word == "1-0" || word == "0-1" || word == "1/2-1/2" || word == "*"; }

    // python-chess's null moves, a side passing.
    bool isNullMove(std::string_view word) { return word == "--" || word == "Z0" || word == "0000" || word == "@@@@"; }

    // the end of the line, or of the text.
    size_t endOfLine(std::string_view text, size_t from)
    {
        const size_t eol = text.find('\n', from);
        return eol == std::string_view::npos ? text.size() : eol;
    }

    // the start of the next line, or the end of the text.
    size_t nextLine(std::string_view text, size_t from) { return std::min(endOfLine(text, from) + 1, text.size()); }

    // whether the line from here holds nothing but whitespace; the end of the text is a blank line too.
    bool blankLine(std::string_view text, size_t from)
    {
        for (size_t i = from; i < text.size() && text[i] != '\n'; ++i) {
            if (!isSpace(text[i]))
                return false;
        }
        return true;
    }

    bool lineStart(std::string_view text, size_t pos) { return pos == 0 || text[pos - 1] == '\n'; }
} // namespace

    std::optional<Token> MovetextTokenizer::next() {
        if (m_pos == 0 && blankLine(m_text, 0))
            m_done = true;  // the game has no movetext
        while (!m_done) {
            while (m_pos < m_text.size() && isSpace(m_text[m_pos])) {
                if (m_text[m_pos] == '\n' && blankLine(m_text, m_pos + 1)) {
                    m_done = true;  // a blank line ends the game
                    return std::nullopt;
                }
                ++m_pos;
            }
            if (m_pos >= m_text.size())
                break;

            const char c = m_text[m_pos];
            if (c == '{') {
                size_t close = m_text.find('}', m_pos + 1);
                if (close == std::string_view::npos)
                    close = m_text.size();  // an unclosed comment runs to the end
                Token comment{ Token::Kind::Comment, m_text.substr(m_pos + 1, close - m_pos - 1) };
                m_pos = std::min(close + 1, m_text.size());
                return comment;
            }
            if (c == ';') {
                const size_t eol = endOfLine(m_text, m_pos);
                Token comment{ Token::Kind::Comment, m_text.substr(m_pos + 1, eol - m_pos - 1) };
                m_pos = eol;
                return comment;
            }
            if (c == '%' && (m_pos == 0 || m_text[m_pos - 1] == '\n')) {
                m_pos = endOfLine(m_text, m_pos);  // an escape line isn't the game's
                continue;
            }
            if (c == '(') {
                skipVariation();
                continue;
            }
            if (c == ')' || c == '}') {
                ++m_pos;
                continue;
            }
            if (c == '$') {
                ++m_pos;
                while (m_pos < m_text.size() && isDigit(m_text[m_pos]))
                    ++m_pos;
                continue;
            }
            if (c == '[')
                break;  // the next game's tags

            const size_t start = m_pos;
            while (m_pos < m_text.size() && !endsWord(m_text[m_pos]))
                ++m_pos;
            std::string_view word = m_text.substr(start, m_pos - start);

            if (isResult(word))
                return Token{ Token::Kind::Result, word };
            if (isNullMove(word))
                return Token{ Token::Kind::NullMove, word };
            // a move number, 12. or 12..., possibly with its move right behind it; 0-0 is castling, not a number.
            if (isDigit(word.front()) && !word.starts_with("0-0")) {
                size_t i = 0;
                while (i < word.size() && isDigit(word[i]))
                    ++i;
                word.remove_prefix(i);
            }
            while (!word.empty() && word.front() == '.')
                word.remove_prefix(1);
            while (!word.empty() && (word.back() == '!' || word.back() == '?'))
                word.remove_suffix(1);
            if (isNullMove(word))
                return Token{ Token::Kind::NullMove, word };
            if (!word.empty())
                return Token{ Token::Kind::Move, word };
        }
        m_done = true;
        return std::nullopt;
    }

    // past the variation's closing parenthesis, the variations nested in it and their comments included, or up to a
    // blank line, which ends the game in a variation too.
    void MovetextTokenizer::skipVariation() {
        u32 depth = 0;
        while (m_pos < m_text.size()) {
            const char c = m_text[m_pos];
            if (c == '\n' && blankLine(m_text, m_pos + 1))
                return;
            if (c == '{') {
                const size_t close = m_text.find('}', m_pos + 1);
                m_pos = close == std::string_view::npos ? m_text.size() : close + 1;
                continue;
            }
            if (c == ';') {
                m_pos = endOfLine(m_text, m_pos);
                continue;
            }
            ++m_pos;
            if (c == '(')
                ++depth;
            else if (c == ')' && --depth == 0)
                return;
        }
    }

    const std::string* Game::tag(std::string_view name) const {
        for (const Tag& tag : tags) {
            if (tag.name == name)
                return &tag.value;
        }
        return nullptr;
    }

    // the blank lines, escape lines (%) and comment lines (;) before a game.
    void GameReader::skipWhitespace() {
        while (m_pos < m_stream.size()) {
            const char c = m_stream[m_pos];
            if (isSpace(c))
                ++m_pos;
            else if ((c == '%' || c == ';') && lineStart(m_stream, m_pos))
                m_pos = endOfLine(m_stream, m_pos);
            else if (m_stream.substr(m_pos).starts_with("\xEF\xBB\xBF"))
                m_pos += 3;  // a UTF-8 byte order mark
            else
                return;
        }
    }

    // A tag is a line, [Name "value"], read as python-chess reads it: the value runs from the first quote to the last
    // "] on the line and is kept as written, a \" in it stays a \". A line that isn't a tag is skipped.
    bool GameReader::readTag(Game& game) {
        const size_t eol = endOfLine(m_stream, m_pos);
        std::string_view line = m_stream.substr(m_pos, eol - m_pos);
        m_pos = eol;
        while (!line.empty() && isSpace(line.back()))
            line.remove_suffix(1);

        auto isNameChar = [](char c) {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || isDigit(c) || c == '_' || c == '+' || c == '#'
                || c == '=' || c == ':' || c == '-';
        };
        size_t i = 1;
        while (i < line.size() && isNameChar(line[i]))
            ++i;
        const std::string_view name = line.substr(1, i - 1);
        const size_t quote = i;
        while (i < line.size() && isSpace(line[i]))
            ++i;

        const bool read = !name.empty() && std::string_view("_+#=:-").find(name.front()) == std::string_view::npos && i > quote && i < line.size() && line[i] == '"'
            && line.size() >= i + 3 && line.ends_with("\"]");
        if (!read)
            return false;
        game.tags.push_back(Tag{ std::string(name), std::string(line.substr(i + 1, line.size() - 2 - (i + 1))) });
        return true;
    }

    std::optional<Game> GameReader::next() {
        skipWhitespace();
        if (m_pos >= m_stream.size())
            return std::nullopt;

        Game game;
        game.offset = m_pos;
        // the tags, a line at a time: one blank line between them is skipped, a second ends the game without moves.
        bool blank = false;
        while (m_pos < m_stream.size()) {
            const char c = m_stream[m_pos];
            if ((c == '%' || c == ';') && lineStart(m_stream, m_pos)) {
                m_pos = nextLine(m_stream, m_pos);
                continue;
            }
            if (blankLine(m_stream, m_pos)) {
                if (blank)
                    break;
                blank = true;
                m_pos = nextLine(m_stream, m_pos);
                continue;
            }
            if (c != '[')
                break;
            blank = false;
            readTag(game);
            m_pos = nextLine(m_stream, m_pos);
        }

        MovetextTokenizer tokenizer(m_stream.substr(m_pos));
        while (tokenizer.next()) {}
        game.movetext = m_stream.substr(m_pos, tokenizer.position());
        m_pos += tokenizer.position();
        game.length = m_pos - game.offset;
        return game;
    }

    bool standardVariant(std::string_view variant) {
        static constexpr std::array<std::string_view, 6> c_names = { "standard", "chess", "classical", "normal",
            "illegal", "from position" };
        return std::any_of(c_names.begin(), c_names.end(), [variant](std::string_view name) {
            return std::equal(name.begin(), name.end(), variant.begin(), variant.end(), [](char a, char b) {
                return a == static_cast<char>(std::tolower(static_cast<unsigned char>(b)));
            });
        });
    }

    void deserialize(GameContext& game, std::string_view pgn) {
        GameReader reader(pgn);
        std::optional<Game> parsed = reader.next();
        if (!parsed)
            return;

        [[maybe_unused]] const std::string* variant = parsed->tag("Variant");
        THROW_EXPR(!variant || standardVariant(*variant), ephant::io_error,
            std::format("pgn_parser :: Not standard chess, Variant \"{}\"", *variant));
        if (const std::string* fen = parsed->tag("FEN")) {
            game.Reset();
            [[maybe_unused]] const bool read = io::fen_parser::deserialize(*fen, game.editChessboard());
            THROW_EXPR(read, ephant::io_error, std::format("pgn_parser :: The FEN tag doesn't read: {}", *fen));
        }

        MovetextTokenizer tokenizer(parsed->movetext);
        while (std::optional<Token> token = tokenizer.next()) {
            if (token->kind == Token::Kind::NullMove) {
                THROW_EXPR(false, ephant::io_error,
                    std::format("pgn_parser :: A null move ({}) can't be played on a GameContext", token->text));
                continue;
            }
            if (token->kind != Token::Kind::Move)
                continue;
            const bool whiteToMove = game.readToPlay() == Set::WHITE;
            PackedMove move = san_parser::deserialize(game.readChessPosition(), whiteToMove, token->text);
            game.MakeMove<true>(move);
        }
    }

} // namespace pgn_parser
} // namespace io
