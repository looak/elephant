#include <gtest/gtest.h>

#include <core/chessboard.hpp>
#include <core/game_context.hpp>
#include <io/fen_parser.hpp>
#include <io/pgn_parser.hpp>
#include <diagnostics/exceptions.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace ElephantTest {
////////////////////////////////////////////////////////////////

/**
 * @file pgn_parser_test.cpp
 * @brief Testing PGN movetext as OpenBench, fastchess and the PGN standard write it. The expected positions are
 * python-chess's (chess.pgn.read_game), which nyayo reads games with today.
 * Naming convention as of October 2023: <TestedFunctionality>_<ExpectedResult>
 * @author Alexander Loodin Ek  */
class PgnParserFixture : public ::testing::Test {
public:
    // the fen without its halfmove clock, as move_executor_test.cpp compares them, the full move number is kept.
    std::string fen() const
    {
        std::string output;
        io::fen_parser::serialize(m_game.readChessboard(), output);
        const size_t moveNumber = output.find_last_of(' ');
        const size_t halfmoveClock = output.find_last_of(' ', moveNumber - 1);
        return output.substr(0, halfmoveClock) + output.substr(moveNumber);
    }

    size_t plies() const { return m_game.readGameHistory().moveUndoUnits.size(); }

    GameContext m_game;
};

// 1.e4 e5 2.Nf3 Nc6, the position most of these tests end in.
static const std::string c_kingsKnight = "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 3";

////////////////////////////////////////////////////////////////

// the PGN standard's export format puts a space between a move number and its move.
TEST_F(PgnParserFixture, SpaceAfterMoveNumber)
{
    m_game.NewGame();

    io::pgn_parser::deserialize(m_game, "1. e4 e5 2. Nf3 Nc6");

    EXPECT_EQ(4u, plies());
    EXPECT_EQ(c_kingsKnight, fen());
}

TEST_F(PgnParserFixture, NewlineBetweenMoves)
{
    m_game.NewGame();

    io::pgn_parser::deserialize(m_game, "1.e4 e5\n2.Nf3 Nc6");

    EXPECT_EQ(4u, plies());
    EXPECT_EQ(c_kingsKnight, fen());
}

TEST_F(PgnParserFixture, CommentWithSpacesAroundIt)
{
    m_game.NewGame();

    io::pgn_parser::deserialize(m_game, "1.e4 {a comment} e5 2.Nf3 Nc6");

    EXPECT_EQ(4u, plies());
    EXPECT_EQ(c_kingsKnight, fen());
}

// after a comment, black's move is numbered again with three dots.
TEST_F(PgnParserFixture, BlackMoveNumberAfterComment)
{
    m_game.NewGame();

    io::pgn_parser::deserialize(m_game, "1.e4 e5 2.Nf3 {a comment} 2...Nc6 3.Bb5 a6");

    EXPECT_EQ(6u, plies());
    EXPECT_EQ("r1bqkbnr/1ppp1ppp/p1n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4", fen());
}

// every game's movetext ends in its result.
TEST_F(PgnParserFixture, ResultToken_EndsTheGame)
{
    for (const char* result : { "1-0", "0-1", "1/2-1/2", "*" }) {
        m_game.NewGame();

        io::pgn_parser::deserialize(m_game, std::string("1.e4 e5 2.Nf3 Nc6 ") + result);

        EXPECT_EQ(4u, plies()) << result;
        EXPECT_EQ(c_kingsKnight, fen()) << result;
    }
}

// a game white ends, its last move number has no black move.
TEST_F(PgnParserFixture, OddNumberOfPlies)
{
    m_game.NewGame();

    io::pgn_parser::deserialize(m_game, "1.e4 e5 2.Nf3");

    EXPECT_EQ(3u, plies());
    EXPECT_EQ("rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 2", fen());
}

TEST_F(PgnParserFixture, MoveSuffixesAndNags_AreSkipped)
{
    m_game.NewGame();

    io::pgn_parser::deserialize(m_game, "1.e4! e5?! 2.Nf3 $1 Nc6");

    EXPECT_EQ(4u, plies());
    EXPECT_EQ(c_kingsKnight, fen());
}

// a variation is an alternative to the move before it, the mainline goes on after it.
TEST_F(PgnParserFixture, Variation_IsSkipped)
{
    m_game.NewGame();

    io::pgn_parser::deserialize(m_game, "1.e4 e5 (1...c5 2.Nf3 d6) 2.Nf3 Nc6");

    EXPECT_EQ(4u, plies());
    EXPECT_EQ(c_kingsKnight, fen());
}

