#include <gtest/gtest.h>
#include <core/chessboard.hpp>
#include <core/game_context.hpp>
#include <move/move_executor.hpp>
#include <position/hash_zobrist.hpp>

#include "chess_positions.hpp"

namespace ElephantTest {

TEST(ZobristHashing, Initialization)
{
    zobrist::internals::initialize();
    bool initialized = zobrist::internals::initialized();
    EXPECT_TRUE(initialized) << "Zobrist hashing should be initialized after calling initialize().";

    // calling initialize again should not change anything.
    zobrist::internals::initialize();
    initialized = zobrist::internals::initialized();
    EXPECT_TRUE(initialized) << "Zobrist hashing should remain initialized after calling initialize() again.";
}


TEST(ZobristHashing, Hashing)
{
    zobrist::internals::initialize();

    Chessboard board;
    u64 hash = zobrist::computeBoardHash(board);
    EXPECT_EQ(hash, 0) << "Empty board should have a hash of zero.";

    chess_positions::defaultStartingPosition(board.editPosition());
    hash = zobrist::computeBoardHash(board);
    EXPECT_NE(hash, 0) << "Default starting position should not have a hash of zero.";
    EXPECT_NE(board.readPosition().hash(), 0) << "Default starting position should not have a hash of zero."; 
    EXPECT_EQ(board.readPosition().hash(), hash) << "Hash from computeBoardHash should match position's stored hash.";
}

TEST(ZobristHashing, SamePositionSameHash)
{
    zobrist::internals::initialize();

    Chessboard boardOne;
    Chessboard boardTwo;

    u64 hashOne = zobrist::computeBoardHash(boardOne);
    u64 hashTwo = zobrist::computeBoardHash(boardTwo);

    EXPECT_EQ(hashOne, hashTwo) << "Two identical empty boards should have the same hash.";
}

TEST(ZobristHashing, DifferentPositionDifferentHash)
{
    zobrist::internals::initialize();

    Chessboard boardOne;
    Chessboard boardTwo;
    PositionEditor editorOne = boardOne.editPosition();
    PositionEditor editorTwo = boardTwo.editPosition();

    editorOne.placePieces(piece_constants::white_pawn, Square::E2);
    editorTwo.placePieces(piece_constants::black_pawn, Square::E2);

    u64 hashOne = zobrist::computeBoardHash(boardOne);
    u64 hashTwo = zobrist::computeBoardHash(boardTwo);

    EXPECT_NE(hashOne, hashTwo) << "Two different board positions should have different hashes.";
}

TEST(ZobristHashing, StartingPosition_EqualHash)
{
    zobrist::internals::initialize();

    Chessboard boardOne;
    Chessboard boardTwo;
    PositionEditor editorOne = boardOne.editPosition();
    PositionEditor editorTwo = boardTwo.editPosition();

    chess_positions::defaultStartingPosition(editorOne);
    chess_positions::defaultStartingPosition(editorTwo);

    u64 hashOne = zobrist::computeBoardHash(boardOne);
    u64 hashTwo = zobrist::computeBoardHash(boardTwo);

    EXPECT_EQ(hashOne, hashTwo) << "Two identical starting positions should have the same hash.";
    EXPECT_EQ(editorOne.hash(), editorTwo.hash()) << "Two identical starting positions should have the same hash.";
    EXPECT_EQ(hashOne, editorOne.hash()) << "Hash and editor hash should match for identical positions.";
}

TEST(ZobristHashing, PlacingPiecesAndHashingBoard_ShouldResultWithEqualHash)
{
    zobrist::internals::initialize();

    Chessboard board;
    PositionEditor editor = board.editPosition();

    chess_positions::defaultStartingPosition(editor);

    u64 initialHash = zobrist::computeBoardHash(board);

    editor.placePieces(piece_constants::white_pawn, Square::E4);
    u64 newHash = zobrist::computeBoardHash(board);

    EXPECT_NE(initialHash, newHash) << "Hash should change after placing a piece.";
    EXPECT_EQ(editor.hash(), newHash) << "Editor hash should match the new board hash.";

    editor.castling().revokeBlackKingSide();
    u64 boardHash = zobrist::computeBoardHash(board);
    EXPECT_NE(newHash, boardHash) << "Hash should change after revoking castling rights.";
    EXPECT_EQ(editor.hash(), boardHash) << "Editor hash should match the final board hash.";
}

TEST(ZobristHashing, MakeAndUnmakeMove_ShouldRestoreHash)
{
    zobrist::internals::initialize();

    GameContext game;
    game.NewGame();
    MoveExecutor executor(game);

    PositionReader positionReader = game.readChessPosition();
    u64 initialHash = positionReader.hash();

    PackedMove move(Square::E2, Square::E4);
    executor.makeMove<true>(move);
    
    u64 afterMoveHash = positionReader.hash();

    EXPECT_NE(initialHash, afterMoveHash) << "Hash should change after making a move.";
    EXPECT_TRUE(positionReader.enPassant()) << "En passant should be available after the move.";

    // test clear and reset enPassant
    u64 hashWithEnPassant = positionReader.hash();
    Square epSqr = positionReader.enPassant().readSquare();
    PositionEditor editor = game.editChessPosition();
    editor.enPassant().clear();
    EXPECT_NE(hashWithEnPassant, positionReader.hash()) << "Hash should match after clearing en passant.";

    editor.enPassant().writeSquare(epSqr);
    EXPECT_EQ(hashWithEnPassant, positionReader.hash()) << "Hash should match after resetting en passant.";

    executor.unmakeMove();

    EXPECT_EQ(initialHash, positionReader.hash()) << "Hash should be restored to initial value after unmaking the move.";

}

} // namespace ElephantTest