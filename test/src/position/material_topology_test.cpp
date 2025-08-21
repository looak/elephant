#include <gtest/gtest.h>
#include "bitboard_test_helpers.hpp"
#include <material/material_topology.hpp>
#include <position/position.hpp>
#include <position/position_accessors.hpp>

namespace ElephantTest
{
////////////////////////////////////////////////////////////////

/**
 * @file material_topology_test.cpp
 * @brief Material topology is the interaction of pieces on the chessboard. Key role it plays is informing move generation.
 * Naming convention: <TestedPieceOrPieces>_<TestDescriptionOrFunctionality>_<OptionalResult>
 * @author Alexander Loodin Ek */
class MaterialTopologyTestFixture : public ::testing::Test
{
public:   
    Position testingPosition;
};

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ x ][ x ][ x ][ . ][ . ]
// 1 [ . ][ . ][ . ][ x ][ K ][ x ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(MaterialTopologyTestFixture, King_OnlyWhiteKingOnBoardAtE1)
{
    // setup
    using enum Square;
    PositionEditor editor(testingPosition);
    editor.placePiece(piece_constants::white_king, E1);

    // expected
    Bitboard expected = BitboardResultFactory::buildExpectedBoard(D1, F1, D2, E2, F2);

    // do
    Bitboard result = editor.material().topology<Set::WHITE>().computeThreatenedSquaresKing();
    EXPECT_EQ(expected, result);

    // result shouldn't change since we're isolating the colors
    editor.placePiece(piece_constants::black_king, E8);
    Bitboard resultWithBlack = editor.material().topology<Set::WHITE>().computeThreatenedSquaresKing();
    EXPECT_EQ(expected, resultWithBlack);
}


// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ x ][ x ][ x ][ . ][ . ][ . ]
// 4 [ . ][ . ][ x ][ K ][ x ][ . ][ . ][ . ]
// 3 [ . ][ . ][ x ][ x ][ x ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(MaterialTopologyTestFixture, King_WhiteKingOnD4)
{
    // setup
    using enum Square;
    PositionEditor editor(testingPosition);
    editor.placePiece(piece_constants::white_king, D4);

    // setup
    Bitboard expected = BitboardResultFactory::buildExpectedBoard(C5, D5, E5, C4, E4, C3, D3, E3);

    Bitboard result = editor.material().topology<Set::WHITE>().computeThreatenedSquaresKing();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ x ][ k ][ x ][ . ][ . ]
// 7 [ . ][ . ][ . ][ x ][ x ][ x ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(MaterialTopologyTestFixture, King_BlackKingOnE8)
{
    // setup
    using enum Square;
    PositionEditor editor(testingPosition);
    editor.placePiece(piece_constants::black_king, E8);

    // setup expected result
    Bitboard expected = BitboardResultFactory::buildExpectedBoard(D8, D7, E7, F8, F7);

    Bitboard result = editor.material().topology<Set::BLACK>().computeThreatenedSquaresKing();
    EXPECT_EQ(expected, result);
}

// 8 [ k ][ x ][ . ][ . ][ . ][ . ][ x ][ k ]
// 7 [ x ][ x ][ . ][ . ][ . ][ . ][ x ][ x ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ x ][ x ][ . ][ . ][ . ][ . ][ x ][ x ]
// 1 [ k ][ x ][ . ][ . ][ . ][ . ][ x ][ k ]
//     A    B    C    D    E    F    G    H
TEST_F(MaterialTopologyTestFixture, King_EachCorner_NoWrapAroundOfMovesOnBoard)
{
    // each corner but one corner at a time.
    // setup
    using enum Square;
    PositionEditor editor(testingPosition);
    editor.placePiece(piece_constants::black_king, A1);

    // build expected result
    Bitboard expected = BitboardResultFactory::buildExpectedBoard(A2, B1, B2);

    // do and verify
    Bitboard result = editor.material().topology<Set::BLACK>().computeThreatenedSquaresKing();
    EXPECT_EQ(expected, result); 
    
    // setup a8 corner
    editor.clear();
    editor.placePiece(piece_constants::black_king, A8);
    
    // build expected result
    expected = BitboardResultFactory::buildExpectedBoard(B8, B7, A7);

    // do and verify
    result = editor.material().topology<Set::BLACK>().computeThreatenedSquaresKing();
    EXPECT_EQ(expected, result);

    // setup h8 corner
    editor.clear();
    editor.placePiece(piece_constants::black_king, H8);

    // build expected result
    expected = BitboardResultFactory::buildExpectedBoard(H7, G8, G7);

    // do and verify
    result = editor.material().topology<Set::BLACK>().computeThreatenedSquaresKing();
    EXPECT_EQ(expected, result);

    // setup h1 corner
    editor.clear();
    editor.placePiece(piece_constants::black_king, H1);

    // build expected result
    expected = BitboardResultFactory::buildExpectedBoard(H2, G1, G2);

    // do and verify
    result = editor.material().topology<Set::BLACK>().computeThreatenedSquaresKing();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ p ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ x ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 4 [ x ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ P ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(MaterialTopologyTestFixture, Pawn_ThreatenedSquares)
{
    using enum Square;

    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces( piece_constants::white_pawn, B3,
                        piece_constants::black_pawn, B6);
    

    // expected
    Bitboard whiteExpected = BitboardResultFactory::buildExpectedBoard(A4, C4);
    Bitboard blackExpected = BitboardResultFactory::buildExpectedBoard(A5, C5);

    // do    
    Bitboard whiteResult = editor.material().topology<Set::WHITE>().computeThreatenedSquaresPawnBulk();
    Bitboard blackResult = editor.material().topology<Set::BLACK>().computeThreatenedSquaresPawnBulk();

    // validate
    EXPECT_EQ(whiteExpected, whiteResult);
    EXPECT_EQ(blackExpected, blackResult);
}


} // namespace ElephantTest