TEST_F(PgnParserFixture, RestOfLineComment_IsSkipped)
{
    m_game.NewGame();

    io::pgn_parser::deserialize(m_game, "1.e4 e5 ; to the end of the line\n2.Nf3 Nc6");

    EXPECT_EQ(4u, plies());
    EXPECT_EQ(c_kingsKnight, fen());
}

// a whole game as a file holds it: tags, a blank line, the movetext and its result.
TEST_F(PgnParserFixture, TagSectionAndMovetext)
{
    m_game.NewGame();

    io::pgn_parser::deserialize(m_game,
        "[Event \"?\"]\n"
        "[White \"Elephant\"]\n"
        "[Black \"Elephant\"]\n"
        "[Result \"1-0\"]\n"
        "\n"
        "1. e4 e5 2. Nf3 Nc6 1-0\n");

    EXPECT_EQ(4u, plies());
    EXPECT_EQ(c_kingsKnight, fen());
}

// the movetext starts after the tag section, not after the last ']' in the game.
TEST_F(PgnParserFixture, TagSection_BracketInAComment)
{
    m_game.NewGame();

    io::pgn_parser::deserialize(m_game, "[Result \"*\"]\n\n1.e4 e5 {see [1]} 2.Nf3 Nc6 *");

    EXPECT_EQ(4u, plies());
    EXPECT_EQ(c_kingsKnight, fen());
}

// a PGN from a position with black to move starts with black's move.
TEST_F(PgnParserFixture, BlackMovesFirst)
{
    io::fen_parser::deserialize("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1", m_game.editChessboard());

    io::pgn_parser::deserialize(m_game, "1... e5 2. Nf3 Nc6");

    EXPECT_EQ(3u, plies());
    EXPECT_EQ(c_kingsKnight, fen());
}

// OpenBench (UnitTests/example3.pgn in openbench_eg): from the book's FEN, no move numbers, a comment after every move.
TEST_F(PgnParserFixture, OpenBenchGame)
{
    io::fen_parser::deserialize("r3kbnr/ppqb2pp/2n1pp2/3pP3/3p4/2P2N2/PP2BPPP/RNBQR1K1 w kq - 0 1",
        m_game.editChessboard());

    io::pgn_parser::deserialize(m_game,
        "cxd4 {+1.31 14/40 159 377044} fxe5 {-1.21 14/34 136 319833} dxe5 {+1.36 15/34 160 378606} "
        "Bc5 {-1.39 14/33 128 301358} 1-0");

    EXPECT_EQ(4u, plies());
    EXPECT_EQ("r3k1nr/ppqb2pp/2n1p3/2bpP3/8/5N2/PP2BPPP/RNBQR1K1 w kq - 3", fen());
}

// fastchess, as uwanja's gauntlets write it: from the book's FEN at move 2, comments, lines wrapped.
TEST_F(PgnParserFixture, FastchessGame)
{
    io::fen_parser::deserialize("rnbqkb1r/pppppppp/5n2/8/3P4/8/PPP1PPPP/RNBQKBNR w KQkq - 1 2", m_game.editChessboard());

    io::pgn_parser::deserialize(m_game,
        "2. e3 {+0.04/10 0.049s} d5 {-0.04/9 0.033s} 3. Nf3 {+0.05/8 0.015s}\n"
        "e6 {-0.10/9 0.037s} 4. Nc3 {0.00/8 0.021s} Bd6 {-0.15/9 0.026s}\n"
        "5. Bd3 {+0.18/9 0.018s} O-O {-0.14/9 0.021s} 6. O-O {+0.20/9 0.020s} 1/2-1/2");

    EXPECT_EQ(9u, plies());
    EXPECT_EQ("rnbq1rk1/ppp2ppp/3bpn2/3p4/3P4/2NBPN2/PPP2PPP/R1BQ1RK1 b - - 6", fen());
}

// the parser looped forever on a space where it expected a move.
TEST_F(PgnParserFixture, LeadingAndRepeatedSpaces)
{
    m_game.NewGame();

    io::pgn_parser::deserialize(m_game, "   1.e4   e5    2.Nf3\t\tNc6   ");

    EXPECT_EQ(4u, plies());
    EXPECT_EQ(c_kingsKnight, fen());
}

