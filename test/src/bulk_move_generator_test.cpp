#include <gtest/gtest.h>
#include "bitboard_test_helpers.hpp"
#include <move_generation/bulk_move_generator.hpp>
#include <position/position.hpp>
#include <position/position_accessors.hpp>

namespace ElephantTest
{
////////////////////////////////////////////////////////////////

/**
 * @file bulk_move_generator_test.cpp
 * @brief 
 * Naming convention: <TestedFunctionality>_<TestedColor>_<ExpectedResult>
 * @author Alexander Loodin Ek */
class BulkMoveGeneratorTestFixture : public ::testing::Test
{
public:   
    Position testingPosition;
};


// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ xq][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ K ][ x ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BulkMoveGeneratorTestFixture, King_CanCaptureQueenButNotMoveInQueensRays)
{
    using enum Square;

    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces( piece_constants::white_king, Square::D5, 
                        piece_constants::black_queen, Square::C6);

    Bitboard expected = BitboardResultFactory::buildExpectedBoard(C6, D4, E5);

    // do
    BulkMoveGenerator generator(testingPosition);
    Bitboard result = generator.computeKingMoves<Set::WHITE>();

    ASSERT_EQ(result, expected);
}


// 8 [ r ][ . ][ x ][ x ][ k ][ x ][ x ][ r ]
// 7 [ . ][ . ][ . ][ x ][ x ][ x ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BulkMoveGeneratorTestFixture, King_BlackCastlingRights_AllAvailable)
{
    using enum Square;

    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces( piece_constants::black_king, Square::E8,
                        piece_constants::black_rook, Square::H8,
                        piece_constants::black_rook, Square::A8);

    editor.castling().grantAllBlack();
    
    // expected
    Bitboard expected = BitboardResultFactory::buildExpectedBoard(C8, D8, F8, G8, D7, E7, F7);

    // do
    BulkMoveGenerator generator(testingPosition);
    Bitboard result = generator.computeKingMoves<Set::BLACK>();
    EXPECT_EQ(expected, result);
}

// 8 [ r ][ . ][ . ][ x ][ k ][ x ][ . ][ r ]
// 7 [ . ][ . ][ . ][ x ][ x ][ x ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BulkMoveGeneratorTestFixture, King_BlackCastlingRights_WhiteHasAvailableShouldNotAffectBlack)
{
    using enum Square;

    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces( piece_constants::black_king, Square::E8,
                        piece_constants::black_rook, Square::H8,
                        piece_constants::black_rook, Square::A8);

    editor.castling().grantAllWhite();

        // expected
    Bitboard expected = BitboardResultFactory::buildExpectedBoard(D8, F8, D7, E7, F7);

    // do
    BulkMoveGenerator generator(testingPosition);
    Bitboard result = generator.computeKingMoves<Set::BLACK>();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ x ][ x ][ x ][ . ][ . ]
// 1 [ R ][ . ][ x ][ x ][ K ][ x ][ x ][ R ]
//     A    B    C    D    E    F    G    H
TEST_F(BulkMoveGeneratorTestFixture, King_WhiteCasstlingRights_AllAvailable)
{
    using enum Square;

    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces( piece_constants::white_king, Square::E1,
                        piece_constants::white_rook, Square::H1,
                        piece_constants::white_rook, Square::A1);

    editor.castling().grantAll();

    // expected
    Bitboard expected = BitboardResultFactory::buildExpectedBoard(C1, D1, F1, G1, D2, E2, F2);

    // do
    BulkMoveGenerator generator(testingPosition);
    Bitboard result = generator.computeKingMoves<Set::WHITE>();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ x ][ x ][ x ][ . ][ . ]
// 1 [ R ][ . ][ x ][ x ][ K ][ x ][ x ][ R ]
//     A    B    C    D    E    F    G    H
TEST_F(BulkMoveGeneratorTestFixture, King_WhiteCasstlingRights_SteppingThroughFlags)
{
    using enum Square;

    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces( piece_constants::white_king, Square::E1,
                        piece_constants::white_rook, Square::H1,
                        piece_constants::white_rook, Square::A1);

    editor.castling().clear();

    // expected
    Bitboard expected = BitboardResultFactory::buildExpectedBoard(D1, F1, D2, E2, F2);

    // do
    BulkMoveGenerator generator(testingPosition);
    Bitboard result = generator.computeKingMoves<Set::WHITE>();
    EXPECT_EQ(expected, result);


    // grant queen side
    editor.castling().grantWhiteQueenSide();
    expected[C1] = true; // queen side castling square

    // do
    result = generator.computeKingMoves<Set::WHITE>();
    EXPECT_EQ(expected, result);

    // grant king side
    editor.castling().grantWhiteKingSide();
    editor.castling().revokeWhiteQueenSide();
    expected[C1] = false; // queen side castling square
    expected[G1] = true; // king side castling square

    // do
    result = generator.computeKingMoves<Set::WHITE>();
    EXPECT_EQ(expected, result);

    // grant all
    editor.castling().grantAllWhite();
    expected[C1] = true; // queen side castling square

    // do
    result = generator.computeKingMoves<Set::WHITE>();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ x ][ x ][ x ][ . ][ . ]
// 1 [ R ][ . ][ . ][ Q ][ K ][ x ][ x ][ R ]
//     A    B    C    D    E    F    G    H
TEST_F(BulkMoveGeneratorTestFixture, King_WhiteCastlingRights_OnlyKingSideAvailableQueenBlockingQueenSide)
{
    using enum Square;

    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces( piece_constants::white_king, Square::E1,
                        piece_constants::white_queen, Square::D1,
                        piece_constants::white_rook, Square::H1,
                        piece_constants::white_rook, Square::A1);

    editor.castling().grantAllWhite();

    // expected
    Bitboard expected = BitboardResultFactory::buildExpectedBoard(F1, G1, D2, E2, F2);

    // do
    BulkMoveGenerator generator(testingPosition);
    Bitboard result = generator.computeKingMoves<Set::WHITE>();
    EXPECT_EQ(expected, result);
}


// 8 [ r ][ B ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BulkMoveGeneratorTestFixture, King_Castling_BlockedByOpponentPieceInBetween)
{
    using enum Square;

    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces( piece_constants::black_king, Square::E8,
                        piece_constants::black_rook, Square::A8,
                        piece_constants::white_bishop, Square::B8);

    editor.castling().grantBlackQueenSide();

    // expected
    Bitboard expected = BitboardResultFactory::buildExpectedBoard(D8, F8, D7, E7, F7);

    // do
    BulkMoveGenerator generator(testingPosition);
    Bitboard result = generator.computeKingMoves<Set::BLACK>();

    // validate
    EXPECT_EQ(expected, result.read());
}


// 8 [ r ][ n ][ . ][ . ][ k ][ . ][ . ][ r ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BulkMoveGeneratorTestFixture, King_Castling_BlockedByOwnPieceInBetween)
{
    using enum Square;

    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces( piece_constants::black_rook,    A8,
                        piece_constants::black_knight,  B8,
                        piece_constants::black_king,    E8);

    editor.castling().grantAllBlack();

    // expected
    Bitboard expected = BitboardResultFactory::buildExpectedBoard(D8, F8, G8, D7, E7, F7);

    // do
    BulkMoveGenerator generator(testingPosition);
    Bitboard result = generator.computeKingMoves<Set::BLACK>();

    // validate
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ B ][ . ][ R ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ N ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BulkMoveGeneratorTestFixture, King_Capture_CantCaptureGuardedPieces)
{
    using enum Square;

    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces( piece_constants::white_rook,    F7,
                        piece_constants::white_knight,  H6,
                        piece_constants::white_bishop,  D7,
                        piece_constants::black_king,    E8);

    editor.castling().clear();

    // expected
    Bitboard expected;
    expected[D8] = true;

    // do
    BulkMoveGenerator generator(testingPosition);
    Bitboard result = generator.computeKingMoves<Set::BLACK>();

    // validate
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ B ][ . ][ R ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BulkMoveGeneratorTestFixture, King_Capture_CaptureDespiteBeingChecked)
{
    using enum Square;

    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces( piece_constants::white_rook,    F7,
                        piece_constants::white_bishop,  D7,
                        piece_constants::black_king,    E8);

    editor.castling().clear();

    // expected
    Bitboard expected;
    expected[D8] = true;
    expected[F7] = true;

    // do
    BulkMoveGenerator generator(testingPosition);
    Bitboard result = generator.computeKingMoves<Set::BLACK>();

    // validate
    EXPECT_EQ(expected, result);
}


} // namespace ElephantTest