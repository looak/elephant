#include <gtest/gtest.h>

#include <core/chessboard.hpp>
#include <io/fen_parser.hpp>
#include <io/san_parser.hpp>
#include <move/move_undo.hpp>
#include <position/hash_zobrist.hpp>

#include "chess_positions.hpp"


namespace ElephantTest {
////////////////////////////////////////////////////////////////
class ChessboardFixture : public ::testing::Test {
public:
    virtual void SetUp() {};
    virtual void TearDown() {}; 

};

////////////////////////////////////////////////////////////////

TEST_F(ChessboardFixture, Constructor_Copy)
{
    Chessboard defaultBoard;
    chess_positions::defaultStartingPosition(defaultBoard.editPosition());

    Chessboard copy(defaultBoard);

    u64 copyBoardHash = zobrist::computeBoardHash(copy);
    u64 defaultBoardHash = zobrist::computeBoardHash(defaultBoard);

    EXPECT_EQ(copyBoardHash, defaultBoardHash);
    EXPECT_EQ(copyBoardHash, copy.readPosition().hash());
    EXPECT_EQ(defaultBoardHash, defaultBoard.readPosition().hash());

    Chessboard gameOfCentury;
    chess_positions::windmillPosition(gameOfCentury.editPosition());
    Chessboard scndCopy(gameOfCentury);
    u64 scndCopyHash = zobrist::computeBoardHash(scndCopy);
    u64 goatGame = zobrist::computeBoardHash(gameOfCentury);

    EXPECT_EQ(goatGame, gameOfCentury.readPosition().hash());
    EXPECT_EQ(scndCopyHash, scndCopy.readPosition().hash());
    EXPECT_EQ(goatGame, scndCopyHash);

    Bitboard orgMat = gameOfCentury.readPosition().material().combine<Set::BLACK>();
    Bitboard cpyMat = scndCopy.readPosition().material().combine<Set::BLACK>();
    EXPECT_EQ(orgMat.count(), cpyMat.count());
    EXPECT_EQ(orgMat, cpyMat);

    orgMat = gameOfCentury.readPosition().material().combine<Set::WHITE>();
    cpyMat = scndCopy.readPosition().material().combine<Set::WHITE>();
    EXPECT_EQ(orgMat.count(), cpyMat.count());
    EXPECT_EQ(orgMat, cpyMat);
}

TEST_F(ChessboardFixture, MakeMove_KeepsTheSideToMoveAndCountersInStep)
{
    Chessboard board = *io::fen_parser::read("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 7 20");
    MoveUndoUnit undo;

    board.makeMove(io::san_parser::deserialize("a1a2"), undo);
    EXPECT_EQ(Set::BLACK, board.readToPlay());
    EXPECT_EQ(8, board.readHalfmoveClock());
    EXPECT_EQ(20, board.readMoveCount());
    EXPECT_EQ(zobrist::computeBoardHash(board), board.readPosition().hash());

    board.makeMove(io::san_parser::deserialize("a8a7"), undo);
    EXPECT_EQ(Set::WHITE, board.readToPlay());
    EXPECT_EQ(9, board.readHalfmoveClock());
    EXPECT_EQ(21, board.readMoveCount());
    EXPECT_EQ(zobrist::computeBoardHash(board), board.readPosition().hash());
}

TEST_F(ChessboardFixture, UnmakeMove_RestoresTheBoard)
{
    // black's move completes a full move and a pawn move resets the fifty move count, both have to come back.
    const Chessboard start = *io::fen_parser::read("4k3/4p3/8/8/8/8/4P3/4K3 b - - 7 20");
    Chessboard board(start);
    MoveUndoUnit undo;

    board.makeMove(io::san_parser::deserialize("e7e6"), undo);
    EXPECT_EQ(0, board.readHalfmoveClock());
    EXPECT_EQ(21, board.readMoveCount());

    EXPECT_TRUE(board.unmakeMove(undo));
    EXPECT_TRUE(board.compare(start));
    EXPECT_EQ(start.readPosition().hash(), board.readPosition().hash());
}

TEST_F(ChessboardFixture, MakeNullMove_PassesTheTurn)
{
    // black's d4 pawn can capture on e3, so the en passant square is kept when the FEN is read.
    Chessboard board = *io::fen_parser::read("rnbqkbnr/ppp1pppp/8/8/3pP3/8/PPPP1PPP/RNBQKBNR b KQkq e3 3 3");
    ASSERT_TRUE(board.readPosition().enPassant());

    board.makeNullMove();
    EXPECT_FALSE(board.readPosition().enPassant());
    EXPECT_EQ(Set::WHITE, board.readToPlay());
    EXPECT_EQ(4, board.readHalfmoveClock());
    EXPECT_EQ(4, board.readMoveCount());
    EXPECT_EQ(zobrist::computeBoardHash(board), board.readPosition().hash());
}

////////////////////////////////////////////////////////////////

}  // namespace ElephantTest