// python-chess plays the moves after a result too.
TEST_F(PgnParserFixture, MovesAfterTheResult_ArePlayed)
{
    m_game.NewGame();

    io::pgn_parser::deserialize(m_game, "1.e4 e5 1-0 2.Nf3 Nc6");

    EXPECT_EQ(4u, plies());
    EXPECT_EQ(c_kingsKnight, fen());
}

// a FEN tag sets the start position, with SetUp or without.
TEST_F(PgnParserFixture, FenTag_SetsTheStartPosition)
{
    for (const char* setUp : { "[SetUp \"1\"]\n", "" }) {
        m_game.NewGame();

        io::pgn_parser::deserialize(m_game, std::string(setUp)
            + "[FEN \"r3kbnr/ppqb2pp/2n1pp2/3pP3/3p4/2P2N2/PP2BPPP/RNBQR1K1 w kq - 0 1\"]\n\n"
              "cxd4 fxe5 dxe5 Bc5 1-0");

        EXPECT_EQ(4u, plies()) << setUp;
        EXPECT_EQ("r3k1nr/ppqb2pp/2n1p3/2bpP3/8/5N2/PP2BPPP/RNBQR1K1 w kq - 3", fen()) << setUp;
    }
}

TEST_F(PgnParserFixture, StandardVariant_IsPlayed)
{
    m_game.NewGame();

    io::pgn_parser::deserialize(m_game, "[Variant \"Standard\"]\n\n1.e4 e5 2.Nf3 Nc6 *");

    EXPECT_EQ(c_kingsKnight, fen());
    EXPECT_TRUE(io::pgn_parser::standardVariant("from position"));
    EXPECT_FALSE(io::pgn_parser::standardVariant("Chess960"));
    EXPECT_FALSE(io::pgn_parser::standardVariant(""));
}

TEST_F(PgnParserFixture, OtherVariantsAndNullMoves_ShouldThrow)
{
#ifndef EXCEPTIONS_ENABLED
    GTEST_SKIP() << "EXCEPTIONS_ENABLED is off, the parser doesn't validate input.";
#else
    m_game.NewGame();
    EXPECT_THROW({ io::pgn_parser::deserialize(m_game, "[Variant \"Chess960\"]\n\n1.e4 *"); }, ephant::io_error);
    m_game.NewGame();
    EXPECT_THROW({ io::pgn_parser::deserialize(m_game, "1.e4 -- 2.d4 *"); }, ephant::io_error);
    m_game.NewGame();
    EXPECT_THROW({ io::pgn_parser::deserialize(m_game, "[FEN \"not a fen\"]\n\n1.e4 *"); }, ephant::io_error);
#endif
}

TEST_F(PgnParserFixture, IllegalMove_ShouldThrow)
{
#ifndef EXCEPTIONS_ENABLED
    GTEST_SKIP() << "EXCEPTIONS_ENABLED is off, the parser doesn't validate input.";
#else
    m_game.NewGame();

    EXPECT_THROW({ io::pgn_parser::deserialize(m_game, "1.e4 e5 2.Ke3 Nc6"); }, ephant::io_error);
#endif
}

////////////////////////////////////////////////////////////////
// the tokenizer & the game reader

TEST(PgnTokenizer, MovesCommentsAndResult)
{
    io::pgn_parser::MovetextTokenizer tokenizer("1. e4 {best by test} 1... e5!? (1... c5 {sharp}) 2.Nf3 $1 ; line\n1-0 Nc6");
    using enum io::pgn_parser::Token::Kind;

    std::vector<std::pair<io::pgn_parser::Token::Kind, std::string>> tokens;
    while (auto token = tokenizer.next())
        tokens.emplace_back(token->kind, std::string(token->text));

    const std::vector<std::pair<io::pgn_parser::Token::Kind, std::string>> expected = {
        { Move, "e4" }, { Comment, "best by test" }, { Move, "e5" }, { Move, "Nf3" }, { Comment, " line" },
        { Result, "1-0" }, { Move, "Nc6" } };
    EXPECT_EQ(expected, tokens);
}

