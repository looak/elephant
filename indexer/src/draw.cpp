#include "draw.hpp"

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <format>
#include <unordered_map>
#include <unordered_set>

#include "polyglot.hpp"

#include <core/chessboard.hpp>
#include <io/fen_parser.hpp>
#include <move/move_executor.hpp>
#include <search/search.hpp>
#include <search/search_constants.hpp>
#include <search/search_threadcontext.hpp>
#include <system/time_manager.hpp>

namespace indexer {
namespace {
    // splitmix64, the same sequence on every platform; std::shuffle's isn't.
    struct Random {
        u64 state;
        u64 next()
        {
            u64 z = (state += 0x9e3779b97f4a7c15ull);
            z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
            z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
            return z ^ (z >> 31);
        }
    };

    // a mixed seed for each game, so the games draw independently and a game's sample doesn't depend on the others'.
    u64 gameSeed(u64 seed, u64 game)
    {
        Random mix{ seed ^ (game * 0xd6e8feb86659fd93ull) };
        return mix.next();
    }

    // white's result in halves of a point, 2 for a win; nullopt for a game without one.
    std::optional<u32> resultHalves(const std::string& result)
    {
        if (result == "1-0")
            return 2u;
        if (result == "0-1")
            return 0u;
        if (result == "1/2-1/2")
            return 1u;
        return std::nullopt;
    }

    // an average of results to one place or more, 1.0, 0.5, 0.75, 0.3333, with no zeros to spare.
    std::string averageLabel(u64 halves, u64 games)
    {
        std::string text = std::format("{:.4f}", double(halves) / double(2 * games));
        while (text.size() > 3 && text.back() == '0')
            text.pop_back();
        return "[" + text + "]";
    }

    // a JSON string, quotes and escapes.
    std::string quoted(const std::string& text)
    {
        std::string out = "\"";
        for (char c : text) {
            if (c == '"' || c == '\\')
                out += '\\';
            if (static_cast<unsigned char>(c) < 0x20)
                out += std::format("\\u{:04x}", static_cast<unsigned>(c));
            else
                out += c;
        }
        return out + '"';
    }
} // namespace

struct Drawer::Impl {
    explicit Impl(DrawOptions o)
        : options(o)
        , clock(SearchParameters{}, Set::WHITE)
        , context(Position{}, true, clock)
    {
        context.stopEnabled = false;  // quiescence runs to its own end
    }

    // Not in check, and the engine's own quiescence search finds nothing it would rather play than stand pat: the
    // position is its own evaluation. The search is a test, its line is thrown away.
    bool naturallyQuiet(const Chessboard& board)
    {
        context.position = board.readPosition().copy();
        context.accumulator = PestoAccumulator::computeFromScratch(context.position.read().material());
        context.gameState.whiteToMove = board.readToPlay() == Set::WHITE;

        PVLine line;
        const i16 score = context.gameState.whiteToMove ? Search::quiescenceLine<Set::WHITE>(context, line)
                                                         : Search::quiescenceLine<Set::BLACK>(context, line);
        return line.length == 0 && std::abs(score) < c_checkmateMinScore;
    }

    // a position drawn by one or more games: the sum of their results in halves, 2 a win and 1 a draw.
    struct Merged {
        std::string fen;
        u64 halves = 0;
        u64 games = 0;
    };

