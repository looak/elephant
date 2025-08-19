#include <gtest/gtest.h>

#include <move/move.hpp>
#include <position/position.hpp>
#include <serializing/san_parser.hpp>

#include "chess_positions.hpp"

namespace ElephantTest {
////////////////////////////////////////////////////////////////

/**
 * @file san_parser_test.cpp
 * @brief Testing all things SAN parsing
 * Naming convention as of October 2023: <TestedFunctionality>_<ExpectedResult>
 * @author Alexander Loodin Ek  */
class SanParserFixture : public ::testing::Test {
public:
    Position testingPosition;
};
////////////////////////////////////////////////////////////////


TEST_F(SanParserFixture, ParseValidSan) {
    // setup
    PositionEditor editor(testingPosition);
    editor.placePiece(piece_constants::white_pawn, Square::E2);
    std::string san = "e4";

    // do
    PackedMove move = san_parser::deserialize(testingPosition, san);

    // verify
    EXPECT_FALSE(move.isNull());
    EXPECT_TRUE(move.isQuiet());
    EXPECT_EQ(move.targetSqr(), Square::E4);
    EXPECT_EQ(move.sourceSqr(), Square::E2);
}

} // namespace ElephantTest