// python-chess's four ways to write a side passing, with and without a move number.
TEST(PgnTokenizer, NullMoves)
{
    io::pgn_parser::MovetextTokenizer tokenizer("1. e4 -- 2.-- Z0 3. 0000 @@@@ *");
    using enum io::pgn_parser::Token::Kind;

    std::vector<std::pair<io::pgn_parser::Token::Kind, std::string>> tokens;
    while (auto token = tokenizer.next())
        tokens.emplace_back(token->kind, std::string(token->text));

    const std::vector<std::pair<io::pgn_parser::Token::Kind, std::string>> expected = { { Move, "e4" },
        { NullMove, "--" }, { NullMove, "--" }, { NullMove, "Z0" }, { NullMove, "0000" }, { NullMove, "@@@@" },
        { Result, "*" } };
    EXPECT_EQ(expected, tokens);
}

// a blank line ends the movetext, in a variation too, but not in a comment.
TEST(PgnTokenizer, BlankLine_EndsTheMovetext)
{
    for (const char* movetext : { "1. e4 e5\n\n2. Nf3 *", "1. e4 e5\r\n  \r\n2. Nf3 *", "1. e4 e5 (1... c5\n\n2. Nf3) *" }) {
        io::pgn_parser::MovetextTokenizer tokenizer(movetext);
        EXPECT_EQ("e4", tokenizer.next()->text) << movetext;
        EXPECT_EQ("e5", tokenizer.next()->text) << movetext;
        EXPECT_FALSE(tokenizer.next()) << movetext;
    }

    io::pgn_parser::MovetextTokenizer tokenizer("1. e4 {a comment\n\nover a blank line} e5 *");
    EXPECT_EQ("e4", tokenizer.next()->text);
    EXPECT_EQ("a comment\n\nover a blank line", tokenizer.next()->text);
    EXPECT_EQ("e5", tokenizer.next()->text);
    EXPECT_EQ("*", tokenizer.next()->text);
    EXPECT_FALSE(tokenizer.next());
}

TEST(PgnTokenizer, CastlingWithZerosIsNotAMoveNumber)
{
    io::pgn_parser::MovetextTokenizer tokenizer("5.0-0 0-0-0 *");

    EXPECT_EQ("0-0", tokenizer.next()->text);
    EXPECT_EQ("0-0-0", tokenizer.next()->text);
    EXPECT_EQ("*", tokenizer.next()->text);
    EXPECT_FALSE(tokenizer.next());
}

TEST(PgnGameReader, SplitsAStreamIntoGames)
{
    const std::string stream =
        "\xEF\xBB\xBF[Event \"one\"]\n"
        "[White \"Elephant \\\"herd\\\" 1.0\"]\n"
        "\n"
        "1. e4 e5 1-0\n"
        "\n"
        "[Event \"two\"]\n"
        "[FEN \"8/8/8/8/8/8/8/K6k w - - 0 1\"]\n"
        "\n"
        "{no moves} 1/2-1/2\n"
        "\n"
        "[Event \"three\"]\n"
        "\n"
        "1. d4 {a comment with a [bracket]} d5 *\n";
    io::pgn_parser::GameReader reader(stream);

    auto one = reader.next();
    ASSERT_TRUE(one);
    EXPECT_EQ(3u, one->offset);
    ASSERT_EQ(2u, one->tags.size());
    EXPECT_EQ("one", *one->tag("Event"));
    EXPECT_EQ("Elephant \\\"herd\\\" 1.0", *one->tag("White"));  // as written, as python-chess reads it
    EXPECT_EQ(nullptr, one->tag("Black"));
    EXPECT_EQ("1. e4 e5 1-0", one->movetext);
    EXPECT_EQ(stream.substr(3, one->length), stream.substr(one->offset, one->length));

    auto two = reader.next();
    ASSERT_TRUE(two);
    EXPECT_EQ(stream.find("[Event \"two\"]"), two->offset);
    EXPECT_EQ("8/8/8/8/8/8/8/K6k w - - 0 1", *two->tag("FEN"));
    EXPECT_EQ("{no moves} 1/2-1/2", two->movetext);

    auto three = reader.next();
    ASSERT_TRUE(three);
    EXPECT_EQ("1. d4 {a comment with a [bracket]} d5 *", three->movetext);

    EXPECT_FALSE(reader.next());
}

