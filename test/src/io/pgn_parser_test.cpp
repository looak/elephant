#include <gtest/gtest.h>

#include <core/chessboard.hpp>
#include <core/game_context.hpp>
#include <io/fen_parser.hpp>
#include <io/pgn_parser.hpp>

#include <string>

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

} // namespace ElephantTest
