#include <gtest/gtest.h>

#include <polyglot.hpp>
#include <core/chessboard.hpp>
#include <elephant_gambit.hpp>
#include <io/fen_parser.hpp>
#include <io/san_parser.hpp>
#include <move/move_executor.hpp>

#include <sstream>
#include <string>
#include <vector>

namespace ElephantTest {
////////////////////////////////////////////////////////////////

/**
 * @file polyglot_test.cpp
 * @brief Testing phant-indexer's Polyglot hash against the test vectors Polyglot's book format documents, and the
 * rules python-chess's chess.polyglot.zobrist_hash follows where they matter: an en passant file only with a pawn to
 * take, castling rights only with the king & rook at home. test/indexer_smoke.py compares every position of every
 * game with python-chess.
 * Naming convention as of October 2023: <TestedFunctionality>_<ExpectedResult>
 * @author Alexander Loodin Ek  */
class PolyglotFixture : public ::testing::Test {
public:
    // the start position, then the moves, in SAN.
    u64 hashAfter(const std::string& moves)
    {
        Chessboard board;
        chess::NewGame(board);
        std::istringstream words(moves);
        std::string san;
        MoveUndoUnit undo;
        while (words >> san) {
            const bool white = board.readToPlay() == Set::WHITE;
            PackedMove move = io::san_parser::deserialize(board.readPosition(), white, san);
            MoveExecutor executor(board.editPosition());
            GameState& state = board.editState();
            executor.makeMove<true>(move, undo, state.plyCount);
            state.whiteToMove = !state.whiteToMove;
            state.moveCount += static_cast<u16>(state.whiteToMove);
        }
        return indexer::polyglotHash(board);
    }

    u64 hashOf(const std::string& fen)
    {
        Chessboard board;
        EXPECT_TRUE(io::fen_parser::deserialize(fen, board)) << fen;
        return indexer::polyglotHash(board);
    }
};

////////////////////////////////////////////////////////////////

// Polyglot's own list, every one of them.
TEST_F(PolyglotFixture, TestVectors)
{
    struct TestCase {
        std::string moves;
        u64 hash;
    };
    const std::vector<TestCase> testCases = {
        { "", 0x463b96181691fc9cull },
        { "e4", 0x823c9b50fd114196ull },
        { "e4 d5", 0x0756b94461c50fb0ull },
        { "e4 d5 e5", 0x662fafb965db29d4ull },
        { "e4 d5 e5 f5", 0x22a48b5a8e47ff78ull },
        { "e4 d5 e5 f5 Ke2", 0x652a607ca3f242c1ull },
        { "e4 d5 e5 f5 Ke2 Kf7", 0x00fdd303c946bdd9ull },
        { "a4 b5 h4 b4 c4", 0x3c8123ea7b067637ull },
        { "a4 b5 h4 b4 c4 bxc3 Ra3", 0x5c3f9b829b279560ull },
    };
    for (const TestCase& testCase : testCases)
        EXPECT_EQ(testCase.hash, hashAfter(testCase.moves)) << testCase.moves;
}

// the same vectors from FEN: the move counters aren't hashed, and e4's en passant square isn't, no black pawn can take.
TEST_F(PolyglotFixture, FromFen_CountersAndUncapturableEnPassantIgnored)
{
    EXPECT_EQ(0x463b96181691fc9cull, hashOf("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
    EXPECT_EQ(0x463b96181691fc9cull, hashOf("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 37 90"));
    EXPECT_EQ(0x823c9b50fd114196ull, hashOf("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"));
    EXPECT_EQ(0x823c9b50fd114196ull, hashOf("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1"));
    // after e4 d5 e5 f5 white's e-pawn can take on f6: the file is hashed.
    EXPECT_EQ(0x22a48b5a8e47ff78ull, hashOf("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3"));
}

// a castling right the FEN claims without the rook at home counts for nothing, as python-chess cleans it.
TEST_F(PolyglotFixture, CastlingRight_OnlyWithKingAndRookAtHome)
{
    EXPECT_EQ(hashOf("4k3/8/8/8/8/8/8/4K2R w - - 0 1"), hashOf("4k3/8/8/8/8/8/8/4K2R w Q - 0 1"));
    EXPECT_NE(hashOf("4k3/8/8/8/8/8/8/4K2R w - - 0 1"), hashOf("4k3/8/8/8/8/8/8/4K2R w K - 0 1"));
}

} // namespace ElephantTest
