#include <gtest/gtest.h>
#include "bitboard_test_helpers.hpp"
#include <move/generation/move_bulk_generator.hpp>
#include <position/position.hpp>
#include <position/position_accessors.hpp>
#include <io/fen_parser.hpp>

namespace ElephantTest
{
////////////////////////////////////////////////////////////////

/**
 * @file move_bulk_generator_test.cpp
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


// 8 [ r ][ B ][ . ][ x ][ k ][ x ][ . ][ . ]
// 7 [ . ][ . ][ . ][ x ][ x ][ x ][ . ][ . ]
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


// 8 [ r ][ n ][ . ][ x ][ k ][ x ][ x ][ r ]
// 7 [ . ][ . ][ . ][ x ][ x ][ x ][ . ][ . ]
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

// 8 [ . ][ . ][ . ][ x ][ k ][ . ][ . ][ . ]
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

// 8 [ . ][ . ][ . ][ x ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ B ][ . ][ xR][ . ][ . ]
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

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ x ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ x ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ x ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ P ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BulkMoveGeneratorTestFixture, Pawn_WhiteBaseRank_DoublePushAvailable)
{
    using enum Square;

    // Arrange: a single white pawn on its base rank at E2
    PositionEditor editor(testingPosition);
    editor.placePiece(piece_constants::white_pawn, E2);

    // Expect: E3 and E4 are available (no blocks, no captures)
    Bitboard expected = ElephantTest::BitboardResultFactory::buildExpectedBoard(E3, E4);

    // Act
    BulkMoveGenerator generator(testingPosition);
    Bitboard result = generator.computeBulkPawnMoves<Set::WHITE>();

    // Assert
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ p ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BulkMoveGeneratorTestFixture, Pawn_BlackBulkCalc_SinglePawnOneMove)
{
    using enum Square;

    // setup
    PositionEditor editor(testingPosition);
    editor.placePiece(piece_constants::black_pawn, B3);

    // expected
    Bitboard expected = BitboardResultFactory::buildExpectedBoard(B2);

    // do
    BulkMoveGenerator generator(testingPosition);
    Bitboard result = generator.computeBulkPawnMoves<Set::BLACK>();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ p ][ . ][ . ][ . ][ p ][ . ][ p ][ . ]
// 6 [ x ][ . ][ . ][ p ][ x ][ . ][ x ][ . ]
// 5 [ x ][ . ][ . ][ x ][ x ][ . ][ p ][ . ]
// 4 [ . ][ . ][ p ][ . ][ . ][ . ][ x ][ . ]
// 3 [ . ][ p ][ x ][ . ][ . ][ . ][ . ][ . ]
// 2 [ xP][ p ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BulkMoveGeneratorTestFixture, Pawn_BlackBulkMoves_MoreThanOne)
{
   using enum Square;

    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces( piece_constants::black_pawn, B2,
                        piece_constants::black_pawn, B3,
                        piece_constants::black_pawn, C4,
                        piece_constants::black_pawn, D6,
                        piece_constants::black_pawn, A7,
                        piece_constants::black_pawn, E7,
                        piece_constants::black_pawn, G7,
                        piece_constants::black_pawn, G5,
                        piece_constants::white_pawn, A2);

    // expected
    Bitboard expected = BitboardResultFactory::buildExpectedBoard(A6, A2, A5, B1, C3, D5, E6, E5, G6, G4);

    // do
    BulkMoveGenerator generator(testingPosition);
    Bitboard result = generator.computeBulkPawnMoves<Set::BLACK>();

    // verify
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ px][ x ][ nx][ . ][ . ][ x ][ x ][ . ]
// 5 [ . ][ P ][ . ][ . ][ . ][ p ][ P ][ . ]
// 4 [ . ][ . ][ x ][ x ][ xn][ . ][ . ][ x ]
// 3 [ q ][ . ][ x ][ P ][ . ][ . ][ . ][ x ]
// 2 [ . ][ . ][ P ][ . ][ . ][ . ][ . ][ P ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BulkMoveGeneratorTestFixture, Pawn_WhiteBulkMovesAndCaptures_IncludingEnPassant)
{
    using enum Square;

    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces( 
        // White pieces
        piece_constants::white_pawn, B5,
        piece_constants::white_pawn, C2,
        piece_constants::white_pawn, D3,
        piece_constants::white_pawn, G5,
        piece_constants::white_pawn, H2,
        // Black pieces
        piece_constants::black_pawn,    A6,
        piece_constants::black_queen,   A3,
        piece_constants::black_knight,  C6,
        piece_constants::black_knight,  E4);

    editor.enPassant().writeSquare(F6);

    // expected
    Bitboard expected = BitboardResultFactory::buildExpectedBoard(A6, B6, C6, F6, G6, C4, D4, E4, H4, C3, H3);

    // do
    BulkMoveGenerator generator(testingPosition);
    Bitboard result = generator.computeBulkPawnMoves<Set::WHITE>();
        
    // verify
    EXPECT_EQ(expected, result);
}


// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ P ][ p ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ x ][ # ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BulkMoveGeneratorTestFixture, Pawn_WhiteEnPassant_ShouldBeAbleToCaptureBlackPawn)
{
    using enum Square;

    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces(
        piece_constants::black_pawn, G4,
        piece_constants::white_pawn, F4);
    
    editor.enPassant().writeSquare(F3);

    // expected
    Bitboard expected = BitboardResultFactory::buildExpectedBoard(F3, G3);

    //  do
    BulkMoveGenerator generator(testingPosition);
    Bitboard result = generator.computeBulkPawnMoves<Set::BLACK>();

    // validate
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ N ]
// 7 [ . ][ . ][ . ][ . ][ . ][ x ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ x ][ . ][ x ][ . ]
// 5 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ x ]
// 4 [ x ][ . ][ x ][ . ][ . ][ N ][ . ][ . ]
// 3 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ x ]
// 2 [ . ][ N ][ . ][ . ][ x ][ . ][ x ][ . ]
// 1 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BulkMoveGeneratorTestFixture, Knight_BunchOfWhiteKnights_ShouldIdentifyBoundsOfBoard)
{
    using enum Square;

    // setup
    PositionEditor editor(testingPosition);    
    editor.placePieces(
        piece_constants::white_knight, B2,
        piece_constants::white_knight, F4,
        piece_constants::white_knight, H8);

    // expected
    Bitboard expected = BitboardResultFactory::buildBoardFromAscii({
        ". . . . . . . .", // 8
        ". . . . . x . .", // 7
        ". . . . x . x .", // 6
        ". . . x . . . x", // 5
        "x . x . . . . .", // 4
        ". . . x . . . x", // 3 
        ". . . . x . x .", // 2 
        ". . . x . . . ."  // 1
    });
    

    // do
    BulkMoveGenerator generator(testingPosition);
    Bitboard result = generator.computeBulkKnightMoves<Set::WHITE>();

    // verify
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ x ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
// 1 [ . ][ N ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
// Visual helper demo: Build expected board using ASCII rows for readability
TEST_F(BulkMoveGeneratorTestFixture, VisualHelpers_KnightMovesFromB1)
{
    using enum Square;

    PositionEditor editor(testingPosition);
    editor.placePiece(piece_constants::white_knight, B1);

    // Expected squares using visual rows (rank 8 first, rank 1 last):
    Bitboard expected = BitboardResultFactory::buildBoardFromAscii({
        ". . . . . . . .",    // 8
        ". . . . . . . .",    // 7
        ". . . . . . . .",    // 6
        ". . . . . . . .",    // 5
        ". . . . . . . .",    // 4
        "x . x . . . . .",    // 3 => A3 and C3
        ". . . x . . . .",    // 2 => D2
        ". . . . . . . ."     // 1
    });

    BulkMoveGenerator generator(testingPosition);
    Bitboard result = generator.computeBulkKnightMoves<Set::WHITE>();

    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ x ]
// 7 [ . ][ . ][ . ][ . ][ x ][ . ][ x ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ b ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ x ][ . ][ x ][ . ]
// 4 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ x ]
// 3 [ . ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ x ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BulkMoveGeneratorTestFixture, Bishop_BlackBulkCompute_SingleBishopEmptyBoard)
{   
    // setup
    PositionEditor editor(testingPosition); 
    editor.placePiece(piece_constants::black_bishop, Square::F6);

    // expected
    Bitboard expected = BitboardResultFactory::buildBoardFromAscii({
        ". . . x . . . x",    // 8
        ". . . . x . x .",    // 7
        ". . . . . . . .",    // 6 => Bishop on F6
        ". . . . x . x .",    // 5
        ". . . x . . . x",    // 4
        ". . x . . . . .",    // 3 
        ". x . . . . . .",    // 2 
        "x . . . . . . ."     // 1
     //  A B C D E F G H 
    });

    // do
    BulkMoveGenerator generator(testingPosition);
    Bitboard result = generator.computeBulkBishopMoves<Set::BLACK>();

    // verify
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ x ][ . ][ . ]
// 7 [ x ][ . ][ . ][ . ][ x ][ . ][ . ][ . ]
// 6 [ . ][ x ][ . ][ x ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ B ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ x ][ . ][ x ][ . ][ . ][ . ][ . ]
// 3 [ x ][ . ][ . ][ . ][ xb][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BulkMoveGeneratorTestFixture, Bishop_BulkCalc_WhiteBishopAvailableCapture)
{
    // setup
    PositionEditor editor(testingPosition); 
    editor.placePieces(
        piece_constants::white_bishop, Square::C5,
        piece_constants::black_bishop, Square::E3);

    // expected
    Bitboard expected = BitboardResultFactory::buildBoardFromAscii({
        ". . . . . x . .",    // 8
        "x . . . x . . .",    // 7
        ". x . x . . . .",    // 6 
        ". . . . . . . .",    // 5 => white bishop origin square C5
        ". x . x . . . .",    // 4
        "x . . . x . . .",    // 3 => black bishop blocks further moves but is capturable 
        ". . . . . . . .",    // 2 
        ". . . . . . . ."     // 1
     //  A B C D E F G H 
    });

    // do
    BulkMoveGenerator generator(testingPosition);
    Bitboard result = generator.computeBulkBishopMoves<Set::WHITE>();

    // verify
    EXPECT_EQ(expected, result);
}

// 8 [ x ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ B ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ x ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ x ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ x ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ x ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ x ]
//     A    B    C    D    E    F    G    H
TEST_F(BulkMoveGeneratorTestFixture, Bishop_WhiteBulkCalculateAvailableMovePosition_BishopOnEmptyBoard)
{
    // setup
    PositionEditor editor(testingPosition);
    editor.placePiece(piece_constants::white_bishop, Square::B7);

    // expected
    Bitboard expected = BitboardResultFactory::buildBoardFromAscii({
        " x . x . . . . . ",    // 8
        " . . . . . . . . ",    // 7
        " x . x . . . . . ",    // 6
        " . . . x . . . . ",    // 5
        " . . . . x . . . ",    // 4
        " . . . . . x . . ",    // 3
        " . . . . . . x . ",    // 2
        " . . . . . . . x ",    // 1
     //   A B C D E F G H
    });

    // do
    BulkMoveGenerator generator(testingPosition);
    Bitboard result = generator.computeBulkBishopMoves<Set::WHITE>();

    // verify
    EXPECT_EQ(expected, result);
}

// 8 [ x ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ B ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ x ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ x ][ . ][ x ][ . ][ . ][ . ][ x ]
// 4 [ . ][ . ][ x ][ . ][ x ][ . ][ x ][ . ]
// 3 [ . ][ . ][ . ][ x ][ . ][ x ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ B ][ . ][ x ][ . ]
// 1 [ . ][ . ][ . ][ x ][ . ][ x ][ . ][ x ]
//     A    B    C    D    E    F    G    H
TEST_F(BulkMoveGeneratorTestFixture, Bishop_WhiteBulkCalculateAvailableMovePosition_TwoBishopsOnEmptyBoard)
{
    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces(
        piece_constants::white_bishop, Square::B7,
        piece_constants::white_bishop, Square::E2);

    // expected
    Bitboard expected = BitboardResultFactory::buildBoardFromAscii({
        " x . x . . . . . ",    // 8
        " . . . . . . . . ",    // 7 => white bishop on B7
        " x . x . . . . . ",    // 6
        " . x . x . . . x ",    // 5
        " . . x . x . x . ",    // 4
        " . . . x . x . . ",    // 3
        " . . . . . . x . ",    // 2 => white bishop on E2
        " . . . x . x . x ",    // 1
     //   A B C D E F G H
    });

    // do
    BulkMoveGenerator generator(testingPosition);
    Bitboard result = generator.computeBulkBishopMoves<Set::WHITE>();

    // verify
    EXPECT_EQ(expected, result);
}

TEST_F(BulkMoveGeneratorTestFixture, PerftPositionTwo_MissingSomeCaptures)
{
    using enum Square;

    // setup
    char inputFen[] = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";    
    PositionEditor editor(testingPosition);
    fen_parser::deserialize(inputFen, editor);

    // expected, eight captures in total
    Bitboard expected = BitboardResultFactory::buildExpectedBoard(
        /*Pawn D5*/ E6, 
        /*Knight E5*/ D7, F7, G6,
        /*Queen F3 */ F6, H3,
        /*Bishop E2*/ A6,
        /*Pawn G2*/ H3);

    // do
    BulkMoveGenerator generator(testingPosition);
    Bitboard pawnsCaptures = generator.computeBulkPawnMoves<Set::WHITE, MoveTypes::CAPTURES_ONLY>();
    Bitboard knightCaptures = generator.computeBulkKnightMoves<Set::WHITE, MoveTypes::CAPTURES_ONLY>();
    Bitboard queenCaptures = generator.computeBulkQueenMoves<Set::WHITE, MoveTypes::CAPTURES_ONLY>();
    Bitboard bishopCaptures = generator.computeBulkBishopMoves<Set::WHITE, bishopId,  MoveTypes::CAPTURES_ONLY>();

    Bitboard result = pawnsCaptures | knightCaptures | queenCaptures | bishopCaptures;
    EXPECT_EQ(expected, result);
}


} // namespace ElephantTest