// python-chess's reading: a value runs to the last "] of its line, quotes in it unescaped or not, and a line that
// isn't a tag is skipped.
TEST(PgnGameReader, TagLines_AsPythonChessReadsThem)
{
    const std::string stream =
        "[Event \"one\"]\n[White unquoted]\n[Black \"Elephant \"herd\" dev\"]\n[_Bad \"x\"]\n[Site\"x\"]\n"
        "[Round \"1\"]   \n\n1. e4 *\n\n"
        "[Event \"two\"]\n\n1. d4 *\n";
    io::pgn_parser::GameReader reader(stream);

    auto one = reader.next();
    ASSERT_TRUE(one);
    ASSERT_EQ(3u, one->tags.size());
    EXPECT_EQ("one", *one->tag("Event"));
    EXPECT_EQ("Elephant \"herd\" dev", *one->tag("Black"));
    EXPECT_EQ("1", *one->tag("Round"));
    EXPECT_EQ("1. e4 *", one->movetext);

    auto two = reader.next();
    ASSERT_TRUE(two);
    EXPECT_EQ("1. d4 *", two->movetext);
}

// where python-chess splits games: ; and % lines skipped among the tags, one blank line skipped, a second one ends the
// game without moves, and a blank line in the movetext ends the game there, what follows is the next one.
TEST(PgnGameReader, SplitsWherePythonChessDoes)
{
    const std::string stream =
        "; a comment line\n[Event \"one\"]\n% an escape line\n\n[Site \"x\"]\n\n1. e4 *\n\n"
        "[Event \"two\"]\n\n\n1. d4 d5\n\n2. c4 *\n";
    io::pgn_parser::GameReader reader(stream);

    auto one = reader.next();
    ASSERT_TRUE(one);
    ASSERT_EQ(2u, one->tags.size());
    EXPECT_EQ("x", *one->tag("Site"));
    EXPECT_EQ("1. e4 *", one->movetext);

    auto two = reader.next();
    ASSERT_TRUE(two);
    EXPECT_EQ("two", *two->tag("Event"));
    EXPECT_EQ("", two->movetext);

    auto three = reader.next();
    ASSERT_TRUE(three);
    EXPECT_TRUE(three->tags.empty());
    EXPECT_EQ("1. d4 d5", three->movetext);

    auto four = reader.next();
    ASSERT_TRUE(four);
    EXPECT_EQ("2. c4 *", four->movetext);

    EXPECT_FALSE(reader.next());
}

////////////////////////////////////////////////////////////////
// the fixtures in test/pgn: OpenBench's workload as uploaded (UnitTests/example3.pgn in openbench_eg), cutechess-ob's
// games before OpenBench strips them (example1.pgn, example2.pgn), a fastchess gauntlet as uwanja plays it, and edge
// cases. Every game is played to its end, to its PlyCount where it has one.

TEST(PgnFixtures, EveryGameIsPlayed)
{
#ifndef EXCEPTIONS_ENABLED
    GTEST_SKIP() << "EXCEPTIONS_ENABLED is off, the parser doesn't validate input.";
#else
    struct Fixture {
        const char* file;
        size_t games;
    };
    const std::vector<Fixture> fixtures = { { "openbench_workload.pgn", 34 }, { "openbench_unstripped_1.pgn", 1 },
        { "openbench_unstripped_2.pgn", 1 }, { "fastchess_gauntlet.pgn", 8 }, { "edge_cases.pgn", 11 } };

    GameContext game;
    for (const Fixture& fixture : fixtures) {
        std::ifstream file(std::filesystem::path(ELEPHANT_TEST_PGN_DIR) / fixture.file, std::ios::binary);
        ASSERT_TRUE(file) << fixture.file;
        std::stringstream buffer;
        buffer << file.rdbuf();
        const std::string stream = buffer.str();

        io::pgn_parser::GameReader reader(stream);
        size_t games = 0;
        while (std::optional<io::pgn_parser::Game> parsed = reader.next()) {
            ++games;
            const std::string_view text = std::string_view(stream).substr(parsed->offset, parsed->length);
            const std::string* event = parsed->tag("Event");
            game.NewGame();
            if (event && event->starts_with("an illegal move")) {
                EXPECT_THROW({ io::pgn_parser::deserialize(game, text); }, ephant::io_error) << fixture.file;
                continue;
            }
            EXPECT_NO_THROW({ io::pgn_parser::deserialize(game, text); }) << fixture.file << " game " << games;
            if (const std::string* plyCount = parsed->tag("PlyCount")) {
                EXPECT_EQ(*plyCount, std::to_string(game.readGameHistory().moveUndoUnits.size()))
                    << fixture.file << " game " << games;
            }
        }
        EXPECT_EQ(fixture.games, games) << fixture.file;
    }
#endif
}

} // namespace ElephantTest
