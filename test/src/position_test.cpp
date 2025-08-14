#include <position/position.hpp>
#include <gtest/gtest.h>
#include <array>
#include <material/chess_piece.hpp>
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
// 6 [ . ][ . ][ xq][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ K ][ x ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, King_KingCanCapture_CaptureQueenShouldBeAvailableButCantMoveIntoQueensThreat)
{
    Position pos;
    auto K = WHITEKING;
    auto q = BLACKQUEEN;

    pos.PlacePiece(K, d5.toSquare());
    pos.PlacePiece(q, c6.toSquare());

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << c6.index();  // capture queen
    expected |= INT64_C(1) << d4.index();  // move out of check
    expected |= INT64_C(1) << e5.index();  // move out of check

    u64 result = pos.calcAvailableMovesKing<Set::WHITE>(0).read();
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
    board.PlacePiece(k, e8.toSquare());
    board.PlacePiece(r, h8.toSquare());
    board.PlacePiece(r, a8.toSquare());

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
    board.PlacePiece(k, e8.toSquare());
    board.PlacePiece(r, h8.toSquare());
    board.PlacePiece(r, a8.toSquare());

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
    board.PlacePiece(K, e1.toSquare());
    board.PlacePiece(R, h1.toSquare());
    board.PlacePiece(R, a1.toSquare());

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
    board.PlacePiece(K, e1.toSquare());
    board.PlacePiece(R, h1.toSquare());
    board.PlacePiece(R, a1.toSquare());
    board.PlacePiece(Q, d1.toSquare());

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
    pos.PlacePiece(BLACKROOK, a8.toSquare());
    pos.PlacePiece(WHITEBISHOP, b8.toSquare());
    pos.PlacePiece(BLACKKING, e8.toSquare());

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
    pos.PlacePiece(BLACKROOK, a8.toSquare());
    pos.PlacePiece(BLACKKNIGHT, b8.toSquare());
    pos.PlacePiece(BLACKKING, e8.toSquare());

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

// 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ B ][ . ][ R ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ N ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ K ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, King_Capture_CantCaptureGuardedPieceByKnight)
{
    Position pos;

    // setup
    pos.PlacePiece(BLACKKING, e8.toSquare());
    pos.PlacePiece(WHITEROOK, f7.toSquare());
    pos.PlacePiece(WHITEKNIGHT, h6.toSquare());
    pos.PlacePiece(WHITEBISHOP, d7.toSquare());

    byte castling = 0;

    u64 expected = ~universe;
    expected |= INT64_C(1) << d8.index();

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
    pos.PlacePiece(P, a3.toSquare());

    pos.PlacePiece(BLACKKNIGHT, b4.toSquare());

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << a4.index();
    expected |= INT64_C(1) << b4.index();

    KingPinThreats empty{};
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
    pos.PlacePiece(p, b3.toSquare());

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << b2.index();

    KingPinThreats empty{};
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
    pos.PlacePiece(P, b2.toSquare());

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << b4.index();
    expected |= INT64_C(1) << b3.index();

    KingPinThreats empty{};
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
    board.PlacePiece(P, c2.toSquare());
    board.PlacePiece(N, c3.toSquare());

    // setup
    u64 expected = 0x0;

    KingPinThreats empty{};
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
    board.PlacePiece(p, e7.toSquare());
    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << e6.index();
    expected |= INT64_C(1) << e5.index();

    KingPinThreats empty{};
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

    board.PlacePiece(p, b2.toSquare());
    board.PlacePiece(p, d6.toSquare());
    board.PlacePiece(p, e7.toSquare());
    board.PlacePiece(p, g7.toSquare());

    u64 expected = ~universe;
    expected |= INT64_C(1) << b1.index();
    expected |= INT64_C(1) << d5.index();
    expected |= INT64_C(1) << e6.index();
    expected |= INT64_C(1) << e5.index();
    expected |= INT64_C(1) << g6.index();
    expected |= INT64_C(1) << g5.index();

    // do
    KingPinThreats empty{};
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

    board.PlacePiece(p, b2.toSquare());
    board.PlacePiece(R, b1.toSquare());

    board.PlacePiece(p, c6.toSquare());
    board.PlacePiece(B, c5.toSquare());

    board.PlacePiece(p, d6.toSquare());
    board.PlacePiece(n, d4.toSquare());

    board.PlacePiece(p, e7.toSquare());
    board.PlacePiece(p, e5.toSquare());
    board.PlacePiece(p, g7.toSquare());
    board.PlacePiece(n, g6.toSquare());

    u64 expected = ~universe;
    expected |= INT64_C(1) << d5.index();
    expected |= INT64_C(1) << c5.index();  // available capture
    expected |= INT64_C(1) << e6.index();
    expected |= INT64_C(1) << e4.index();

    // do
    KingPinThreats empty{};
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

    board.PlacePiece(P, b2.toSquare());
    board.PlacePiece(P, c3.toSquare());
    board.PlacePiece(P, d5.toSquare());
    board.PlacePiece(p, d6.toSquare());
    board.PlacePiece(P, e7.toSquare());
    board.PlacePiece(P, f2.toSquare());
    board.PlacePiece(n, f3.toSquare());
    board.PlacePiece(P, g2.toSquare());
    board.PlacePiece(B, g4.toSquare());
    board.PlacePiece(P, h5.toSquare());

    u64 expected = ~universe;
    expected |= INT64_C(1) << b3.index();
    expected |= INT64_C(1) << b4.index();
    expected |= INT64_C(1) << c4.index();
    expected |= INT64_C(1) << e8.index();
    expected |= INT64_C(1) << f3.index();
    expected |= INT64_C(1) << g3.index();
    expected |= INT64_C(1) << h6.index();

    // do
    KingPinThreats empty{};
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

    board.PlacePiece(P, a3.toSquare());
    board.PlacePiece(P, b5.toSquare());
    board.PlacePiece(P, c2.toSquare());
    board.PlacePiece(P, h2.toSquare());

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

    board.PlacePiece(p, a3.toSquare());
    board.PlacePiece(p, b7.toSquare());
    board.PlacePiece(p, d4.toSquare());
    board.PlacePiece(p, h5.toSquare());

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

    board.PlacePiece(P, b5.toSquare());
    board.PlacePiece(P, c2.toSquare());
    board.PlacePiece(P, d3.toSquare());
    board.PlacePiece(P, h2.toSquare());

    board.PlacePiece(p, a6.toSquare());
    board.PlacePiece(q, a3.toSquare());
    board.PlacePiece(n, c6.toSquare());
    board.PlacePiece(n, e4.toSquare());

    u64 expected = ~universe;
    expected |= INT64_C(1) << a6.index();
    expected |= INT64_C(1) << c6.index();
    expected |= INT64_C(1) << e4.index();

    // do
    KingPinThreats empty{};
    u64 result = board.calcThreatenedSquaresPawnBulk<Set::WHITE>().read();
    Bitboard opMaterial = board.readMaterial().combine<Set::BLACK>();

    result = (opMaterial & result).read();
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

    board.PlacePiece(P, b3.toSquare());
    board.PlacePiece(p, b6.toSquare());
    board.PlacePiece(B, a5.toSquare());

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
    pos.PlacePiece(P, g4.toSquare());
    pos.PlacePiece(p, f4.toSquare());
    pos.editEnPassant().writeSquare(g3.toSquare());
    //  do
    KingPinThreats empty{};
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
    pos.PlacePiece(P, g5.toSquare());
    pos.PlacePiece(p, f5.toSquare());
    pos.editEnPassant().writeSquare(f6.toSquare());

    // do
    KingPinThreats empty{};
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
    auto K = WHITEKING;

    // setup
    pos.PlacePiece(p, f7.toSquare());
    pos.PlacePiece(k, e7.toSquare());
    pos.PlacePiece(R, h7.toSquare());
    pos.PlacePiece(K, e1.toSquare());

    // do
    auto kingMask = pos.calcKingMask<Set::BLACK>();
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
    auto k = BLACKKING;

    // setup
    pos.PlacePiece(P, f5.toSquare());
    pos.PlacePiece(P, c5.toSquare());
    pos.PlacePiece(K, e5.toSquare());
    pos.PlacePiece(r, h5.toSquare());
    pos.PlacePiece(k, e7.toSquare());

    // do
    KingPinThreats kingMask = pos.calcKingMask<Set::WHITE>();
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
    auto k = BLACKKING;

    // setup
    pos.PlacePiece(P, f6.toSquare());
    pos.PlacePiece(P, c5.toSquare());
    pos.PlacePiece(K, e5.toSquare());
    pos.PlacePiece(b, g7.toSquare());
    pos.PlacePiece(k, e7.toSquare());

    // do
    KingPinThreats kingMask = pos.calcKingMask<Set::WHITE>();
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
    auto K = WHITEKING;

    // setup
    pos.PlacePiece(p, f7.toSquare());
    pos.PlacePiece(k, e6.toSquare());
    pos.PlacePiece(R, h6.toSquare());
    pos.PlacePiece(K, e1.toSquare());

    // do
    KingPinThreats kingMask = pos.calcKingMask<Set::BLACK>();
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
    auto K = WHITEKING;

    // setup
    pos.PlacePiece(p, f7.toSquare());
    pos.PlacePiece(k, e6.toSquare());
    pos.PlacePiece(R, g6.toSquare());
    pos.PlacePiece(K, e1.toSquare());

    // do
    KingPinThreats kingMask = pos.calcKingMask<Set::BLACK>();
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
    auto K = WHITEKING;

    // setup
    pos.PlacePiece(p, f7.toSquare());
    pos.PlacePiece(k, e5.toSquare());
    pos.PlacePiece(R, g5.toSquare());
    pos.PlacePiece(K, e1.toSquare());

    // do
    KingPinThreats kingMask = pos.calcKingMask<Set::BLACK>();
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
    board.PlacePiece(WHITEPAWN, d4.toSquare());
    board.PlacePiece(WHITEPAWN, c2.toSquare());

    u64 expected = 0x804040000;
    KingPinThreats empty{};
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
    board.PlacePiece(WHITEPAWN, d4.toSquare());
    board.PlacePiece(WHITEPAWN, e4.toSquare());

    u64 expected = 0x1800000000;
    KingPinThreats empty{};
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
    board.PlacePiece(WHITEPAWN, d4.toSquare());
    board.PlacePiece(WHITEPAWN, d2.toSquare());

    u64 expected = 0x800080000;
    KingPinThreats empty{};
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
    pos.PlacePiece(P, g4.toSquare());
    pos.PlacePiece(p, f4.toSquare());
    pos.PlacePiece(p, h4.toSquare());
    pos.editEnPassant().writeSquare(g3.toSquare());
    KingPinThreats empty{};
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
    board.PlacePiece(N, b2.toSquare());

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << a4.index();
    expected |= INT64_C(1) << c4.index();
    expected |= INT64_C(1) << d3.index();
    expected |= INT64_C(1) << d1.index();

    KingPinThreats empty{};
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
    pos.PlacePiece(N, c3.toSquare());

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
    KingPinThreats empty{};
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
    pos.PlacePiece(N, b5.toSquare());

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << a7.index();
    expected |= INT64_C(1) << a3.index();
    expected |= INT64_C(1) << c7.index();
    expected |= INT64_C(1) << c3.index();
    expected |= INT64_C(1) << d6.index();
    expected |= INT64_C(1) << d4.index();

    KingPinThreats empty{};
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

    board.PlacePiece(N, e3.toSquare());
    board.PlacePiece(B, f5.toSquare());
    board.PlacePiece(b, d5.toSquare());

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << d5.index();

    // as of November 2023 I don't see a reason to supply a attacked function
    u64 result = board.calcThreatenedSquaresKnightBulk<Set::WHITE>().read();
    u64 opMat = board.readMaterial().combine<Set::BLACK>().read();
    result &= opMat;
    EXPECT_EQ(expected, result);
}

TEST_F(PositionFixture, Knight_IsolatingPiece_TwoKnightsNotSharingSquaresButBlockingEachOther)
{
    Position board;
    board.PlacePiece(WHITEKNIGHT, d4.toSquare());
    board.PlacePiece(WHITEKNIGHT, e2.toSquare());

    u64 expected = 0x142220660444ull;
    KingPinThreats empty{};
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

TEST_F(PositionFixture, Knight_IsolatingPiece_SharingTargetSquares)
{
    Position board;
    board.PlacePiece(WHITEKNIGHT, d4.toSquare());
    board.PlacePiece(WHITEKNIGHT, f4.toSquare());

    u64 expected = 0x54aa00aa5400ull;
    KingPinThreats empty{};
    u64 movesbb = board.calcAvailableMovesKnightBulk<Set::WHITE>(empty).read();
    EXPECT_EQ(expected, movesbb);

    {
        expected = 0x142200221400ull;
        auto [moves, attks] = board.isolatePiece<Set::WHITE, knightId>(d4, movesbb, empty);
        EXPECT_EQ(expected, moves.read());
    }

    expected = 0x508800885000ull;
    auto [moves, attks] = board.isolatePiece<Set::WHITE, knightId>(f4, movesbb, empty);
    EXPECT_EQ(expected, moves.read());
}

TEST_F(PositionFixture, Knight_IsolatingPiece_SharingTargetSquaresOnEdge)
{
    Position board;
    board.PlacePiece(WHITEKNIGHT, g2.toSquare());
    board.PlacePiece(WHITEKNIGHT, g4.toSquare());

    u64 expected = 0xa010a010a010ull;
    KingPinThreats empty{};
    u64 movesbb = board.calcAvailableMovesKnightBulk<Set::WHITE>(empty).read();
    EXPECT_EQ(expected, movesbb);

    {
        expected = 0xa0100010ull;
        auto [moves, attks] = board.isolatePiece<Set::WHITE, knightId>(g2, movesbb, empty);
        EXPECT_EQ(expected, moves.read());
    }

    expected = 0xa0100010a000ull;
    auto [moves, attks] = board.isolatePiece<Set::WHITE, knightId>(g4, movesbb, empty);
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
    board.PlacePiece(b, f6.toSquare());
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

    KingPinThreats empty{};
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

    board.PlacePiece(b, e3.toSquare());
    board.PlacePiece(B, c5.toSquare());

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
    KingPinThreats empty{};
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

    board.PlacePiece(b, b7.toSquare());

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
    KingPinThreats empty{};
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

    board.PlacePiece(B, b7.toSquare());
    board.PlacePiece(B, e2.toSquare());

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
    KingPinThreats empty{};
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
    board.PlacePiece(BLACKBISHOP, g7.toSquare());
    board.PlacePiece(BLACKBISHOP, d2.toSquare());

    u64 expected = 0xa000a0512a140215ull;
    KingPinThreats empty{};
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

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ x ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ x ][ . ][ . ]
// 6 [ x ][ . ][ . ][ . ][ x ][ . ][ . ][ . ]
// 5 [ . ][ x ][ . ][ x ][ . ][ . ][ . ][ x ]
// 4 [ . ][ . ][ b ][ . ][ . ][ . ][ x ][ . ]
// 3 [ . ][ x ][ . ][ x ][ . ][ x ][ . ][ . ]
// 2 [ x ][ . ][ . ][ . ][ b ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ x ][ . ][ x ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Bishop_IsolatingPiece_BishopsOnSameDiagonal)
{
    Position board;
    board.PlacePiece(BLACKBISHOP, c4.toSquare());
    board.PlacePiece(BLACKBISHOP, e2.toSquare());

    u64 expected = 0x4020118a402a0128ull;
    KingPinThreats empty{};
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
    pos.PlacePiece(WHITEKING, e1.toSquare());
    pos.PlacePiece(WHITEROOK, a8.toSquare());
    pos.PlacePiece(BLACKBISHOP, b7.toSquare());
    pos.PlacePiece(BLACKKING, e8.toSquare());

    // do
    KingPinThreats kingMask = pos.calcKingMask<Set::BLACK>();
    Bitboard bishopMoves = pos.calcAvailableMovesBishopBulk<Set::BLACK>(kingMask);
    {
        auto [nonattacks, attacks] = pos.isolatePiece<Set::BLACK, bishopId>(b7, bishopMoves, kingMask);

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
    auto K = WHITEKING;

    // setup
    pos.PlacePiece(b, c4.toSquare());
    pos.PlacePiece(k, f7.toSquare());
    pos.PlacePiece(b, f3.toSquare());
    pos.PlacePiece(B, a2.toSquare());
    pos.PlacePiece(K, e1.toSquare());

    // do
    KingPinThreats kingMask = pos.calcKingMask<Set::BLACK>();
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

/**
 * 8 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
 * 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
 * 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ b ]
 * 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
 * 4 [ . ][ . ][ . ][ . ][ b ][ . ][ . ][ . ]
 * 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
 * 2 [ . ][ . ][ . ][ . ][ Q ][ . ][ . ][ . ]
 * 1 [ . ][ . ][ . ][ . ][ . ][ . ][ K ][ . ]
 *     A    B    C    D    E    F    G    H
 * @brief Ran into a scenario where the engine thought it could move bisop on E4 to E3 */
TEST_F(PositionFixture, Bishop_IsolatePinnedPiece_E4BishopHasNoMoves)
{
    Position pos;
    auto b = BLACKBISHOP;
    auto k = BLACKKING;
    auto Q = WHITEQUEEN;
    auto K = WHITEKING;

    // setup
    pos.PlacePiece(k, e8.toSquare());
    pos.PlacePiece(b, h6.toSquare());
    pos.PlacePiece(b, e4.toSquare());
    pos.PlacePiece(K, g1.toSquare());
    pos.PlacePiece(Q, e2.toSquare());

    // do
    KingPinThreats kingMask = pos.calcKingMask<Set::BLACK>();
    Bitboard bishopMoves = pos.calcAvailableMovesBishopBulk<Set::BLACK>(kingMask);
    u64 expected = 0x21c2446820384c86ull;  // we don't calculate pins at this point
    EXPECT_EQ(expected, bishopMoves.read());
    {
        auto [nonattacks, attacks] = pos.isolatePiece<Set::BLACK, bishopId>(h6, bishopMoves, kingMask);

        // validate
        expected = 0x2040004020100804ull;
        EXPECT_EQ(expected, nonattacks.read());
        expected = 0;
        EXPECT_EQ(expected, attacks.read());
    }

    auto [nonattacks, attacks] = pos.isolatePiece<Set::BLACK, bishopId>(e4, bishopMoves, kingMask);

    // validate
    expected = 0;
    EXPECT_EQ(expected, nonattacks.read());
    expected = 0;
    EXPECT_EQ(expected, attacks.read());
}

/**
 * 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
 * 7 [ . ][ . ][ . ][ . ][ . ][ k ][ . ][ . ]
 * 6 [ . ][ . ][ . ][ . ][ x ][ . ][ . ][ . ]
 * 5 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ x ]
 * 4 [ . ][ . ][ b ][ . ][ n ][ . ][ x ][ . ]
 * 3 [ . ][ x ][ . ][ . ][ . ][ b ][ . ][ . ]
 * 2 [ B ][ . ][ . ][ . ][ x ][ . ][ x ][ . ]
 * 1 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ x ]
 *     A    B    C    D    E    F    G    H
 * @brief Even though the bishop is pinned, it is able to move along the threatened squares. */
TEST_F(PositionFixture, Bishop_IsolatePinnedPiece_BlockedDiagonalSquareNotShared)
{
    Position pos;
    auto b = BLACKBISHOP;
    auto k = BLACKKING;
    auto n = BLACKKNIGHT;
    auto B = WHITEBISHOP;
    auto K = WHITEKING;

    // setup
    pos.PlacePiece(b, c4.toSquare());
    pos.PlacePiece(k, f7.toSquare());
    pos.PlacePiece(b, f3.toSquare());
    pos.PlacePiece(n, e4.toSquare());
    pos.PlacePiece(B, a2.toSquare());
    pos.PlacePiece(K, e1.toSquare());

    // do
    KingPinThreats kingMask = pos.calcKingMask<Set::BLACK>();
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
    u64 expected = 0x8040005088ull;
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
    board.PlacePiece(R, d4.toSquare());

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

    board.PlacePiece(r, b7.toSquare());

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

    board.PlacePiece(r, b7.toSquare());
    board.PlacePiece(r, h1.toSquare());

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
    board.PlacePiece(r, b7.toSquare());
    board.PlacePiece(r, e7.toSquare());
    board.PlacePiece(n, g7.toSquare());

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

    KingPinThreats empty{};
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
    board.PlacePiece(r, c1.toSquare());

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

    KingPinThreats empty{};
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
    board.PlacePiece(r, a8.toSquare());

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

    KingPinThreats empty{};
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
    board.PlacePiece(r, c3.toSquare());
    board.PlacePiece(p, f3.toSquare());
    board.PlacePiece(N, c5.toSquare());

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
    auto K = WHITEKING;

    // setup
    pos.PlacePiece(r, c3.toSquare());
    pos.PlacePiece(k, e7.toSquare());
    pos.PlacePiece(R, e2.toSquare());
    pos.PlacePiece(K, e1.toSquare());

    // do
    KingPinThreats km = pos.calcKingMask<Set::BLACK>();
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
    auto K = WHITEKING;

    // setup
    pos.PlacePiece(r, c2.toSquare());
    pos.PlacePiece(k, e7.toSquare());
    pos.PlacePiece(R, e2.toSquare());
    pos.PlacePiece(K, e1.toSquare());

    // do
    KingPinThreats km = pos.calcKingMask<Set::BLACK>();
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
    auto K = WHITEKING;

    // setup
    pos.PlacePiece(r, c1.toSquare());
    pos.PlacePiece(r, g4.toSquare());
    pos.PlacePiece(k, e7.toSquare());
    pos.PlacePiece(R, e2.toSquare());
    pos.PlacePiece(K, d2.toSquare());
    constexpr Set black = Set::BLACK;
    // do
    KingPinThreats kingMask = pos.calcKingMask<black>();
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
    board.PlacePiece(BLACKROOK, g7.toSquare());
    board.PlacePiece(BLACKROOK, d2.toSquare());

    u64 expected = 0x40bf404040404040ull;
    KingPinThreats empty{};
    u64 movesbb = board.calcAvailableMovesRookBulk<Set::BLACK>(empty).read();

    auto [moves, attks] = board.isolatePiece<Set::BLACK, rookId>(g7, movesbb, empty);
    EXPECT_EQ(expected, moves.read());
}

TEST_F(PositionFixture, Rook_IsolatingPiece_RooksAreOnSameRank)
{
    Position board;
    board.PlacePiece(BLACKROOK, g7.toSquare());
    board.PlacePiece(BLACKROOK, d7.toSquare());

    KingPinThreats empty{};
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

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ q ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ r ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ R ][ . ][ . ][ . ][ K ][ . ][ . ][ R ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Rook_IsolatingPiece_RooksAreOnBaseRankWithOpPieces)
{
    Position board;
    board.PlacePiece(BLACKROOK, a2.toSquare());
    board.PlacePiece(BLACKQUEEN, h7.toSquare());
    board.PlacePiece(WHITEROOK, a1.toSquare());
    board.PlacePiece(WHITEKING, e1.toSquare());
    board.PlacePiece(WHITEROOK, h1.toSquare());

    KingPinThreats empty{};
    u64 movesbb = board.calcAvailableMovesRookBulk<Set::WHITE>(empty).read();
    {
        auto [moves, attks] = board.isolatePiece<Set::WHITE, rookId>(a1, movesbb, empty);

        u64 expected = 0xe;
        EXPECT_EQ(expected, moves.read());
        expected = 0x100;
        EXPECT_EQ(expected, attks.read());
    }
    {
        auto [moves, attks] = board.isolatePiece<Set::WHITE, rookId>(h1, movesbb, empty);
        u64 expected = 0x808080808060ull;
        EXPECT_EQ(expected, moves.read());
        expected = 0x80000000000000ull;
        EXPECT_EQ(expected, attks.read());
    }
}

// 8 [ r ][ . ][ . ][ . ][ . ][ . ][ . ][ r ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ R ][ . ][ . ][ b ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ R ]
//     A    B    C    D    E    F    G    H
TEST_F(PositionFixture, Rook_IsolatingPiece_RooksShareSquare)
{
    Position board;
    //    board.PlacePiece(BLACKROOK, a8.toSquare());
    //    board.PlacePiece(BLACKROOK, h8.toSquare());
    board.PlacePiece(BLACKBISHOP, d4.toSquare());

    board.PlacePiece(WHITEROOK, a4.toSquare());
    board.PlacePiece(WHITEROOK, h1.toSquare());

    KingPinThreats empty{};
    u64 movesbb = board.calcAvailableMovesRookBulk<Set::WHITE>(empty).read();
    {
        auto [moves, attks] = board.isolatePiece<Set::WHITE, rookId>(a4, movesbb, empty);

        u64 expected = 0x101010106010101ull;
        EXPECT_EQ(expected, moves.read());
        expected = 0x8000000;
        EXPECT_EQ(expected, attks.read());
    }
    {
        auto [moves, attks] = board.isolatePiece<Set::WHITE, rookId>(h1, movesbb, empty);
        u64 expected = 0x808080808080807full;
        EXPECT_EQ(expected, moves.read());
        expected = 0;
        EXPECT_EQ(expected, attks.read());
    }
}

TEST_F(PositionFixture, Rook_IsolatingPiece_RooksAreOnSameFile)
{
    Position board;
    board.PlacePiece(BLACKROOK, d4.toSquare());
    board.PlacePiece(BLACKROOK, d7.toSquare());

    u64 expected = 0x8f70808f7080808ull;
    KingPinThreats empty{};
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
    board.PlacePiece(r, a8.toSquare());
    board.PlacePiece(p, a7.toSquare());
    board.PlacePiece(n, b8.toSquare());

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

    board.PlacePiece(Q, b7.toSquare());
    board.PlacePiece(Q, e2.toSquare());

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

    KingPinThreats empty{};
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

    board.PlacePiece(Q, b7.toSquare());
    board.PlacePiece(Q, e2.toSquare());
    board.PlacePiece(n, g2.toSquare());

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

    KingPinThreats empty{};
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

    board.PlacePiece(B, e3.toSquare());
    board.PlacePiece(q, c5.toSquare());

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
    KingPinThreats empty{};
    u64 result = board.calcAvailableMovesQueenBulk<Set::BLACK>(empty).read();
    EXPECT_EQ(expected, result);
}

TEST_F(PositionFixture, Queen_CalcThreaten_BlockedByPawns)
{
    Position board;
    auto Q = WHITEQUEEN;
    auto P = WHITEPAWN;

    // setup
    board.PlacePiece(Q, d1.toSquare());
    board.PlacePiece(P, c2.toSquare());
    board.PlacePiece(P, d2.toSquare());
    board.PlacePiece(P, e2.toSquare());

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

/**
 * 8 [   ][   ][   ][   ][ k ][   ][   ][   ]
 * 7 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 6 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 5 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 4 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 3 [   ][ q ][   ][   ][   ][   ][   ][   ]
 * 2 [   ][   ][ Q ][   ][   ][ R ][   ][ K ]
 * 1 [ q ][   ][   ][   ][   ][   ][   ][   ]
 *     A    B    C    D    E    F    G    H */
TEST_F(PositionFixture, Queen_IsolateQueen_TwoQueensDiagonal)
{
    Position pos;
    auto K = WHITEKING;
    auto Q = WHITEQUEEN;
    auto R = WHITEROOK;

    auto k = BLACKKING;
    auto q = BLACKQUEEN;

    // setup
    pos.PlacePiece(K, h2.toSquare());
    pos.PlacePiece(Q, c2.toSquare());
    pos.PlacePiece(R, f2.toSquare());

    pos.PlacePiece(k, e8.toSquare());
    pos.PlacePiece(q, a1.toSquare());
    pos.PlacePiece(q, b3.toSquare());

    auto kingMask = pos.calcKingMask<Set::BLACK>();
    // do
    Bitboard queenMoves = pos.calcAvailableMovesQueenBulk<Set::BLACK>(kingMask);

    // validate
    u64 expected = 0xc363331b0ffd07feull;
    EXPECT_EQ(expected, queenMoves.read());

    {
        // do
        auto [nonattacks, attacks] = pos.isolatePiece<Set::BLACK, queenId>(a1, queenMoves, kingMask);

        // validate
        expected = 0x81412111090503feull;
        EXPECT_EQ(expected, nonattacks.read());
        expected = 0;
        EXPECT_EQ(expected, attacks.read());
    }

    {
        // do
        auto [nonattacks, attacks] = pos.isolatePiece<Set::BLACK, queenId>(b3, queenMoves, kingMask);

        // validate
        expected = 0x4222120a07fd0302ull;
        EXPECT_EQ(expected, nonattacks.read());
        expected = 1024;
        EXPECT_EQ(expected, attacks.read());
    }
}

/**
 * 8 [   ][   ][   ][   ][ k ][   ][   ][   ]
 * 7 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 6 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 5 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 4 [ q ][   ][   ][   ][   ][   ][   ][   ]
 * 3 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 2 [   ][   ][   ][   ][   ][ R ][   ][ K ]
 * 1 [ q ][ Q ][   ][   ][   ][   ][   ][   ]
 *     A    B    C    D    E    F    G    H */
TEST_F(PositionFixture, Queen_IsolateQueen_TwoQueensOrthogonal)
{
    Position pos;
    auto K = WHITEKING;
    auto Q = WHITEQUEEN;
    auto R = WHITEROOK;

    auto k = BLACKKING;
    auto q = BLACKQUEEN;

    // setup
    pos.PlacePiece(K, h2.toSquare());
    pos.PlacePiece(Q, b1.toSquare());
    pos.PlacePiece(R, f2.toSquare());

    pos.PlacePiece(k, e8.toSquare());
    pos.PlacePiece(q, a1.toSquare());
    pos.PlacePiece(q, a4.toSquare());

    auto kingMask = pos.calcKingMask<Set::BLACK>();
    // do
    Bitboard queenMoves = pos.calcAvailableMovesQueenBulk<Set::BLACK>(kingMask);

    // validate
    u64 expected = 0x8040201008050300ull | 0x1090503fe030508ull | 2;
    EXPECT_EQ(expected, queenMoves.read());

    {
        // do
        auto [nonattacks, attacks] = pos.isolatePiece<Set::BLACK, queenId>(a4, queenMoves, kingMask);

        // validate
        expected = 0x1090503fe030508ull;
        EXPECT_EQ(expected, nonattacks.read());
        expected = 0;
        EXPECT_EQ(expected, attacks.read());
    }

    {
        // do
        auto [nonattacks, attacks] = pos.isolatePiece<Set::BLACK, queenId>(a1, queenMoves, kingMask);

        // validate
        expected = 0x8040201008050300ull;
        EXPECT_EQ(expected, nonattacks.read());
        expected = 2;
        EXPECT_EQ(expected, attacks.read());
    }
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
    board.PlacePiece(k, a8.toSquare());
    board.PlacePiece(P, b7.toSquare());

    u64 expected = ~universe;
    expected |= INT64_C(1) << b7.index();

    // do
    auto kingMask = board.calcKingMask<Set::BLACK>().combined();
    //auto kingMask = board.calcKingMask(k, Square::A8, { 0, 0 }).combined();

    // validate
    EXPECT_EQ(expected, kingMask.read());
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
    board.PlacePiece(BLACKROOK, g7.toSquare());
    board.PlacePiece(WHITEKING, g2.toSquare());

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