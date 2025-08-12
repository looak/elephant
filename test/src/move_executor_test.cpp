#include <gtest/gtest.h>

#include <chessboard.h>
#include <game_context.h>
#include <move/move_executor.hpp>
#include <position/position.hpp>


#include "chess_positions.hpp"

namespace ElephantTest {
////////////////////////////////////////////////////////////////

/**
 * @file move_executor_test.cpp
 * @brief Testing all things move executioner, make, unmake, make(vargs), etc.
 * Naming convention as of October 2023: <TestedFunctionality>_<ExpectedResult>
 * @author Alexander Loodin Ek  */
class MoveExecutorFixture : public ::testing::Test {
public:
    Position testingPosition;
    GameState gameState;
    GameHistory gameHistory;
};
////////////////////////////////////////////////////////////////


TEST_F(MoveExecutorFixture, MakeValidMove_E2E4_UpdatesBoard) {
    
    // set up    
    chess_positions::defaultStartingPosition(testingPosition);
    MoveExecutor executor(testingPosition.edit(), gameState, gameHistory);
    PositionReader positionReader(testingPosition);

    PackedMove move(Square::E2, Square::E4);

    // do
    executor.makeMove<true>(move);

    // verify
    EXPECT_EQ(positionReader.pieceAt(Square::E4), piece_constants::white_pawn);
    EXPECT_EQ(positionReader.pieceAt(Square::E2), piece_constants::null());
}