    DrawOptions options;
    std::vector<Merged> merged;                       // in the order the positions were first drawn
    std::unordered_map<u64, size_t> mergedIndex;      // Polyglot hash to its place in merged
    TimeManager clock;
    ThreadSearchContext context;
    std::unordered_set<u64> seen;   // the Polyglot hashes written so far
    u64 gamesSeen = 0;
};

Drawer::Drawer(DrawOptions options) : m_impl(std::make_unique<Impl>(options)) {}
Drawer::~Drawer() = default;

std::pair<std::string, std::optional<u32>> splitInputMinimum(const std::string& argument)
{
    const size_t colon = argument.rfind(':');
    if (colon == std::string::npos || colon == 0 || colon + 1 == argument.size())
        return { argument, std::nullopt };
    u32 value = 0;
    const char* first = argument.data() + colon + 1;
    const char* last = argument.data() + argument.size();
    const auto [end, error] = std::from_chars(first, last, value);
    if (error != std::errc{} || end != last)
        return { argument, std::nullopt };
    return { argument.substr(0, colon), value };
}

std::string resultLabel(const std::string& result)
{
    if (result == "1-0")
        return "[1.0]";
    if (result == "0-1")
        return "[0.0]";
    if (result == "1/2-1/2")
        return "[0.5]";
    return "";
}

void Drawer::draw(const GameIndex& entry, std::string& out, std::optional<u32> minPlyHere)
{
    Impl& impl = *m_impl;
    const u64 game = impl.gamesSeen++;
    ++m_stats.games;

    const std::string label = resultLabel(entry.result);
    const std::optional<u32> halves = resultHalves(entry.result);
    // a game that didn't read has no positions to trust; one with a null move was never played over the board.
    if (!entry.ok || entry.played.size() != entry.plies || entry.positions.size() != entry.plies + 1) {
        ++m_stats.unread;
        return;
    }
    if (label.empty()) {
        ++m_stats.noResult;
        return;
    }
    if (std::any_of(entry.played.begin(), entry.played.end(), [](PackedMove move) { return move == PackedMove::NullMove(); })) {
        ++m_stats.nullMoves;
        return;
    }

    Chessboard board;
    try {
        if (!io::fen_parser::deserialize(entry.startFen, board)) {
            ++m_stats.unread;
            return;
        }
    }
    catch (const std::exception&) {
        ++m_stats.unread;
        return;
    }

    ++m_stats.drawn;

    // the plies to skip, counted from the position the game starts at: its tag's, else its file's, else the global one.
    u32 minPly = minPlyHere ? *minPlyHere : impl.options.minPly;
    bool own = minPlyHere.has_value();
    for (const io::pgn_parser::Tag& tag : entry.tags) {
        if (tag.name != c_minPlyTag)
            continue;
        u32 value = 0;
        const auto [end, error] = std::from_chars(tag.value.data(), tag.value.data() + tag.value.size(), value);
        if (error == std::errc{} && end == tag.value.data() + tag.value.size() && !tag.value.empty()) {
            minPly = value;
            own = true;
        }
        else {
            ++m_stats.badMinimumTags;
        }
    }
    if (own)
        ++m_stats.ownMinimum;

    struct Candidate {
        u32 ply;
        std::string fen;
    };
    std::vector<Candidate> candidates;

    // the positions after ply 0, 1, ... in turn, the ones from minPly on that are worth a look.
    MoveUndoUnit undo;
    u32 candidateCount = 0;
    for (u32 ply = 0;; ++ply) {
        if (ply >= minPly) {
            const PositionFacts facts = positionFacts(board);
            if (!facts.inCheck && facts.canMove) {
                ++candidateCount;
                if (!impl.options.quiet || impl.naturallyQuiet(board)) {
                    std::string fen;
                    io::fen_parser::serialize(board, fen);
                    candidates.push_back({ ply, std::move(fen) });
                }
            }
        }
        if (ply == entry.plies)
            break;
        MoveExecutor executor(board.editPosition());
        GameState& state = board.editState();
        executor.makeMove<true>(entry.played[ply], undo, state.plyCount);
        state.whiteToMove = !state.whiteToMove;
        state.moveCount += static_cast<u16>(state.whiteToMove);
    }

    m_stats.candidates += candidateCount;
    m_stats.quiet += candidates.size();
    m_stats.quietPerGame.push_back(static_cast<u32>(candidates.size()));

    // a seeded shuffle, then the first that no game has given yet; with merge the first N, whoever gave them before.
    Random random{ gameSeed(impl.options.seed, game) };
    for (size_t i = candidates.size(); i > 1; --i)
        std::swap(candidates[i - 1], candidates[random.next() % i]);

    if (impl.options.merge) {
        const size_t taken = std::min<size_t>(candidates.size(), impl.options.perGame);
        for (size_t i = 0; i < taken; ++i) {
            const auto [place, added] = impl.mergedIndex.try_emplace(entry.positions[candidates[i].ply], impl.merged.size());
            if (added)
                impl.merged.push_back({ candidates[i].fen, 0, 0 });
            impl.merged[place->second].halves += *halves;
            ++impl.merged[place->second].games;
            ++m_stats.draws;
        }
        return;
    }

    u32 written = 0;
    for (const Candidate& candidate : candidates) {
        if (written == impl.options.perGame)
            break;
        if (!impl.seen.insert(entry.positions[candidate.ply]).second) {
            ++m_stats.duplicates;
            continue;
        }
        out += candidate.fen;
        out += ' ';
        out += label;
        out += '\n';
        ++written;
    }
    m_stats.written += written;
    (*halves == 2 ? m_stats.whiteWins : *halves == 0 ? m_stats.blackWins : m_stats.drawResults) += written;
}

void Drawer::finish(std::string& out)
{
    Impl& impl = *m_impl;
    if (!impl.options.merge)
        return;
    for (const Impl::Merged& position : impl.merged) {
        out += position.fen;
        out += ' ';
        out += averageLabel(position.halves, position.games);
        out += '\n';
        if (position.games > 1)
            ++m_stats.merged;
        if (position.halves == 2 * position.games)
            ++m_stats.whiteWins;
        else if (position.halves == 0)
            ++m_stats.blackWins;
        else if (position.halves == position.games)
            ++m_stats.drawResults;
        else
            ++m_stats.mixed;
    }
    m_stats.written = impl.merged.size();
    impl.merged.clear();
    impl.mergedIndex.clear();
}

std::string statsJson(const DrawStats& stats, const DrawOptions& options, const std::vector<DrawInput>& inputs,
    const std::string& reader)
{
    std::vector<u32> quiet = stats.quietPerGame;
    std::sort(quiet.begin(), quiet.end());
    auto at = [&](double fraction) {
        return quiet.empty() ? 0u : quiet[static_cast<size_t>(fraction * double(quiet.size() - 1))];
    };
    const double mean = quiet.empty() ? 0.0 : double(stats.quiet) / double(quiet.size());
    const u64 few = static_cast<u64>(std::count_if(quiet.begin(), quiet.end(), [&](u32 n) { return n < options.perGame; }));

    std::string json = std::format("{{\"stats\":{},\"reader\":{},", c_statsVersion, quoted(reader));
    json += std::format("\"options\":{{\"draw\":{},\"quiet\":{},\"min_ply\":{},\"merge\":{},\"seed\":{}}},", options.perGame,
        options.quiet ? "true" : "false", options.minPly, options.merge ? "true" : "false", options.seed);
    json += "\"inputs\":[";
    for (size_t i = 0; i < inputs.size(); ++i) {
        json += std::format("{}{{\"file\":{},\"min_ply\":{},\"games\":{},\"used\":{}}}", i ? "," : "", quoted(inputs[i].file),
            inputs[i].minPly ? std::to_string(*inputs[i].minPly) : std::string("null"), inputs[i].games, inputs[i].drawn);
    }
    json += "],";
    json += std::format("\"games\":{{\"read\":{},\"used\":{},\"skipped\":{},\"unread\":{},\"no_result\":{},\"null_moves\":{},"
                        "\"own_minimum\":{},\"bad_minimum_tags\":{}}},",
        stats.games, stats.drawn, stats.games - stats.drawn, stats.unread, stats.noResult, stats.nullMoves, stats.ownMinimum,
        stats.badMinimumTags);
    json += std::format("\"positions\":{{\"candidates\":{},\"quiet\":{},"
                        "\"quiet_per_game\":{{\"mean\":{:.2f},\"median\":{},\"p5\":{},\"min\":{}}},\"games_under_n\":{},"
                        "\"written\":{},\"duplicates_dropped\":{},\"drawn\":{},\"merged\":{},"
                        "\"results\":{{\"white\":{},\"draw\":{},\"black\":{},\"mixed\":{}}}}}}}",
        stats.candidates, stats.quiet, mean, at(0.5), at(0.05), at(0.0), few, stats.written,
        options.merge ? std::string("null") : std::to_string(stats.duplicates),
        options.merge ? std::to_string(stats.draws) : std::string("null"),
        options.merge ? std::to_string(stats.merged) : std::string("null"), stats.whiteWins, stats.drawResults, stats.blackWins,
        stats.mixed);
    return json;
}

} // namespace indexer
