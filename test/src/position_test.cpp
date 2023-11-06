#include "position.hpp"
#include <gtest/gtest.h>
#include <array>
#include "chess_piece.h"
#include "elephant_test_utils.h"
#include "notation.h"

namespace ElephantTest {
////////////////////////////////////////////////////////////////

/**
 * @file position_test.cpp
 * @brief Fixture for testing board position and functionality around it.
 * Naming convention as of November 2023: <Piece>_<TestedFunctionality>_<ExpectedResult>
 * @author Alexander Loodin Ek
 */
class PositionFixture : public ::testing::Test {
public:
};

////////////////////////////////////////////////////////////////

TEST_F(PositionFixture, Utils_StaticSquareValidation_JustShouldNotFail)
{
    Notation n(0);
    bool result = Position::IsValidSquare(n);
    EXPECT_TRUE(result) << Notation::toString(n) << "\n";

    n = Notation(63);
    result = Position::IsValidSquare(n);
    EXPECT_TRUE(result) << Notation::toString(n) << "\n";

    n = Notation(64);
    result = Position::IsValidSquare(n);
    EXPECT_FALSE(result) << Notation::toString(n) << "\n";

    // when index is 128 our notation will actually be 0, 0. i.e. a1
    // it's wrapped around the board.
    n = Notation(128);
    result = Position::IsValidSquare(n);
    EXPECT_TRUE(result) << Notation::toString(n) << "\n";

    for (byte i = 0; i < 64; ++i) {
        n = Notation(i);
        result = Position::IsValidSquare(n);
        EXPECT_TRUE(result) << Notation::toString(n) << "\n";
    }

    for (byte i = 64; i < 0x80; ++i) {
        result = Position::IsValidSquare(i);
        EXPECT_FALSE(result) << (int)i;
    }
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ x ][ x ][ x ][ . ][ . ]
// 1 [ . ][ . ][ . ][ x ][ K ][ x ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, King_OnlyWhiteKingOnBoard_e1_ShouldHaveMoves)
{
    Position board;
    auto K = WHITEKING;
    bool placementRes = board.PlacePiece(K, e1);
    EXPECT_EQ(true, placementRes);

    // setup
    u64 expected = ~universe;
    // d1 should be available for moving
    expected |= INT64_C(1) << d1.index();
    // f1 should be available for moving
    expected |= INT64_C(1) << f1.index();
    // d2 should be available for moving
    expected |= INT64_C(1) << d2.index();
    // e2 should be available for moving
    expected |= INT64_C(1) << e2.index();
    // f2 should be available for moving
    expected |= INT64_C(1) << f2.index();

    auto result = board.calcAvailableMovesKing<Set::WHITE>(0).read();
    EXPECT_EQ(expected, result);
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
TEST_F(PositionFixture, King_OnlyWhiteKingOnBoard_d4_ShouldHaveMoves)
{
    Position board;
    auto K = WHITEKING;
    board.PlacePiece(K, d4);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << c5.index();
    expected |= INT64_C(1) << c4.index();
    expected |= INT64_C(1) << c3.index();
    expected |= INT64_C(1) << d5.index();
    expected |= INT64_C(1) << d3.index();
    expected |= INT64_C(1) << e5.index();
    expected |= INT64_C(1) << e4.index();
    expected |= INT64_C(1) << e3.index();

    u64 result = board.calcAvailableMovesKing<Set::WHITE>(0).read();
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
TEST_F(PositionFixture, King_OnlyBlackKingOnBoard_e8_ShouldHaveMoves)
{
    Position board;
    auto k = BLACKKING;
    board.PlacePiece(k, e8);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << d8.index();
    expected |= INT64_C(1) << d7.index();
    expected |= INT64_C(1) << e7.index();
    expected |= INT64_C(1) << f8.index();
    expected |= INT64_C(1) << f7.index();

    u64 result = board.calcAvailableMovesKing<Set::BLACK>(0).read();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ xq][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ K ][ x ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, King_KinCanCapture_CaptureQueenShouldBeAvailableButCantMoveIntoQueensThreat)
{
    Position pos;
    auto K = WHITEKING;
    auto q = BLACKQUEEN;

    pos.PlacePiece(K, d5);
    pos.PlacePiece(q, c6);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << c6.index();  // capture queen
    expected |= INT64_C(1) << d4.index();  // move out of check
    expected |= INT64_C(1) << e5.index();  // move out of check

    u64 result = pos.calcAvailableMovesKing<Set::WHITE>(0).read();
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
TEST_F(PositionFixture, King_InEachCorner_NoWrapAroundOfMovesOnBoard)
{
    // each corner but one corner at a time.
    Position board;
    auto k = BLACKKING;
    board.PlacePiece(k, a1);

    // setup a1 corner
    u64 expected = ~universe;
    expected |= INT64_C(1) << a2.index();
    expected |= INT64_C(1) << b2.index();
    expected |= INT64_C(1) << b1.index();

    u64 result = board.calcAvailableMovesKing<Set::BLACK>(0).read();
    EXPECT_EQ(expected, result);

    // setup a8 corner
    board.ClearPiece(k, a1);
    board.PlacePiece(k, a8);
    expected = ~universe;
    expected |= INT64_C(1) << b8.index();
    expected |= INT64_C(1) << b7.index();
    expected |= INT64_C(1) << a7.index();

    result = board.calcAvailableMovesKing<Set::BLACK>(0).read();
    EXPECT_EQ(expected, result);

    // setup h8 corner
    board.ClearPiece(k, a8);
    board.PlacePiece(k, h8);
    expected = ~universe;
    expected |= INT64_C(1) << h7.index();
    expected |= INT64_C(1) << g8.index();
    expected |= INT64_C(1) << g7.index();

    result = board.calcAvailableMovesKing<Set::BLACK>(0).read();
    EXPECT_EQ(expected, result);

    // setup h1 corner
    board.ClearPiece(k, h8);
    board.PlacePiece(k, h1);
    expected = ~universe;
    expected |= INT64_C(1) << h2.index();
    expected |= INT64_C(1) << g1.index();
    expected |= INT64_C(1) << g2.index();

    result = board.calcAvailableMovesKing<Set::BLACK>(0).read();
    EXPECT_EQ(expected, result);
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
TEST_F(PositionFixture, King_BlackCastlingRights_AllAvailable)
{
    Position board;
    auto k = BLACKKING;
    auto r = BLACKROOK;
    board.PlacePiece(k, e8);
    board.PlacePiece(r, h8);
    board.PlacePiece(r, a8);

    u64 expected = ~universe;
    expected |= INT64_C(1) << c8.index();
    expected |= INT64_C(1) << d8.index();
    expected |= INT64_C(1) << d7.index();
    expected |= INT64_C(1) << e7.index();
    expected |= INT64_C(1) << f8.index();
    expected |= INT64_C(1) << f7.index();
    expected |= INT64_C(1) << g8.index();

    byte castling = 0xc;  // black has not moved king nor rooks and should have all castling available.
    u64 result = board.calcAvailableMovesKing<Set::BLACK>(castling).read();
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
TEST_F(PositionFixture, King_BlackCastlingRights_WhiteHasAvailableShouldNotAffectBlack)
{
    Position board;
    auto k = BLACKKING;
    auto r = BLACKROOK;
    board.PlacePiece(k, e8);
    board.PlacePiece(r, h8);
    board.PlacePiece(r, a8);

    u64 expected = ~universe;
    expected |= INT64_C(1) << d8.index();
    expected |= INT64_C(1) << d7.index();
    expected |= INT64_C(1) << e7.index();
    expected |= INT64_C(1) << f8.index();
    expected |= INT64_C(1) << f7.index();

    byte castling = 0x3;  // white has castling rights but black does not.
    u64 result = board.calcAvailableMovesKing<Set::BLACK>(castling).read();
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
TEST_F(PositionFixture, King_WhiteCasstlingRights_AllAvailable)
{
    Position board;
    auto K = WHITEKING;
    auto R = WHITEROOK;
    board.PlacePiece(K, e1);
    board.PlacePiece(R, h1);
    board.PlacePiece(R, a1);

    u64 expected = ~universe;
    expected |= INT64_C(1) << c1.index();
    expected |= INT64_C(1) << d2.index();
    expected |= INT64_C(1) << d1.index();
    expected |= INT64_C(1) << e2.index();
    expected |= INT64_C(1) << f2.index();
    expected |= INT64_C(1) << f1.index();
    expected |= INT64_C(1) << g1.index();

    byte castlingRights = 0x3;
    u64 result = board.calcAvailableMovesKing<Set::WHITE>(castlingRights).read();
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
TEST_F(PositionFixture, King_WhiteCastlingRights_OnlyKingSideAvailableQueenBlockingQueenSide)
{
    Position board;
    auto K = WHITEKING;
    auto R = WHITEROOK;
    auto Q = WHITEQUEEN;
    board.PlacePiece(K, e1);
    board.PlacePiece(R, h1);
    board.PlacePiece(R, a1);
    board.PlacePiece(Q, d1);

    u64 expected = ~universe;
    expected |= INT64_C(1) << d2.index();
    expected |= INT64_C(1) << e2.index();
    expected |= INT64_C(1) << f2.index();
    expected |= INT64_C(1) << f1.index();
    expected |= INT64_C(1) << g1.index();

    byte castlingRights = 0x3;
    u64 result = board.calcAvailableMovesKing<Set::WHITE>(castlingRights).read();
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
TEST_F(PositionFixture, King_Castling_BlockedByOpponentPieceInBetween)
{
    Position pos;

    // setup
    pos.PlacePiece(BLACKROOK, a8);
    pos.PlacePiece(WHITEBISHOP, b8);
    pos.PlacePiece(BLACKKING, e8);

    // queen side castling available
    byte castling = 8;

    u64 expected = ~universe;
    expected |= INT64_C(1) << d8.index();
    expected |= INT64_C(1) << f8.index();
    expected |= INT64_C(1) << d7.index();
    expected |= INT64_C(1) << e7.index();
    expected |= INT64_C(1) << f7.index();

    // do
    Bitboard result = pos.calcAvailableMovesKing<Set::BLACK>(castling);

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
TEST_F(PositionFixture, King_Castling_BlockedByOwnPieceInBetween)
{
    Position pos;

    // setup
    pos.PlacePiece(BLACKROOK, a8);
    pos.PlacePiece(BLACKKNIGHT, b8);
    pos.PlacePiece(BLACKKING, e8);

    // queen side castling available
    byte castling = 8 + 4;

    u64 expected = ~universe;
    expected |= INT64_C(1) << d8.index();
    expected |= INT64_C(1) << f8.index();
    expected |= INT64_C(1) << g8.index();
    expected |= INT64_C(1) << d7.index();
    expected |= INT64_C(1) << e7.index();
    expected |= INT64_C(1) << f7.index();

    // do
    Bitboard result = pos.calcAvailableMovesKing<Set::BLACK>(castling);

    // validate
    EXPECT_EQ(expected, result.read());
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ x ][ nx][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ P ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Pawn_BulkCalc_SinglePawnShouldIdentifyCaptureAsWell)
{
    Position pos;
    auto P = WHITEPAWN;
    pos.PlacePiece(P, a3);

    pos.PlacePiece(BLACKKNIGHT, b4);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << a4.index();
    expected |= INT64_C(1) << b4.index();

    KingMask empty{};
    u64 result = pos.calcAvailableMovesPawnBulk<Set::WHITE>(empty).read();
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
TEST_F(PositionFixture, Pawn_BulkCalc_SinglePawnOneMove)
{
    Position pos;
    auto p = BLACKPAWN;
    pos.PlacePiece(p, b3);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << b2.index();

    KingMask empty{};
    u64 result = pos.calcAvailableMovesPawnBulk<Set::BLACK>(empty).read();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ P ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Pawn_BulkMoveDoubleMove_SinglePawnShouldBeAbleToPushTwoRanks)
{
    Position pos;
    auto P = WHITEPAWN;
    pos.PlacePiece(P, b2);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << b4.index();
    expected |= INT64_C(1) << b3.index();

    KingMask empty{};
    u64 result = pos.calcAvailableMovesPawnBulk<Set::WHITE>(empty).read();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ N ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ P ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Pawn_Blocked_NoMovesAvailable)
{
    Position board;
    auto P = WHITEPAWN;
    auto N = WHITEKNIGHT;
    board.PlacePiece(P, c2);
    board.PlacePiece(N, c3);

    // setup
    u64 expected = 0x0;

    KingMask empty{};
    u64 result = board.calcAvailableMovesPawnBulk<Set::WHITE>(empty).read();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ p ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ x ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ x ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Pawn_BlackBaseRank_DoubleMoveAvailable)
{
    Position board;
    auto p = BLACKPAWN;
    board.PlacePiece(p, e7);
    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << e6.index();
    expected |= INT64_C(1) << e5.index();

    KingMask empty{};
    u64 result = board.calcAvailableMovesPawnBulk<Set::BLACK>(empty).read();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ p ][ . ][ p ][ . ]
// 6 [ . ][ . ][ . ][ p ][ x ][ . ][ x ][ . ]
// 5 [ . ][ . ][ . ][ x ][ x ][ . ][ x ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ p ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Pawn_BulkMovesBlack_NothingIsBlocked)
{
    // setup
    Position board;
    auto p = BLACKPAWN;

    board.PlacePiece(p, b2);
    board.PlacePiece(p, d6);
    board.PlacePiece(p, e7);
    board.PlacePiece(p, g7);

    u64 expected = ~universe;
    expected |= INT64_C(1) << b1.index();
    expected |= INT64_C(1) << d5.index();
    expected |= INT64_C(1) << e6.index();
    expected |= INT64_C(1) << e5.index();
    expected |= INT64_C(1) << g6.index();
    expected |= INT64_C(1) << g5.index();

    // do
    KingMask empty{};
    u64 result = board.calcAvailableMovesPawnBulk<Set::BLACK>(empty).read();

    // verify
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ p ][ . ][ p ][ . ]
// 6 [ . ][ . ][ p ][ p ][ x ][ . ][ n ][ . ]
// 5 [ . ][ . ][ xB][ x ][ p ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ n ][ x ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ p ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ R ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Pawn_BulkMovesBlack_SomePawnsAreBlocked)
{
    // setup
    Position board;
    auto p = BLACKPAWN;
    auto R = WHITEROOK;
    auto B = WHITEBISHOP;
    auto n = BLACKKNIGHT;

    board.PlacePiece(p, b2);
    board.PlacePiece(R, b1);

    board.PlacePiece(p, c6);
    board.PlacePiece(B, c5);

    board.PlacePiece(p, d6);
    board.PlacePiece(n, d4);

    board.PlacePiece(p, e7);
    board.PlacePiece(p, e5);
    board.PlacePiece(p, g7);
    board.PlacePiece(n, g6);

    u64 expected = ~universe;
    expected |= INT64_C(1) << d5.index();
    expected |= INT64_C(1) << c5.index();  // available capture
    expected |= INT64_C(1) << e6.index();
    expected |= INT64_C(1) << e4.index();

    // do
    KingMask empty{};
    u64 result = board.calcAvailableMovesPawnBulk<Set::BLACK>(empty).read();

    // verify
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ x ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ P ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ p ][ . ][ . ][ . ][ x ]
// 5 [ . ][ . ][ . ][ P ][ . ][ . ][ . ][ P ]
// 4 [ . ][ x ][ x ][ . ][ . ][ . ][ B ][ . ]
// 3 [ . ][ x ][ P ][ . ][ . ][ xn][ x ][ . ]
// 2 [ . ][ P ][ . ][ . ][ . ][ P ][ P ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Pawn_BulkMovesWhite_SomeMixOfBlockedAndNonBlocked)
{
    // setup
    Position board;
    auto p = BLACKPAWN;
    auto P = WHITEPAWN;
    auto B = WHITEBISHOP;
    auto n = BLACKKNIGHT;

    board.PlacePiece(P, b2);
    board.PlacePiece(P, c3);
    board.PlacePiece(P, d5);
    board.PlacePiece(p, d6);
    board.PlacePiece(P, e7);
    board.PlacePiece(P, f2);
    board.PlacePiece(n, f3);
    board.PlacePiece(P, g2);
    board.PlacePiece(B, g4);
    board.PlacePiece(P, h5);

    u64 expected = ~universe;
    expected |= INT64_C(1) << b3.index();
    expected |= INT64_C(1) << b4.index();
    expected |= INT64_C(1) << c4.index();
    expected |= INT64_C(1) << e8.index();
    expected |= INT64_C(1) << f3.index();
    expected |= INT64_C(1) << g3.index();
    expected |= INT64_C(1) << h6.index();

    // do
    KingMask empty{};
    u64 result = board.calcAvailableMovesPawnBulk<Set::WHITE>(empty).read();

    // verify
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ x ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ P ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ P ][ x ][ . ][ x ][ . ][ . ][ x ][ . ]
// 2 [ . ][ . ][ P ][ . ][ . ][ . ][ . ][ P ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Pawn_BulkThreatsWhite_ThereShouldBeAFewThreatenedSquares)
{
    // setup
    Position board;
    auto P = WHITEPAWN;

    board.PlacePiece(P, a3);
    board.PlacePiece(P, b5);
    board.PlacePiece(P, c2);
    board.PlacePiece(P, h2);

    u64 expected = ~universe;
    expected |= INT64_C(1) << a6.index();
    expected |= INT64_C(1) << b3.index();
    expected |= INT64_C(1) << b4.index();
    expected |= INT64_C(1) << c6.index();
    expected |= INT64_C(1) << d3.index();
    expected |= INT64_C(1) << g3.index();

    // do
    u64 result = board.calcThreatenedSquaresPawnBulk<Set::WHITE>().read();

    // verify
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ p ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ x ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ p ]
// 4 [ . ][ . ][ . ][ p ][ . ][ . ][ x ][ . ]
// 3 [ p ][ . ][ x ][ . ][ x ][ . ][ . ][ . ]
// 2 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Pawn_BulkThreatsBlack_ThereShouldBeAFewThreatenedSquares)
{
    // setup
    Position board;
    auto p = BLACKPAWN;

    board.PlacePiece(p, a3);
    board.PlacePiece(p, b7);
    board.PlacePiece(p, d4);
    board.PlacePiece(p, h5);

    u64 expected = ~universe;
    expected |= INT64_C(1) << a6.index();
    expected |= INT64_C(1) << b2.index();
    expected |= INT64_C(1) << c6.index();
    expected |= INT64_C(1) << c3.index();
    expected |= INT64_C(1) << e3.index();
    expected |= INT64_C(1) << g4.index();

    // do
    u64 result = board.calcThreatenedSquaresPawnBulk<Set::BLACK>().read();

    // verify
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ px][ . ][ nx][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ P ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ xn][ . ][ . ][ . ]
// 3 [ q ][ . ][ . ][ P ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ P ][ . ][ . ][ . ][ . ][ P ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Pawn_BulkAttacksWhite_ThereShouldBeAFewAttackedPieces)
{
    // setup
    Position board;
    auto P = WHITEPAWN;
    auto p = BLACKPAWN;
    auto n = BLACKKNIGHT;
    auto q = BLACKQUEEN;

    board.PlacePiece(P, b5);
    board.PlacePiece(P, c2);
    board.PlacePiece(P, d3);
    board.PlacePiece(P, h2);

    board.PlacePiece(p, a6);
    board.PlacePiece(q, a3);
    board.PlacePiece(n, c6);
    board.PlacePiece(n, e4);

    u64 expected = ~universe;
    expected |= INT64_C(1) << a6.index();
    expected |= INT64_C(1) << c6.index();
    expected |= INT64_C(1) << e4.index();

    // do
    KingMask empty{};
    u64 result = board.calcAvailableAttacksPawnBulk<Set::WHITE>(empty).read();

    // verify
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ p ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ xB][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 4 [ x ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ P ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Pawn_ThreatenVsAttack_ThreatAndAttacksAreNotTheSameThing)
{
    Position board;
    auto P = WHITEPAWN;
    auto p = BLACKPAWN;
    auto B = WHITEBISHOP;

    board.PlacePiece(P, b3);
    board.PlacePiece(p, b6);
    board.PlacePiece(B, a5);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << a4.index();
    expected |= INT64_C(1) << c4.index();

    // do
    u64 result = board.calcThreatenedSquaresPawnBulk<Set::WHITE>().read();
    // validate
    EXPECT_EQ(expected, result);

    // setup
    expected = ~universe;
    expected |= INT64_C(1) << a5.index();
    expected |= INT64_C(1) << c5.index();
    // do
    result = board.calcThreatenedSquaresPawnBulk<Set::BLACK>().read();
    // validate
    EXPECT_EQ(expected, result);

    // setup
    expected = ~universe;
    expected |= INT64_C(1) << a5.index();
    // do
    KingMask empty{};
    result = board.calcAvailableAttacksPawnBulk<Set::BLACK>(empty).read();
    // validate
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ p ][ P ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ x ][ x ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Pawn_EnPassant_BlackShouldBeAbleToCaptureWhitePawn)
{
    Position pos;
    auto P = WHITEPAWN;
    auto p = BLACKPAWN;

    // setup
    pos.PlacePiece(P, g4);
    pos.PlacePiece(p, f4);
    pos.writeEnPassant().writeEnPassant(g3.toSquare(), Set::WHITE);
    //  do
    KingMask empty{};
    u64 result = pos.calcAvailableMovesPawnBulk<Set::BLACK>(empty).read();

    // validate
    u64 expected = ~universe;
    expected |= INT64_C(1) << f3.index();
    expected |= INT64_C(1) << g3.index();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ x ][ x ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ p ][ P ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Pawn_EnPassantWhitePerspective_AvailableEnPassantCapture)
{
    Position pos;
    auto P = WHITEPAWN;
    auto p = BLACKPAWN;

    // setup
    pos.PlacePiece(P, g5);
    pos.PlacePiece(p, f5);
    pos.writeEnPassant().writeEnPassant(f6.toSquare(), Set::BLACK);

    // do
    KingMask empty{};
    u64 result = pos.calcAvailableMovesPawnBulk<Set::WHITE>(empty).read();

    // validate
    u64 expected = ~universe;
    expected |= INT64_C(1) << f6.index();
    expected |= INT64_C(1) << g6.index();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ k ][ p ][ . ][ R ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Pawn_PinnedPieceBlack_NoAvailableMovesSinceItsPinned)
{
    Position pos;
    auto p = BLACKPAWN;
    auto k = BLACKKING;
    auto R = WHITEROOK;

    // setup
    pos.PlacePiece(p, f7);
    pos.PlacePiece(k, e7);
    pos.PlacePiece(R, h7);

    // do
    KingMask kingMask = pos.calcKingMask<Set::BLACK>();
    Bitboard pawnMoves = pos.calcAvailableMovesPawnBulk<Set::BLACK>(kingMask);

    auto [nonattacks, attacks] = pos.isolatePiece<Set::BLACK, pawnId>(f7, pawnMoves, kingMask);

    // validate
    u64 expected = 0x0;
    EXPECT_EQ(expected, nonattacks.read());
    EXPECT_EQ(expected, attacks.read());
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ P ][ . ][ K ][ P ][ . ][ r ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Pawn_PinnedPieceWhite_NoAvailableMovesSinceItsPinned)
{
    Position pos;
    auto P = WHITEPAWN;
    auto K = WHITEKING;
    auto r = BLACKROOK;

    // setup
    pos.PlacePiece(P, f5);
    pos.PlacePiece(P, c5);
    pos.PlacePiece(K, e5);
    pos.PlacePiece(r, h5);

    // do
    KingMask kingMask = pos.calcKingMask<Set::WHITE>();
    Bitboard pawnMoves = pos.calcAvailableMovesPawnBulk<Set::WHITE>(kingMask);
    {
        auto [nonattacks, attacks] = pos.isolatePiece<Set::WHITE, pawnId>(f5, pawnMoves, kingMask);

        // validate
        u64 expected = 0x0;
        EXPECT_EQ(expected, nonattacks.read());
        EXPECT_EQ(expected, attacks.read());
    }

    auto [nonattacks, attacks] = pos.isolatePiece<Set::WHITE, pawnId>(c5, pawnMoves, kingMask);

    // validate
    u64 expected = 0x40000000000ull;
    EXPECT_EQ(expected, nonattacks.read());
    expected = 0x0;
    EXPECT_EQ(expected, attacks.read());
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ b ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ P ][ . ][ . ]
// 5 [ . ][ . ][ P ][ . ][ K ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Pawn_PinnedPieceWhite_CanCapturePinningBishop)
{
    Position pos;
    auto P = WHITEPAWN;
    auto K = WHITEKING;
    auto b = BLACKBISHOP;

    // setup
    pos.PlacePiece(P, f6);
    pos.PlacePiece(P, c5);
    pos.PlacePiece(K, e5);
    pos.PlacePiece(b, g7);

    // do
    KingMask kingMask = pos.calcKingMask<Set::WHITE>();
    Bitboard pawnMoves = pos.calcAvailableMovesPawnBulk<Set::WHITE>(kingMask);
    {
        auto [nonattacks, attacks] = pos.isolatePiece<Set::WHITE, pawnId>(f6, pawnMoves, kingMask);

        // validate
        u64 expected = 0x0;
        EXPECT_EQ(expected, nonattacks.read());
        expected = 0x40000000000000ull;
        EXPECT_EQ(expected, attacks.read());
    }

    auto [nonattacks, attacks] = pos.isolatePiece<Set::WHITE, pawnId>(c5, pawnMoves, kingMask);

    // validate
    u64 expected = 0x40000000000ull;
    EXPECT_EQ(expected, nonattacks.read());
    expected = 0x0;
    EXPECT_EQ(expected, attacks.read());
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ p ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ R ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Pawn_KingMask_PawnCanMoveIntoBlockingCheckButNoFurther)
{
    Position pos;
    auto p = BLACKPAWN;
    auto k = BLACKKING;
    auto R = WHITEROOK;

    // setup
    pos.PlacePiece(p, f7);
    pos.PlacePiece(k, e6);
    pos.PlacePiece(R, h6);

    // do
    KingMask kingMask = pos.calcKingMask<Set::BLACK>();
    Bitboard pawnMoves = pos.calcAvailableMovesPawnBulk<Set::BLACK>(kingMask);
    {
        auto [nonattacks, attacks] = pos.isolatePiece<Set::BLACK, pawnId>(f7, pawnMoves, kingMask);

        // validate
        u64 expected = 0x200000000000ull;
        EXPECT_EQ(expected, nonattacks.read());
        expected = 0x0;
        EXPECT_EQ(expected, attacks.read());
    }
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ p ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ k ][ . ][ R ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Pawn_KingMask_CanCaptureCheckingPiece)
{
    Position pos;
    auto p = BLACKPAWN;
    auto k = BLACKKING;
    auto R = WHITEROOK;

    // setup
    pos.PlacePiece(p, f7);
    pos.PlacePiece(k, e6);
    pos.PlacePiece(R, g6);

    // do
    KingMask kingMask = pos.calcKingMask<Set::BLACK>();
    Bitboard pawnMoves = pos.calcAvailableMovesPawnBulk<Set::BLACK>(kingMask);
    {
        auto [nonattacks, attacks] = pos.isolatePiece<Set::BLACK, pawnId>(f7, pawnMoves, kingMask);

        // validate
        u64 expected = 0x200000000000ull;
        EXPECT_EQ(expected, nonattacks.read());
        expected = 0x400000000000;
        EXPECT_EQ(expected, attacks.read());
    }
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ p ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ k ][ . ][ R ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Pawn_KingMask_CanOnlyDoubleMoveToBlockCheck)
{
    Position pos;
    auto p = BLACKPAWN;
    auto k = BLACKKING;
    auto R = WHITEROOK;

    // setup
    pos.PlacePiece(p, f7);
    pos.PlacePiece(k, e5);
    pos.PlacePiece(R, g5);

    // do
    KingMask kingMask = pos.calcKingMask<Set::BLACK>();
    Bitboard pawnMoves = pos.calcAvailableMovesPawnBulk<Set::BLACK>(kingMask);
    {
        auto [nonattacks, attacks] = pos.isolatePiece<Set::BLACK, pawnId>(f7, pawnMoves, kingMask);

        // validate
        u64 expected = 0x2000000000ull;
        EXPECT_EQ(expected, nonattacks.read());
        expected = 0x0ull;
        EXPECT_EQ(expected, attacks.read());
    }
}

TEST_F(PositionFixture, Pawn_IsolatingPiece_PawnOnDifferentRankAndFile)
{
    Position board;
    board.PlacePiece(WHITEPAWN, d4);
    board.PlacePiece(WHITEPAWN, c2);

    u64 expected = 0x804040000;
    KingMask empty{};
    u64 movesbb = board.calcAvailableMovesPawnBulk<Set::WHITE>(empty).read();
    EXPECT_EQ(expected, movesbb);

    {
        expected = 0x800000000;
        auto [moves, attks] = board.isolatePiece<Set::WHITE, pawnId>(d4, movesbb, empty);
        EXPECT_EQ(expected, moves.read());
    }

    expected = 0x4040000;
    auto [moves, attks] = board.isolatePiece<Set::WHITE, pawnId>(c2, movesbb, empty);
    EXPECT_EQ(expected, moves.read());
}

TEST_F(PositionFixture, Pawn_IsolatingPiece_PawnOnSameRank)
{
    Position board;
    board.PlacePiece(WHITEPAWN, d4);
    board.PlacePiece(WHITEPAWN, e4);

    u64 expected = 0x1800000000;
    KingMask empty{};
    u64 movesbb = board.calcAvailableMovesPawnBulk<Set::WHITE>(empty).read();
    EXPECT_EQ(expected, movesbb);

    {
        expected = 0x800000000;
        auto [moves, attks] = board.isolatePiece<Set::WHITE, pawnId>(d4, movesbb, empty);
        EXPECT_EQ(expected, moves.read());
    }

    expected = 0x1000000000;
    auto [moves, attks] = board.isolatePiece<Set::WHITE, pawnId>(e4, movesbb, empty);
    EXPECT_EQ(expected, moves.read());
}

TEST_F(PositionFixture, Pawn_IsolatingPiece_PawnOnSameFile)
{
    Position board;
    board.PlacePiece(WHITEPAWN, d4);
    board.PlacePiece(WHITEPAWN, d2);

    u64 expected = 0x800080000;
    KingMask empty{};
    u64 movesbb = board.calcAvailableMovesPawnBulk<Set::WHITE>(empty).read();
    EXPECT_EQ(expected, movesbb);

    {
        expected = 0x800000000;
        auto [moves, attks] = board.isolatePiece<Set::WHITE, pawnId>(d4, movesbb, empty);
        EXPECT_EQ(expected, moves.read());
    }

    expected = 0x80000;
    auto [moves, attks] = board.isolatePiece<Set::WHITE, pawnId>(d2, movesbb, empty);
    EXPECT_EQ(expected, moves.read());
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ p ][ P ][ p ]
// 3 [ . ][ . ][ . ][ . ][ . ][ x ][ x ][ x ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Pawn_EnPassantIsolate_BothBlackPawnsShouldBeAbleToCaptureWhitePawn)
{
    Position pos;
    auto P = WHITEPAWN;
    auto p = BLACKPAWN;

    // setup
    pos.PlacePiece(P, g4);
    pos.PlacePiece(p, f4);
    pos.PlacePiece(p, h4);
    pos.writeEnPassant().writeEnPassant(g3.toSquare(), Set::WHITE);
    KingMask empty{};
    Bitboard movesbb = pos.calcAvailableMovesPawnBulk<Set::BLACK>(empty);

    {
        //  do
        auto [nonattks, attks] = pos.isolatePiece<Set::BLACK, pawnId>(f4, movesbb, empty);
        // validate
        Bitboard expected = ~universe;
        expected |= INT64_C(1) << f3.index();
        EXPECT_EQ(expected, nonattks);
        expected = INT64_C(1) << g3.index();
        EXPECT_EQ(expected, attks);
    }

    //  do
    auto [nonattks, attks] = pos.isolatePiece<Set::BLACK, pawnId>(h4, movesbb, empty);
    // validate
    Bitboard expected = ~universe;
    expected |= INT64_C(1) << h3.index();
    EXPECT_EQ(expected, nonattks);
    expected = INT64_C(1) << g3.index();
    EXPECT_EQ(expected, attks);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ x ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
// 2 [ . ][ N ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Knight_MovesFromB2_ShouldIdentifyBoundsOfBoard)
{
    Position board;
    auto N = WHITEKNIGHT;
    board.PlacePiece(N, b2);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << a4.index();
    expected |= INT64_C(1) << c4.index();
    expected |= INT64_C(1) << d3.index();
    expected |= INT64_C(1) << d1.index();

    KingMask empty{};
    u64 result = board.calcAvailableMovesKnightBulk<Set::WHITE>(empty).read();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ x ][ . ][ x ][ . ][ . ][ . ][ . ]
// 4 [ x ][ . ][ . ][ . ][ x ][ . ][ . ][ . ]
// 3 [ . ][ . ][ N ][ . ][ . ][ . ][ . ][ . ]
// 2 [ x ][ . ][ . ][ . ][ x ][ . ][ . ][ . ]
// 1 [ . ][ x ][ . ][ x ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Knight_AvailableMoves_SimplestCaseResultShouldHaveEightBitsSet)
{
    Position pos;
    auto N = WHITEKNIGHT;

    // setup
    pos.PlacePiece(N, c3);

    u64 expected = ~universe;
    expected |= INT64_C(1) << b1.index();
    expected |= INT64_C(1) << d1.index();
    expected |= INT64_C(1) << a2.index();
    expected |= INT64_C(1) << e2.index();
    expected |= INT64_C(1) << a4.index();
    expected |= INT64_C(1) << e4.index();
    expected |= INT64_C(1) << b5.index();
    expected |= INT64_C(1) << d5.index();

    // do
    KingMask empty{};
    u64 result = pos.calcAvailableMovesKnightBulk<Set::WHITE>(empty).read();

    // validate
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ x ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
// 5 [ . ][ N ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
// 3 [ x ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Knight_CloseToEdge_CanNotPlaceKnightOutsideOfBoardAndShouldNotWrap)
{
    Position pos;
    auto N = WHITEKNIGHT;
    pos.PlacePiece(N, b5);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << a7.index();
    expected |= INT64_C(1) << a3.index();
    expected |= INT64_C(1) << c7.index();
    expected |= INT64_C(1) << c3.index();
    expected |= INT64_C(1) << d6.index();
    expected |= INT64_C(1) << d4.index();

    KingMask empty{};
    u64 result = pos.calcAvailableMovesKnightBulk<Set::WHITE>(empty).read();
    EXPECT_EQ(expected, result);
}

// board position
// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ b ][ . ][ B ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ N ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
// expected result
// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Knight_AttackedPieces_ThreateningToCaptureOpponentsBishop)
{
    Position board;
    auto N = WHITEKNIGHT;
    auto B = WHITEBISHOP;
    auto b = BLACKBISHOP;

    board.PlacePiece(N, e3);
    board.PlacePiece(B, f5);
    board.PlacePiece(b, d5);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << d5.index();

    // as of November 2023 I don't see a reason to supply a attacked function
    u64 result = board.calcThreatenedSquaresKnightBulk<Set::WHITE>().read();
    u64 opMat = board.readMaterial<Set::BLACK>().combine().read();
    result &= opMat;
    EXPECT_EQ(expected, result);
}

TEST_F(PositionFixture, Knight_Threat_ClearPieceFromBoard)
{
    Position board;
    auto N = WHITEKNIGHT;
    auto B = WHITEBISHOP;
    auto b = BLACKBISHOP;

    board.PlacePiece(N, e3);
    board.PlacePiece(B, f5);
    board.PlacePiece(b, d5);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << d5.index();

    u64 result = board.calcAttackedSquares(e3, N);
    EXPECT_EQ(expected, result);

    // clear piece
    board.ClearPiece(b, d5);

    expected = ~universe;
    result = board.calcAttackedSquares(e3, N);
    EXPECT_EQ(expected, result);
}

TEST_F(PositionFixture, Knight_IsolatingPiece_TwoKnightsNotSharingSquaresButBlockingEachOther)
{
    Position board;
    board.PlacePiece(WHITEKNIGHT, d4);
    board.PlacePiece(WHITEKNIGHT, e2);

    u64 expected = 0x142220660444ull;
    KingMask empty{};
    u64 movesbb = board.calcAvailableMovesKnightBulk<Set::WHITE>(empty).read();
    EXPECT_EQ(expected, movesbb);

    {
        expected = 0x142200220400ull;
        auto [moves, attks] = board.isolatePiece<Set::WHITE, knightId>(d4, movesbb, empty);
        EXPECT_EQ(expected, moves.read());
    }

    expected = 0x20440044ull;
    auto [moves, attks] = board.isolatePiece<Set::WHITE, knightId>(e2, movesbb, empty);
    EXPECT_EQ(expected, moves.read());
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
TEST_F(PositionFixture, Bishop_BulkCalc_SingleBishopEmptyBoardBlack)
{
    Position board;
    auto b = BLACKBISHOP;
    board.PlacePiece(b, f6);
    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << a1.index();
    expected |= INT64_C(1) << b2.index();
    expected |= INT64_C(1) << c3.index();
    expected |= INT64_C(1) << d8.index();
    expected |= INT64_C(1) << d4.index();
    expected |= INT64_C(1) << e7.index();
    expected |= INT64_C(1) << e5.index();
    expected |= INT64_C(1) << g7.index();
    expected |= INT64_C(1) << g5.index();
    expected |= INT64_C(1) << h8.index();
    expected |= INT64_C(1) << h4.index();

    KingMask empty{};
    u64 result = board.calcAvailableMovesBishopBulk<Set::BLACK>(empty).read();
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
TEST_F(PositionFixture, Bishop_BulkCalc_WhiteBishopAvailableCapture)
{
    Position board;
    auto b = BLACKBISHOP;
    auto B = WHITEBISHOP;

    board.PlacePiece(b, e3);
    board.PlacePiece(B, c5);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << a7.index();
    expected |= INT64_C(1) << a3.index();
    expected |= INT64_C(1) << b6.index();
    expected |= INT64_C(1) << b4.index();
    expected |= INT64_C(1) << d6.index();
    expected |= INT64_C(1) << d4.index();
    expected |= INT64_C(1) << e7.index();
    expected |= INT64_C(1) << e3.index();
    expected |= INT64_C(1) << f8.index();
    KingMask empty{};
    u64 result = board.calcAvailableMovesBishopBulk<Set::WHITE>(empty).read();
    EXPECT_EQ(expected, result);
}

// 8 [ x ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ b ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ x ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ x ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ x ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ x ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ x ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Bishop_BulkCalculateAvailableMovePosition_BishopOnEmptyBoard)
{
    Position board;
    auto b = BLACKBISHOP;

    board.PlacePiece(b, b7);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << a8.index();
    expected |= INT64_C(1) << a6.index();
    expected |= INT64_C(1) << c8.index();
    expected |= INT64_C(1) << c6.index();
    expected |= INT64_C(1) << d5.index();
    expected |= INT64_C(1) << e4.index();
    expected |= INT64_C(1) << f3.index();
    expected |= INT64_C(1) << g2.index();
    expected |= INT64_C(1) << h1.index();
    KingMask empty{};
    u64 result = board.calcAvailableMovesBishopBulk<Set::BLACK>(empty).read();
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
TEST_F(PositionFixture, Bishop_BulkCalculateAvailableMovePosition_TwoWhiteBishopsOnEmptyBoard)
{
    Position board;
    auto B = WHITEBISHOP;

    board.PlacePiece(B, b7);
    board.PlacePiece(B, e2);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << a8.index();
    expected |= INT64_C(1) << a6.index();

    expected |= INT64_C(1) << b5.index();

    expected |= INT64_C(1) << c8.index();
    expected |= INT64_C(1) << c6.index();
    expected |= INT64_C(1) << c4.index();

    expected |= INT64_C(1) << d5.index();
    expected |= INT64_C(1) << d3.index();
    expected |= INT64_C(1) << d1.index();

    expected |= INT64_C(1) << e4.index();

    expected |= INT64_C(1) << f3.index();
    expected |= INT64_C(1) << f1.index();

    expected |= INT64_C(1) << g4.index();
    expected |= INT64_C(1) << g2.index();

    expected |= INT64_C(1) << h5.index();
    expected |= INT64_C(1) << h1.index();
    KingMask empty{};
    u64 result = board.calcAvailableMovesBishopBulk<Set::WHITE>(empty).read();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ x ][ . ][ x ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ b ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ x ][ . ][ x ]
// 5 [ . ][ . ][ . ][ . ][ x ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ x ][ . ][ b ][ . ][ . ][ . ][ . ]
// 1 [ x ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Bishop_IsolatingPiece_OnlyOneBishopLeftInTheMask)
{
    Position board;
    board.PlacePiece(BLACKBISHOP, g7);
    board.PlacePiece(BLACKBISHOP, d2);

    u64 expected = 0xa000a0512a140215ull;
    KingMask empty{};
    u64 movesbb = board.calcAvailableMovesBishopBulk<Set::BLACK>(empty).read();
    EXPECT_EQ(expected, movesbb);
    {
        expected = 0xa000a01008040201ull;
        auto [moves, attacks] = board.isolatePiece<Set::BLACK, bishopId>(g7, movesbb, empty);
        EXPECT_EQ(expected, moves.read());
    }

    {
        expected = 0x804122140014ull;
        auto [moves, attacks] = board.isolatePiece<Set::BLACK, bishopId>(d2, movesbb, empty);
        EXPECT_EQ(expected, moves.read());
    }
}

TEST_F(PositionFixture, Bishop_IsolatingPiece_BishopsOnSameDiagonal)
{
    Position board;
    board.PlacePiece(BLACKBISHOP, c4);
    board.PlacePiece(BLACKBISHOP, e2);

    u64 expected = 0x4020118a402a0128ull;
    KingMask empty{};
    u64 movesbb = board.calcAvailableMovesBishopBulk<Set::BLACK>(empty).read();
    EXPECT_EQ(expected, movesbb);

    {
        expected = 0x4020110a000a0100ull;
        auto [moves, attks] = board.isolatePiece<Set::BLACK, bishopId>(c4, movesbb, empty);
        EXPECT_EQ(expected, moves.read());
    }

    {
        expected = 0x8040280028ull;
        auto [moves, attks] = board.isolatePiece<Set::BLACK, bishopId>(e2, movesbb, empty);
        EXPECT_EQ(expected, moves.read());
    }
}

// 8 [ R ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ b ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Bishop_KingMask_AvailableMovesAreCaptureCheckingPieceOrBlockingCheck)
{
    Position pos;

    // setup
    pos.PlacePiece(WHITEROOK, a8);
    pos.PlacePiece(BLACKBISHOP, b7);
    pos.PlacePiece(BLACKKING, e8);

    // do
    KingMask kingMask = pos.calcKingMask<Set::BLACK>();
    Bitboard pawnMoves = pos.calcAvailableMovesBishopBulk<Set::BLACK>(kingMask);
    {
        auto [nonattacks, attacks] = pos.isolatePiece<Set::BLACK, bishopId>(b7, pawnMoves, kingMask);

        // validate
        u64 expected = 0x400000000000000ull;
        EXPECT_EQ(expected, nonattacks.read());
        expected = 0x100000000000000ull;
        EXPECT_EQ(expected, attacks.read());
    }
}

/**
 * 8 [ x ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
 * 7 [ . ][ x ][ . ][ . ][ . ][ k ][ . ][ . ]
 * 6 [ . ][ . ][ x ][ . ][ x ][ . ][ . ][ . ]
 * 5 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ x ]
 * 4 [ . ][ . ][ b ][ . ][ x ][ . ][ x ][ . ]
 * 3 [ . ][ x ][ . ][ . ][ . ][ b ][ . ][ . ]
 * 2 [ B ][ . ][ . ][ . ][ x ][ . ][ x ][ . ]
 * 1 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ x ]
 *     A    B    C    D    E    F    G    H
 * @brief Even though the bishop is pinned, it is able to move along the threatened squares. */
TEST_F(PositionFixture, Bishop_IsolatePinnedPiece_AbleToMoveAlongThreatenedSquares)
{
    Position pos;
    auto b = BLACKBISHOP;
    auto k = BLACKKING;
    auto B = WHITEBISHOP;

    // setup
    pos.PlacePiece(b, c4);
    pos.PlacePiece(k, f7);
    pos.PlacePiece(B, a2);
    pos.PlacePiece(b, f3);

    // do
    KingMask kingMask = pos.calcKingMask<Set::BLACK>();
    Bitboard bishopMoves = pos.calcAvailableMovesBishopBulk<Set::BLACK>(kingMask);
    {
        auto [nonattacks, attacks] = pos.isolatePiece<Set::BLACK, bishopId>(c4, bishopMoves, kingMask);

        // validate
        u64 expected = 0x100800020000ull;
        EXPECT_EQ(expected, nonattacks.read());
        expected = 0x100ull;
        EXPECT_EQ(expected, attacks.read());
    }

    auto [nonattacks, attacks] = pos.isolatePiece<Set::BLACK, bishopId>(f3, bishopMoves, kingMask);

    // validate
    u64 expected = 0x102048850005088ull;
    EXPECT_EQ(expected, nonattacks.read());
    expected = 0x0ull;
    EXPECT_EQ(expected, attacks.read());
}

// 8 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
// 4 [ x ][ x ][ x ][ R ][ x ][ x ][ x ][ x ]
// 3 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Rook_BulkCalculateThreat_EmptyBoardWhite)
{
    Position board;
    auto R = WHITEROOK;
    board.PlacePiece(R, d4);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << a4.index();
    expected |= INT64_C(1) << b4.index();
    expected |= INT64_C(1) << c4.index();
    expected |= INT64_C(1) << e4.index();
    expected |= INT64_C(1) << f4.index();
    expected |= INT64_C(1) << g4.index();
    expected |= INT64_C(1) << h4.index();
    expected |= INT64_C(1) << d1.index();
    expected |= INT64_C(1) << d2.index();
    expected |= INT64_C(1) << d3.index();
    expected |= INT64_C(1) << d5.index();
    expected |= INT64_C(1) << d6.index();
    expected |= INT64_C(1) << d7.index();
    expected |= INT64_C(1) << d8.index();

    u64 result = board.calcThreatenedSquaresRookBulk<Set::WHITE>().read();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ x ][ r ][ x ][ x ][ x ][ x ][ x ][ x ]
// 6 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Rook_BulkCalculateThreatenedSquares_EmptyBoardBlack)
{
    Position board;
    auto r = BLACKROOK;

    board.PlacePiece(r, b7);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << a7.index();
    expected |= INT64_C(1) << c7.index();
    expected |= INT64_C(1) << d7.index();
    expected |= INT64_C(1) << e7.index();
    expected |= INT64_C(1) << f7.index();
    expected |= INT64_C(1) << g7.index();
    expected |= INT64_C(1) << h7.index();
    expected |= INT64_C(1) << b1.index();
    expected |= INT64_C(1) << b2.index();
    expected |= INT64_C(1) << b3.index();
    expected |= INT64_C(1) << b4.index();
    expected |= INT64_C(1) << b5.index();
    expected |= INT64_C(1) << b6.index();
    expected |= INT64_C(1) << b8.index();

    u64 result = board.calcThreatenedSquaresRookBulk<Set::BLACK>().read();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ x ]
// 7 [ x ][ r ][ x ][ x ][ x ][ x ][ x ][ x ]
// 6 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ x ]
// 5 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ x ]
// 4 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ x ]
// 3 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ x ]
// 2 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ x ]
// 1 [ x ][ x ][ x ][ x ][ x ][ x ][ x ][ r ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Rook_BulkCalculateThreatenedSquares_TwoBlackRooksOnEmptyBoard)
{
    Position board;
    auto r = BLACKROOK;

    board.PlacePiece(r, b7);
    board.PlacePiece(r, h1);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << a1.index();
    expected |= INT64_C(1) << a7.index();
    expected |= INT64_C(1) << b1.index();
    expected |= INT64_C(1) << c1.index();
    expected |= INT64_C(1) << c7.index();
    expected |= INT64_C(1) << d1.index();
    expected |= INT64_C(1) << d7.index();
    expected |= INT64_C(1) << e1.index();
    expected |= INT64_C(1) << e7.index();
    expected |= INT64_C(1) << f1.index();
    expected |= INT64_C(1) << f7.index();
    expected |= INT64_C(1) << g7.index();
    expected |= INT64_C(1) << g1.index();
    expected |= INT64_C(1) << h8.index();
    expected |= INT64_C(1) << h7.index();
    expected |= INT64_C(1) << h6.index();
    expected |= INT64_C(1) << h5.index();
    expected |= INT64_C(1) << h4.index();
    expected |= INT64_C(1) << h3.index();
    expected |= INT64_C(1) << h2.index();
    expected |= INT64_C(1) << b2.index();
    expected |= INT64_C(1) << b3.index();
    expected |= INT64_C(1) << b4.index();
    expected |= INT64_C(1) << b5.index();
    expected |= INT64_C(1) << b6.index();
    expected |= INT64_C(1) << b8.index();

    u64 result = board.calcThreatenedSquaresRookBulk<Set::BLACK>().read();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ x ][ . ][ . ][ x ][ . ][ . ][ . ]
// 7 [ x ][ r ][ x ][ x ][ r ][ x ][ n ][ . ]
// 6 [ . ][ x ][ . ][ . ][ x ][ . ][ . ][ . ]
// 5 [ . ][ x ][ . ][ . ][ x ][ . ][ . ][ . ]
// 4 [ . ][ x ][ . ][ . ][ x ][ . ][ . ][ . ]
// 3 [ . ][ x ][ . ][ . ][ x ][ . ][ . ][ . ]
// 2 [ . ][ x ][ . ][ . ][ x ][ . ][ . ][ . ]
// 1 [ . ][ x ][ . ][ . ][ x ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Rook_BulkCalculateAvailableMovePosition_TwoRooksMoveBlockEachOtherAndByKnight)
{
    Position board;
    auto r = BLACKROOK;
    auto n = BLACKKNIGHT;
    board.PlacePiece(r, b7);
    board.PlacePiece(r, e7);
    board.PlacePiece(n, g7);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << a7.index();

    expected |= INT64_C(1) << b1.index();
    expected |= INT64_C(1) << b2.index();
    expected |= INT64_C(1) << b3.index();
    expected |= INT64_C(1) << b4.index();
    expected |= INT64_C(1) << b5.index();
    expected |= INT64_C(1) << b6.index();
    expected |= INT64_C(1) << b8.index();

    expected |= INT64_C(1) << c7.index();

    expected |= INT64_C(1) << d7.index();

    expected |= INT64_C(1) << e8.index();
    expected |= INT64_C(1) << e6.index();
    expected |= INT64_C(1) << e5.index();
    expected |= INT64_C(1) << e4.index();
    expected |= INT64_C(1) << e3.index();
    expected |= INT64_C(1) << e2.index();
    expected |= INT64_C(1) << e1.index();

    expected |= INT64_C(1) << f7.index();

    KingMask empty{};
    u64 result = board.calcAvailableMovesRookBulk<Set::BLACK>(empty).read();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 1 [ x ][ x ][ r ][ x ][ x ][ x ][ x ][ x ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Rook_BulkCalculateAvailableMovePosition_EmptyBoardBlackFromC1)
{
    Position board;
    auto r = BLACKROOK;
    board.PlacePiece(r, c1);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << a1.index();
    expected |= INT64_C(1) << b1.index();
    expected |= INT64_C(1) << d1.index();
    expected |= INT64_C(1) << e1.index();
    expected |= INT64_C(1) << f1.index();
    expected |= INT64_C(1) << g1.index();
    expected |= INT64_C(1) << h1.index();
    expected |= INT64_C(1) << c2.index();
    expected |= INT64_C(1) << c3.index();
    expected |= INT64_C(1) << c4.index();
    expected |= INT64_C(1) << c5.index();
    expected |= INT64_C(1) << c6.index();
    expected |= INT64_C(1) << c7.index();
    expected |= INT64_C(1) << c8.index();

    KingMask empty{};
    u64 result = board.calcAvailableMovesRookBulk<Set::BLACK>(empty).read();
    EXPECT_EQ(expected, result);
}

// 8 [ r ][ x ][ x ][ x ][ x ][ x ][ x ][ x ]
// 7 [ x ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ x ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ x ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ x ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ x ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ x ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ x ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Rook_BulkCalculateAvailableMovePosition_EmptyBoardBlackFromA8)
{
    Position board;
    auto r = BLACKROOK;
    board.PlacePiece(r, a8);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << b8.index();
    expected |= INT64_C(1) << c8.index();
    expected |= INT64_C(1) << d8.index();
    expected |= INT64_C(1) << e8.index();
    expected |= INT64_C(1) << f8.index();
    expected |= INT64_C(1) << g8.index();
    expected |= INT64_C(1) << h8.index();
    expected |= INT64_C(1) << a1.index();
    expected |= INT64_C(1) << a2.index();
    expected |= INT64_C(1) << a3.index();
    expected |= INT64_C(1) << a4.index();
    expected |= INT64_C(1) << a5.index();
    expected |= INT64_C(1) << a6.index();
    expected |= INT64_C(1) << a7.index();

    KingMask empty{};
    u64 result = board.calcAvailableMovesRookBulk<Set::BLACK>(empty).read();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ xN][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 3 [ x ][ x ][ r ][ x ][ x ][ xp][ . ][ . ]
// 2 [ . ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Rook_CalcThreat_BlockedBySomePieces)
{
    Position board;
    auto r = BLACKROOK;
    auto p = BLACKPAWN;
    auto N = WHITEKNIGHT;

    // setup
    board.PlacePiece(r, c3);
    board.PlacePiece(p, f3);
    board.PlacePiece(N, c5);

    u64 expected = ~universe;
    expected |= INT64_C(1) << a3.index();
    expected |= INT64_C(1) << b3.index();
    expected |= INT64_C(1) << d3.index();
    expected |= INT64_C(1) << e3.index();
    expected |= INT64_C(1) << f3.index();
    expected |= INT64_C(1) << c5.index();
    expected |= INT64_C(1) << c4.index();
    expected |= INT64_C(1) << c2.index();
    expected |= INT64_C(1) << c1.index();

    // do
    u64 result = board.calcThreatenedSquaresRookBulk<Set::BLACK>().read();
    // validate
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ r ][ . ][ x ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ R ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Rook_KingMask_OnlyAvailableMoveIsToBlockCheck)
{
    Position pos;
    auto r = BLACKROOK;
    auto k = BLACKKING;
    auto R = WHITEROOK;

    // setup
    pos.PlacePiece(r, c3);
    pos.PlacePiece(k, e7);
    pos.PlacePiece(R, e2);

    // do
    KingMask km = pos.calcKingMask<Set::BLACK>();
    Bitboard rookMoves = pos.calcAvailableMovesRookBulk<Set::BLACK>(km);

    // validate
    u64 expected = ~universe;
    expected |= INT64_C(1) << e3.index();
    EXPECT_EQ(expected, rookMoves.read());
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ r ][ . ][ xR][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Rook_KingMask_CanCaptureCheckingPiece)
{
    Position pos;
    auto r = BLACKROOK;
    auto k = BLACKKING;
    auto R = WHITEROOK;

    // setup
    pos.PlacePiece(r, c2);
    pos.PlacePiece(k, e7);
    pos.PlacePiece(R, e2);

    // do
    KingMask km = pos.calcKingMask<Set::BLACK>();
    Bitboard rookMoves = pos.calcAvailableMovesRookBulk<Set::BLACK>(km);

    // validate
    u64 expected = ~universe;
    expected |= INT64_C(1) << e2.index();
    EXPECT_EQ(expected, rookMoves.read());
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ x ][ . ][ r ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ R ][ . ][ . ][ . ]
// 1 [ . ][ . ][ r ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Rook_CheckedMask_HasNoMovesSinceKingIsInCheck)
{
    Position pos;
    auto r = BLACKROOK;
    auto k = BLACKKING;
    auto R = WHITEROOK;

    // setup
    pos.PlacePiece(r, c1);
    pos.PlacePiece(r, g4);
    pos.PlacePiece(k, e7);
    pos.PlacePiece(R, e2);
    constexpr Set black = Set::BLACK;
    // do
    KingMask kingMask = pos.calcKingMask<black>();
    Bitboard rookMoves = pos.calcAvailableMovesRookBulk<black>(kingMask);

    auto [nonattacks, attacks] = pos.isolatePiece<black, rookId>(c1, rookMoves, kingMask);

    // validate
    u64 expected = 0x0;
    EXPECT_EQ(expected, nonattacks.read());
    EXPECT_EQ(expected, attacks.read());
}

TEST_F(PositionFixture, Rook_IsolatingPiece_OnlyOneRookLeftInMask)
{
    Position board;
    board.PlacePiece(BLACKROOK, g7);
    board.PlacePiece(BLACKROOK, d2);

    u64 expected = 0x40bf404040404040ull;
    KingMask empty{};
    u64 movesbb = board.calcAvailableMovesRookBulk<Set::BLACK>(empty).read();

    auto [moves, attks] = board.isolatePiece<Set::BLACK, rookId>(g7, movesbb, empty);
    EXPECT_EQ(expected, moves.read());
}

TEST_F(PositionFixture, Rook_IsolatingPiece_RooksAreOnSameRank)
{
    Position board;
    board.PlacePiece(BLACKROOK, g7);
    board.PlacePiece(BLACKROOK, d7);

    KingMask empty{};
    u64 movesbb = board.calcAvailableMovesRookBulk<Set::BLACK>(empty).read();
    {
        u64 expected = 0x40b0404040404040;
        auto [moves, attks] = board.isolatePiece<Set::BLACK, rookId>(g7, movesbb, empty);
        EXPECT_EQ(expected, moves.read());
    }
    {
        u64 expected = 0x837080808080808;
        auto [moves, attks] = board.isolatePiece<Set::BLACK, rookId>(d7, movesbb, empty);
        EXPECT_EQ(expected, moves.read());
    }
}

TEST_F(PositionFixture, Rook_IsolatingPiece_RooksAreOnSameFile)
{
    Position board;
    board.PlacePiece(BLACKROOK, d4);
    board.PlacePiece(BLACKROOK, d7);

    u64 expected = 0x8f70808f7080808ull;
    KingMask empty{};
    u64 movesbb = board.calcAvailableMovesRookBulk<Set::BLACK>(empty).read();
    EXPECT_EQ(expected, movesbb);

    {
        expected = 0x808f7080808ull;
        auto [moves, attk] = board.isolatePiece<Set::BLACK, rookId>(d4, movesbb, empty);
        EXPECT_EQ(expected, moves.read());
    }
    {
        expected = 0x8f7080800000000ull;
        auto [moves, attk] = board.isolatePiece<Set::BLACK, rookId>(d7, movesbb, empty);
        EXPECT_EQ(expected, moves.read());
    }
}

// 8 [ r ][ xn][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ xp][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Rook_ThreatenMask_FromStartingPos)
{
    Position board;
    auto r = BLACKROOK;
    auto p = BLACKPAWN;
    auto n = BLACKKNIGHT;

    // setup
    board.PlacePiece(r, a8);
    board.PlacePiece(p, a7);
    board.PlacePiece(n, b8);

    u64 expected = ~universe;
    expected |= INT64_C(1) << a7.index();
    expected |= INT64_C(1) << b8.index();

    // do
    u64 threat = board.calcThreatenedSquaresRookBulk<Set::BLACK>().read();

    // validate
    EXPECT_EQ(expected, threat);
}

// 8 [ x ][ x ][ x ][ . ][ x ][ . ][ . ][ . ]
// 7 [ x ][ Q ][ x ][ x ][ x ][ x ][ x ][ x ]
// 6 [ x ][ x ][ x ][ . ][ x ][ . ][ . ][ . ]
// 5 [ . ][ x ][ . ][ x ][ x ][ . ][ . ][ x ]
// 4 [ . ][ x ][ x ][ . ][ x ][ . ][ x ][ . ]
// 3 [ . ][ x ][ . ][ x ][ x ][ x ][ . ][ . ]
// 2 [ x ][ x ][ x ][ x ][ Q ][ x ][ x ][ x ]
// 1 [ . ][ x ][ . ][ x ][ x ][ x ][ . ][ x ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Queen_BulkCalculateAvailableMovePosition_TwoWhiteQueensOnEmptyBoard)
{
    Position board;
    auto Q = WHITEQUEEN;

    board.PlacePiece(Q, b7);
    board.PlacePiece(Q, e2);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << a8.index();
    expected |= INT64_C(1) << a7.index();
    expected |= INT64_C(1) << a6.index();
    expected |= INT64_C(1) << a2.index();

    expected |= INT64_C(1) << b8.index();
    expected |= INT64_C(1) << b6.index();
    expected |= INT64_C(1) << b5.index();
    expected |= INT64_C(1) << b4.index();
    expected |= INT64_C(1) << b3.index();
    expected |= INT64_C(1) << b2.index();
    expected |= INT64_C(1) << b1.index();

    expected |= INT64_C(1) << c8.index();
    expected |= INT64_C(1) << c7.index();
    expected |= INT64_C(1) << c6.index();
    expected |= INT64_C(1) << c4.index();
    expected |= INT64_C(1) << c2.index();

    expected |= INT64_C(1) << d7.index();
    expected |= INT64_C(1) << d5.index();
    expected |= INT64_C(1) << d3.index();
    expected |= INT64_C(1) << d2.index();
    expected |= INT64_C(1) << d1.index();

    expected |= INT64_C(1) << e8.index();
    expected |= INT64_C(1) << e7.index();
    expected |= INT64_C(1) << e6.index();
    expected |= INT64_C(1) << e5.index();
    expected |= INT64_C(1) << e4.index();
    expected |= INT64_C(1) << e3.index();
    expected |= INT64_C(1) << e1.index();

    expected |= INT64_C(1) << f7.index();
    expected |= INT64_C(1) << f3.index();
    expected |= INT64_C(1) << f2.index();
    expected |= INT64_C(1) << f1.index();

    expected |= INT64_C(1) << g7.index();
    expected |= INT64_C(1) << g4.index();
    expected |= INT64_C(1) << g2.index();

    expected |= INT64_C(1) << h7.index();
    expected |= INT64_C(1) << h5.index();
    expected |= INT64_C(1) << h2.index();
    expected |= INT64_C(1) << h1.index();

    KingMask empty{};
    u64 result = board.calcAvailableMovesQueenBulk<Set::WHITE>(empty).read();
    EXPECT_EQ(expected, result);
}

// 8 [ x ][ x ][ x ][ . ][ x ][ . ][ . ][ . ]
// 7 [ x ][ Q ][ x ][ x ][ x ][ x ][ x ][ x ]
// 6 [ x ][ x ][ x ][ . ][ x ][ . ][ . ][ . ]
// 5 [ . ][ x ][ . ][ x ][ x ][ . ][ . ][ x ]
// 4 [ . ][ x ][ x ][ . ][ x ][ . ][ x ][ . ]
// 3 [ . ][ x ][ . ][ x ][ x ][ x ][ . ][ . ]
// 2 [ x ][ x ][ x ][ x ][ Q ][ x ][ xn][ . ]
// 1 [ . ][ x ][ . ][ x ][ x ][ x ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Queen_BulkCalculateAvailableMovePosition_TwoQueensCanCaptureKnight)
{
    Position board;
    auto Q = WHITEQUEEN;
    auto n = BLACKKNIGHT;

    board.PlacePiece(Q, b7);
    board.PlacePiece(Q, e2);
    board.PlacePiece(n, g2);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << a8.index();
    expected |= INT64_C(1) << a7.index();
    expected |= INT64_C(1) << a6.index();
    expected |= INT64_C(1) << a2.index();

    expected |= INT64_C(1) << b8.index();
    expected |= INT64_C(1) << b6.index();
    expected |= INT64_C(1) << b5.index();
    expected |= INT64_C(1) << b4.index();
    expected |= INT64_C(1) << b3.index();
    expected |= INT64_C(1) << b2.index();
    expected |= INT64_C(1) << b1.index();

    expected |= INT64_C(1) << c8.index();
    expected |= INT64_C(1) << c7.index();
    expected |= INT64_C(1) << c6.index();
    expected |= INT64_C(1) << c4.index();
    expected |= INT64_C(1) << c2.index();

    expected |= INT64_C(1) << d7.index();
    expected |= INT64_C(1) << d5.index();
    expected |= INT64_C(1) << d3.index();
    expected |= INT64_C(1) << d2.index();
    expected |= INT64_C(1) << d1.index();

    expected |= INT64_C(1) << e8.index();
    expected |= INT64_C(1) << e7.index();
    expected |= INT64_C(1) << e6.index();
    expected |= INT64_C(1) << e5.index();
    expected |= INT64_C(1) << e4.index();
    expected |= INT64_C(1) << e3.index();
    expected |= INT64_C(1) << e1.index();

    expected |= INT64_C(1) << f7.index();
    expected |= INT64_C(1) << f3.index();
    expected |= INT64_C(1) << f2.index();
    expected |= INT64_C(1) << f1.index();

    expected |= INT64_C(1) << g7.index();
    expected |= INT64_C(1) << g4.index();
    expected |= INT64_C(1) << g2.index();

    expected |= INT64_C(1) << h7.index();
    expected |= INT64_C(1) << h5.index();

    KingMask empty{};
    u64 result = board.calcAvailableMovesQueenBulk<Set::WHITE>(empty).read();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ x ][ . ][ . ][ x ][ . ][ . ]
// 7 [ x ][ . ][ x ][ . ][ x ][ . ][ . ][ . ]
// 6 [ . ][ x ][ x ][ x ][ . ][ . ][ . ][ . ]
// 5 [ x ][ x ][ q ][ x ][ x ][ x ][ x ][ x ]
// 4 [ . ][ x ][ x ][ x ][ . ][ . ][ . ][ . ]
// 3 [ x ][ . ][ x ][ . ][ xB][ . ][ . ][ . ]
// 2 [ . ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Queen_BulkCalc_BlackQueenCaptureAndMoves)
{
    Position board;
    auto q = BLACKQUEEN;
    auto B = WHITEBISHOP;

    board.PlacePiece(B, e3);
    board.PlacePiece(q, c5);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << a7.index();
    expected |= INT64_C(1) << a3.index();
    expected |= INT64_C(1) << b6.index();
    expected |= INT64_C(1) << b4.index();
    expected |= INT64_C(1) << d6.index();
    expected |= INT64_C(1) << d4.index();
    expected |= INT64_C(1) << e7.index();
    expected |= INT64_C(1) << e3.index();
    expected |= INT64_C(1) << f8.index();

    expected |= INT64_C(1) << a5.index();
    expected |= INT64_C(1) << b5.index();
    expected |= INT64_C(1) << d5.index();
    expected |= INT64_C(1) << e5.index();
    expected |= INT64_C(1) << f5.index();
    expected |= INT64_C(1) << g5.index();
    expected |= INT64_C(1) << h5.index();

    expected |= INT64_C(1) << c8.index();
    expected |= INT64_C(1) << c7.index();
    expected |= INT64_C(1) << c6.index();
    expected |= INT64_C(1) << c4.index();
    expected |= INT64_C(1) << c3.index();
    expected |= INT64_C(1) << c2.index();
    expected |= INT64_C(1) << c1.index();
    KingMask empty{};
    u64 result = board.calcAvailableMovesQueenBulk<Set::BLACK>(empty).read();
    EXPECT_EQ(expected, result);
}

TEST_F(PositionFixture, Queen_CalcThreaten_BlockedByPawns)
{
    Position board;
    auto Q = WHITEQUEEN;
    auto P = WHITEPAWN;

    // setup
    board.PlacePiece(Q, d1);
    board.PlacePiece(P, c2);
    board.PlacePiece(P, d2);
    board.PlacePiece(P, e2);

    u64 expected = ~universe;
    expected |= INT64_C(1) << c2.index();
    expected |= INT64_C(1) << d2.index();
    expected |= INT64_C(1) << e2.index();
    expected |= INT64_C(1) << a1.index();
    expected |= INT64_C(1) << b1.index();
    expected |= INT64_C(1) << c1.index();
    expected |= INT64_C(1) << e1.index();
    expected |= INT64_C(1) << f1.index();
    expected |= INT64_C(1) << g1.index();
    expected |= INT64_C(1) << h1.index();

    // do
    u64 threat = board.calcThreatenedSquaresQueenBulk<Set::WHITE>().read();

    // validate
    EXPECT_EQ(expected, threat);
}

// 8 [ k ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ xP][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, KingMask_Pawns)
{
    // as of november 2023 I don't know what the point of this code is.
    Position board;
    auto k = BLACKKING;
    auto P = WHITEPAWN;

    // setup
    board.PlacePiece(k, a8);
    board.PlacePiece(P, b7);

    u64 expected = ~universe;
    expected |= INT64_C(1) << b7.index();

    // do
    u64 kingMask = CombineKingMask(board.calcKingMask(k, a8, {0, 0}));

    // validate
    EXPECT_EQ(expected, kingMask);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ b ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ q ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ P ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ K ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, KingMask_BishopAndQueenNotThreatening)
{
    Position board;

    // setup
    board.PlacePiece(BLACKBISHOP, g7);
    board.PlacePiece(BLACKQUEEN, f3);
    board.PlacePiece(WHITEPAWN, g2);
    board.PlacePiece(WHITEKING, g1);

    // expected to be empty
    u64 expected = ~universe;

    // build sliding mask
    constexpr bool includeMaterial = true;
    u64 diagonal = board.calcThreatenedSquaresDiagonal<Set::BLACK, includeMaterial>().read();
    u64 orthogonal = board.calcThreatenedSquaresOrthogonal<Set::BLACK, includeMaterial>().read();

    // do
    u64 result = CombineKingMask(board.calcKingMask(WHITEKING, g1, {orthogonal, diagonal}));

    // validate
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ r ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ K ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, KingMask_Threatened_KingisPierced)
{
    Position board;

    // setup
    board.PlacePiece(BLACKROOK, g7);
    board.PlacePiece(WHITEKING, g2);

    u64 orthogonal = ~universe;
    constexpr bool includeMaterial = true;
    constexpr bool kingisPierced = true;
    orthogonal |= board.calcThreatenedSquares<Set::BLACK, includeMaterial, kingisPierced>().read();

    u64 expected = ~universe;
    expected |= INT64_C(1) << a7.index();
    expected |= INT64_C(1) << b7.index();
    expected |= INT64_C(1) << c7.index();
    expected |= INT64_C(1) << d7.index();
    expected |= INT64_C(1) << e7.index();
    expected |= INT64_C(1) << f7.index();
    expected |= INT64_C(1) << g7.index();
    expected |= INT64_C(1) << h7.index();
    expected |= INT64_C(1) << g8.index();
    expected |= INT64_C(1) << g6.index();
    expected |= INT64_C(1) << g5.index();
    expected |= INT64_C(1) << g4.index();
    expected |= INT64_C(1) << g3.index();
    expected |= INT64_C(1) << g2.index();
    expected |= INT64_C(1) << g1.index();

    // validate
    EXPECT_EQ(expected, orthogonal);
}

}  // namespace ElephantTest