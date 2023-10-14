#include "bitboard.h"
#include <gtest/gtest.h>
#include <array>
#include "chess_piece.h"
#include "elephant_test_utils.h"
#include "notation.h"

namespace ElephantTest {
////////////////////////////////////////////////////////////////

/**
 * @file checkmate_test.cpp
 * @brief Fixture for testing bitboard functionality.
 * Naming convention as of April 2023: <TestedFunctionality>_<TestedColor>_<ExpectedResult>
 * @author Alexander Loodin Ek
 */
class BitboardFixture : public ::testing::Test {
public:
};

////////////////////////////////////////////////////////////////

TEST_F(BitboardFixture, ValidSquare)
{
    Notation n(0);
    bool result = Bitboard::IsValidSquare(n);
    EXPECT_TRUE(result) << Notation::toString(n) << "\n";

    n = Notation(63);
    result = Bitboard::IsValidSquare(n);
    EXPECT_TRUE(result) << Notation::toString(n) << "\n";

    n = Notation(64);
    result = Bitboard::IsValidSquare(n);
    EXPECT_FALSE(result) << Notation::toString(n) << "\n";

    // when index is 128 our notation will actually be 0, 0. i.e. a1
    // it's wrapped around the board.
    n = Notation(128);
    result = Bitboard::IsValidSquare(n);
    EXPECT_TRUE(result) << Notation::toString(n) << "\n";

    for (byte i = 0; i < 64; ++i) {
        n = Notation(i);
        result = Bitboard::IsValidSquare(n);
        EXPECT_TRUE(result) << Notation::toString(n) << "\n";
    }

    for (byte i = 64; i < 0x80; ++i) {
        result = Bitboard::IsValidSquare(i);
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
TEST_F(BitboardFixture, King_Move_e1)
{
    Bitboard board;
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

    auto result = board.calcAvailableMoves(e1, K, 0, {});
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
TEST_F(BitboardFixture, King_Move_d4)
{
    Bitboard board;
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

    u64 result = board.calcAvailableMovesKingBulk<Set::WHITE>();
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
TEST_F(BitboardFixture, Black_King_Move_e8)
{
    Bitboard board;
    auto k = BLACKKING;
    board.PlacePiece(k, e8);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << d8.index();
    expected |= INT64_C(1) << d7.index();
    expected |= INT64_C(1) << e7.index();
    expected |= INT64_C(1) << f8.index();
    expected |= INT64_C(1) << f7.index();

    u64 result = board.calcAvailableMovesKingBulk<Set::BLACK>();
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ xq][ x ][ x ][ . ][ . ][ . ]
// 5 [ . ][ . ][ x ][ K ][ x ][ . ][ . ][ . ]
// 4 [ . ][ . ][ x ][ x ][ x ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BitboardFixture, Black_King_Attack_d5)
{
    Bitboard board;
    auto K = WHITEKING;
    auto q = BLACKQUEEN;

    board.PlacePiece(K, d5);
    board.PlacePiece(q, c6);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << c6.index();
    expected |= INT64_C(1) << c5.index();
    expected |= INT64_C(1) << c4.index();
    expected |= INT64_C(1) << d6.index();
    expected |= INT64_C(1) << d4.index();
    expected |= INT64_C(1) << e6.index();
    expected |= INT64_C(1) << e5.index();
    expected |= INT64_C(1) << e4.index();

    u64 result = board.calcAvailableMoves(d5, K, 0, {});
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
TEST_F(BitboardFixture, King_InEachCorner)
{
    // each corner but one corner at a time.
    Bitboard board;
    auto k = BLACKKING;
    board.PlacePiece(k, a1);

    // setup a1 corner
    u64 expected = ~universe;
    expected |= INT64_C(1) << a2.index();
    expected |= INT64_C(1) << b2.index();
    expected |= INT64_C(1) << b1.index();

    u64 result = board.calcAvailableMovesKingBulk<Set::BLACK>();
    EXPECT_EQ(expected, result);

    // setup a8 corner
    board.ClearPiece(k, a1);
    board.PlacePiece(k, a8);
    expected = ~universe;
    expected |= INT64_C(1) << b8.index();
    expected |= INT64_C(1) << b7.index();
    expected |= INT64_C(1) << a7.index();

    result = board.calcAvailableMovesKingBulk<Set::BLACK>();
    EXPECT_EQ(expected, result);

    // setup h8 corner
    board.ClearPiece(k, a8);
    board.PlacePiece(k, h8);
    expected = ~universe;
    expected |= INT64_C(1) << h7.index();
    expected |= INT64_C(1) << g8.index();
    expected |= INT64_C(1) << g7.index();

    result = board.calcAvailableMovesKingBulk<Set::BLACK>();
    EXPECT_EQ(expected, result);

    // setup h1 corner
    board.ClearPiece(k, h8);
    board.PlacePiece(k, h1);
    expected = ~universe;
    expected |= INT64_C(1) << h2.index();
    expected |= INT64_C(1) << g1.index();
    expected |= INT64_C(1) << g2.index();

    result = board.calcAvailableMovesKingBulk<Set::BLACK>();
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
TEST_F(BitboardFixture, Black_King_Moves_With_Rooks)
{
    Bitboard board;
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
    u64 result = board.calcAvailableMoves(e8, k, castling, {});
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
TEST_F(BitboardFixture, Black_King_Moves_With_Rooks_NoCastling)
{
    Bitboard board;
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

    u64 result = board.calcAvailableMoves(e8, k, 0x3, {});
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
TEST_F(BitboardFixture, White_King_Moves_With_Rooks)
{
    Bitboard board;
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

    u64 result = board.calcAvailableMoves(e1, K, 0x3, {});
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
TEST_F(BitboardFixture, White_King_Moves_With_Rooks_Blocked)
{
    Bitboard board;
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

    u64 result = board.calcAvailableMoves(e1, K, 0x3, {});
    EXPECT_EQ(expected, result);
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
TEST_F(BitboardFixture, White_Rook_Move)
{
    Bitboard board;
    auto R = WHITEROOK;

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

    u64 result = board.calcAvailableMoves(d4, R, 0, {});
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
TEST_F(BitboardFixture, Black_Rook_Move)
{
    Bitboard board;
    auto r = BLACKROOK;

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

    u64 result = board.calcAvailableMoves(b7, r, 0, {});
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
TEST_F(BitboardFixture, Bulk_Black_RooksMove)
{
    Bitboard board;
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

    u64 result = board.calcAvailableMovesRookBulk<Set::BLACK>();
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
TEST_F(BitboardFixture, Bulk_Black_TwoRooksMove)
{
    Bitboard board;
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

    u64 result = board.calcAvailableMovesRookBulk<Set::BLACK>();
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
TEST_F(BitboardFixture, Bulk_Black_BishopMove)
{
    Bitboard board;
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

    u64 result = board.calcAvailableMovesBishopBulk<Set::BLACK>();
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
TEST_F(BitboardFixture, Bulk_White_TwoBishopMove)
{
    Bitboard board;
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

    u64 result = board.calcAvailableMovesBishopBulk<Set::WHITE>();
    EXPECT_EQ(expected, result);
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
TEST_F(BitboardFixture, Bulk_White_TwoQueensMove)
{
    Bitboard board;
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

    u64 result = board.calcAvailableMovesBishopBulk<Set::WHITE, queenId>();
    result |= board.calcAvailableMovesRookBulk<Set::WHITE, queenId>();
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
TEST_F(BitboardFixture, Bulk_White_TwoQueensCanCaptureKnight)
{
    Bitboard board;
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

    u64 result = board.calcAvailableMovesBishopBulk<Set::WHITE, queenId>();
    result |= board.calcAvailableMovesRookBulk<Set::WHITE, queenId>();
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
TEST_F(BitboardFixture, Bulk_Black_TwoRooksMoveBlockEachOtherAndByKnight)
{
    Bitboard board;
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

    u64 result = board.calcAvailableMovesRookBulk<Set::BLACK>();
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
TEST_F(BitboardFixture, Black_Rook_Move_c1)
{
    Bitboard board;
    auto r = BLACKROOK;

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

    u64 result = board.calcAvailableMoves(c1, r, 0, {});
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
TEST_F(BitboardFixture, Black_Rook_Move_a8)
{
    Bitboard board;
    auto r = BLACKROOK;

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

    u64 result = board.calcAvailableMoves(a8, r, 0, {});
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
TEST_F(BitboardFixture, Black_Bishop_Move_f6)
{
    Bitboard board;
    auto b = BLACKBISHOP;

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

    u64 result = board.calcAvailableMoves(f6, b, 0, {});
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
TEST_F(BitboardFixture, White_Bishop_Move_c5_blocked)
{
    Bitboard board;
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

    u64 result = board.calcAvailableMoves(c5, B, 0, {});
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
TEST_F(BitboardFixture, Black_Queen_Moves_Capture_Available)
{
    Bitboard board;
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

    u64 result = board.calcAvailableMoves(c5, q, 0, {});
    EXPECT_EQ(expected, result);
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
TEST_F(BitboardFixture, White_Knight_Move_b2)
{
    Bitboard board;
    auto N = WHITEKNIGHT;

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << a4.index();
    expected |= INT64_C(1) << c4.index();
    expected |= INT64_C(1) << d3.index();
    expected |= INT64_C(1) << d1.index();

    u64 result = board.calcAvailableMoves(b2, N, 0, {});
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ x ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ P ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BitboardFixture, White_Pawn_Move_b3)
{
    Bitboard board;
    auto P = WHITEPAWN;

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << b4.index();

    u64 result = board.calcAvailableMoves(b3, P, 0, {});
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
TEST_F(BitboardFixture, Black_Pawn_Move_b3)
{
    Bitboard board;
    auto p = BLACKPAWN;

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << b2.index();

    u64 result = board.calcAvailableMoves(b3, p, 0, {});
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
TEST_F(BitboardFixture, White_Pawn_Move_b2)
{
    Bitboard board;
    auto P = WHITEPAWN;

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << b4.index();
    expected |= INT64_C(1) << b3.index();

    u64 result = board.calcAvailableMoves(b2, P, 0, {});
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
TEST_F(BitboardFixture, White_Pawn_Move_b2_blocked)
{
    Bitboard board;
    auto P = WHITEPAWN;
    auto N = WHITEKNIGHT;
    board.PlacePiece(N, c3);

    // setup
    u64 expected = ~universe;

    u64 result = board.calcAvailableMoves(c2, P, 0, {});
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
TEST_F(BitboardFixture, Black_Pawn_Move_e7)
{
    Bitboard board;
    auto p = BLACKPAWN;

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << e6.index();
    expected |= INT64_C(1) << e5.index();

    u64 result = board.calcAvailableMoves(e7, p, 0, {});
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
TEST_F(BitboardFixture, Bulk_PawnMovesBlack_NothingIsBlocked)
{
    // setup
    Bitboard board;
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
    u64 result = board.calcAvailableMovesPawnsBulk<Set::BLACK>();

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
TEST_F(BitboardFixture, Bulk_PawnMovesBlack_Blocked)
{
    // setup
    Bitboard board;
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
    expected |= INT64_C(1) << c5.index();
    expected |= INT64_C(1) << e6.index();
    expected |= INT64_C(1) << e4.index();

    // do
    u64 result = board.calcAvailableMovesPawnsBulk<Set::BLACK>();

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
TEST_F(BitboardFixture, Bulk_PawnMovesWhite_SomeMixOfBlockedAndNonBlocked)
{
    // setup
    Bitboard board;
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
    u64 result = board.calcAvailableMovesPawnsBulk<Set::WHITE>();

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
TEST_F(BitboardFixture, Bulk_PawnThreatsWhite_ThereShouldBeAFewThreatenedSquares)
{
    // setup
    Bitboard board;
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
    u64 result = board.calcThreatenedSquaresPawnsBulk<Set::WHITE>();

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
TEST_F(BitboardFixture, Bulk_PawnThreatsBlack_ThereShouldBeAFewThreatenedSquares)
{
    // setup
    Bitboard board;
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
    u64 result = board.calcThreatenedSquaresPawnsBulk<Set::BLACK>();

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
TEST_F(BitboardFixture, Bulk_PawnAttacksWhite_ThereShouldBeAFewAttackedPieces)
{
    // setup
    Bitboard board;
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
    u64 result = board.calcAvailableAttacksPawnsBulk<Set::WHITE>();

    // verify
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
TEST_F(BitboardFixture, White_Knight_Move_b5)
{
    Bitboard board;
    auto N = WHITEKNIGHT;

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << a7.index();
    expected |= INT64_C(1) << a3.index();
    expected |= INT64_C(1) << c7.index();
    expected |= INT64_C(1) << c3.index();
    expected |= INT64_C(1) << d6.index();
    expected |= INT64_C(1) << d4.index();

    u64 result = board.calcAvailableMoves(b5, N, 0, {});
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
TEST_F(BitboardFixture, White_Knight_Attack_e3)
{
    Bitboard board;
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
}

TEST_F(BitboardFixture, White_Knight_Attack_e3_ClearPiece)
{
    Bitboard board;
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

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ p ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ xB][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 4 [ x ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ P ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BitboardFixture, White_Pawn_Threaten)
{
    Bitboard board;
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
    u64 result = board.calcThreatenedSquaresPawnsBulk<Set::WHITE>();
    // validate
    EXPECT_EQ(expected, result);

    // setup
    expected = ~universe;
    expected |= INT64_C(1) << a5.index();
    expected |= INT64_C(1) << c5.index();
    // do
    result = board.calcThreatenedSquaresPawnsBulk<Set::BLACK>();
    // validate
    EXPECT_EQ(expected, result);

    // setup
    expected = ~universe;
    expected |= INT64_C(1) << a5.index();
    // do
    result = board.calcAvailableAttacksPawnsBulk<Set::BLACK>();
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
TEST_F(BitboardFixture, Black_Pawn_Avaliable_Move_EnPassant)
{
    Bitboard board;
    auto P = WHITEPAWN;
    auto p = BLACKPAWN;

    board.PlacePiece(P, g4);
    board.PlacePiece(p, f4);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << f3.index();
    expected |= INT64_C(1) << g3.index();

    auto enPassantSqr = g3;
    // do
    u64 result = board.calcAvailableMoves(f4, p, 0, enPassantSqr);
    // validate
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
TEST_F(BitboardFixture, White_Pawn_Avaliable_Move_EnPassant)
{
    Bitboard board;
    auto P = WHITEPAWN;
    auto p = BLACKPAWN;

    board.PlacePiece(P, g5);
    board.PlacePiece(p, f5);

    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << f6.index();
    expected |= INT64_C(1) << g6.index();

    auto enPassantSqr = f6;
    // do
    u64 result = board.calcAvailableMoves(g5, P, 0, enPassantSqr);
    // validate
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
TEST_F(BitboardFixture, Black_Rook_Threat_Blocked_By_Some_Pieces)
{
    Bitboard board;
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
    u64 result = board.calcThreatenedSquaresRookBulk<Set::BLACK>();
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
TEST_F(BitboardFixture, KingCheckedMask_Black_OnlyAvailableMoveIsToBlockCheck)
{
    Bitboard board;
    auto r = BLACKROOK;
    auto k = BLACKKING;
    auto R = WHITEROOK;

    // setup
    board.PlacePiece(r, c3);
    board.PlacePiece(k, e7);
    board.PlacePiece(R, e2);

    u64 expected = ~universe;
    expected |= INT64_C(1) << e3.index();

    // do
    constexpr bool includeMaterial = true;
    u64 threat = board.calcThreatenedSquares<Set::WHITE, includeMaterial>();
    KingMask kingMask = board.calcKingMask(k, e7, {threat, 0});
    KingMask checkedMask;
    for (u8 i = 0; i < 8; ++i) {
        checkedMask.threats[i] = threat & kingMask.threats[i];
    }

    u64 result = board.calcAvailableMoves(c3, r, 0, {}, threat, checkedMask, kingMask);

    // validate
    EXPECT_EQ(expected, result);
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
TEST_F(BitboardFixture, KingCheckedMask_Black_OnlyAvailableMoveIsToCaptureThreateningPiece)
{
    Bitboard board;
    auto r = BLACKROOK;
    auto k = BLACKKING;
    auto R = WHITEROOK;

    // setup
    board.PlacePiece(r, c2);
    board.PlacePiece(k, e7);
    board.PlacePiece(R, e2);

    u64 expected = ~universe;
    expected |= INT64_C(1) << e2.index();

    // do
    constexpr bool includeMaterial = true;
    u64 threatMask = board.calcThreatenedSquares<Set::WHITE, includeMaterial>();
    KingMask kingMask = board.calcKingMask(k, e7, {threatMask, 0});
    KingMask checkedMask;
    for (u8 i = 0; i < 8; ++i) {
        checkedMask.threats[i] = threatMask & kingMask.threats[i];
    }
    u64 result = board.calcAvailableMoves(c2, r, 0, {}, threatMask, checkedMask, kingMask);

    // validate
    EXPECT_EQ(expected, result);
}

/**
 * 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
 * 7 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
 * 6 [ . ][ . ][ . ][ . ][ x ][ . ][ . ][ . ]
 * 5 [ . ][ . ][ . ][ . ][ x ][ . ][ . ][ . ]
 * 4 [ . ][ . ][ . ][ . ][ r ][ . ][ . ][ . ]
 * 3 [ . ][ . ][ . ][ . ][ x ][ . ][ . ][ . ]
 * 2 [ . ][ . ][ . ][ . ][ xR][ . ][ . ][ . ]
 * 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
 *     A    B    C    D    E    F    G    H
 * @brief Even though the rook is pinned, it is able to move along the threatened squares. */
TEST_F(BitboardFixture, PinnedPiece_Black_AbleToMoveAlongThreatenedSquares)
{
    Bitboard board;
    auto r = BLACKROOK;
    auto k = BLACKKING;
    auto R = WHITEROOK;

    // setup
    board.PlacePiece(r, e4);
    board.PlacePiece(k, e7);
    board.PlacePiece(R, e2);

    u64 expected = ~universe;
    expected |= INT64_C(1) << e2.index();
    expected |= INT64_C(1) << e3.index();
    expected |= INT64_C(1) << e5.index();
    expected |= INT64_C(1) << e6.index();

    // do
    constexpr bool includeMaterial = true;
    u64 threatMask = board.calcThreatenedSquares<Set::WHITE, includeMaterial>();
    KingMask kingMask = board.calcKingMask(k, e7, {threatMask, 0});
    KingMask checkedMask;  // we are not in check so this should be 0.
    // for (u8 i = 0; i < 8; ++i)
    // {
    //     // we are not in check
    //     checkedMask.threats[i] = 0;//threatWithmat & kingMask.threats[i];
    // }
    u64 result = board.calcAvailableMoves(e4, r, 0, {}, threatMask, checkedMask, kingMask);

    // validate
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ R ][ . ][ . ][ . ]
// 1 [ . ][ . ][ r ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BitboardFixture, KingCheckedMask_Black_RookHasNoAvailableMoves)
{
    Bitboard board;
    auto r = BLACKROOK;
    auto k = BLACKKING;
    auto R = WHITEROOK;

    // setup
    board.PlacePiece(r, c1);
    board.PlacePiece(k, e7);
    board.PlacePiece(R, e2);

    u64 expected = ~universe;

    // do
    constexpr bool includeMaterial = true;
    u64 threatMask = board.calcThreatenedSquares<Set::WHITE, includeMaterial>();
    KingMask kingMask = board.calcKingMask(k, e7, {threatMask, 0});
    KingMask checkedMask;
    for (u8 i = 0; i < 8; ++i) {
        checkedMask.threats[i] = threatMask & kingMask.threats[i];
    }
    u64 result = board.calcAvailableMoves(c1, r, 0, {}, threatMask, kingMask, kingMask);

    // validate
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
TEST_F(BitboardFixture, PinnedPiece_Black_PawnHasNoAvailableMovesSinceItsPinned)
{
    Bitboard board;
    auto p = BLACKPAWN;
    auto k = BLACKKING;
    auto R = WHITEROOK;

    // setup
    board.PlacePiece(p, f7);
    board.PlacePiece(k, e7);
    board.PlacePiece(R, h7);

    u64 expected = ~universe;

    // do
    constexpr bool includeMaterial = true;
    u64 threatMask = board.calcThreatenedSquares<Set::WHITE, includeMaterial>();
    KingMask kingMask = board.calcKingMask(k, e7, {threatMask, 0});
    KingMask checkedMask;
    for (u8 i = 0; i < 8; ++i) {
        checkedMask.threats[i] = threatMask & kingMask.threats[i];
    }
    u64 result = board.calcAvailableMoves(f7, p, 0, {}, threatMask, checkedMask, kingMask);

    // validate
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ K ][ P ][ . ][ r ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BitboardFixture, PinnedPiece_White_PawnHasNoAvailableMovesSinceItsPinned)
{
    Bitboard board;
    auto p = WHITEPAWN;
    auto k = WHITEKING;
    auto R = BLACKROOK;

    // setup
    board.PlacePiece(p, f5);
    board.PlacePiece(k, e5);
    board.PlacePiece(R, h5);

    u64 expected = ~universe;

    // do
    constexpr bool includeMaterial = true;
    u64 orthogonalThreat = board.calcThreatenedSquaresOrthogonal<Set::BLACK, includeMaterial>();
    KingMask kingMask = board.calcKingMask(k, e5, {orthogonalThreat, 0});
    KingMask checkedMask;
    for (u8 i = 0; i < 8; ++i) {
        checkedMask.threats[i] = orthogonalThreat & kingMask.threats[i];
    }

    u64 threat = board.calcThreatenedSquares<Set::BLACK, includeMaterial>();
    u64 result = board.calcAvailableMoves(f5, p, 0, {}, threat, checkedMask, kingMask);

    // validate
    EXPECT_EQ(expected, result);
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
TEST_F(BitboardFixture, CheckedMask_Black_PawnCanMoveIntoBlockingCheckButNoFurther)
{
    Bitboard board;
    auto p = BLACKPAWN;
    auto k = BLACKKING;
    auto R = WHITEROOK;

    // setup
    board.PlacePiece(p, f7);
    board.PlacePiece(k, e6);
    board.PlacePiece(R, h6);

    u64 expected = ~universe;
    expected |= INT64_C(1) << f6.index();

    // do
    constexpr bool includeMaterial = true;
    u64 threatMask = board.calcThreatenedSquares<Set::WHITE, includeMaterial>();
    KingMask kingMask = board.calcKingMask(k, e6, {threatMask, 0});
    KingMask checkedMask;
    for (u8 i = 0; i < 8; ++i) {
        checkedMask.threats[i] = threatMask & kingMask.threats[i];
    }
    u64 result = board.calcAvailableMoves(f7, p, 0, {}, threatMask, checkedMask, kingMask);

    // validate
    EXPECT_EQ(expected, result);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ p ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ K ][ . ][ R ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BitboardFixture, CheckedMask_Black_PawnCanCaptureCheckingPiece)
{
    Bitboard board;
    auto p = BLACKPAWN;
    auto k = BLACKKING;
    auto R = WHITEROOK;

    // setup
    board.PlacePiece(p, f7);
    board.PlacePiece(k, e6);
    board.PlacePiece(R, g6);

    u64 expected = ~universe;
    expected |= INT64_C(1) << f6.index();
    expected |= INT64_C(1) << g6.index();

    // do
    constexpr bool includeMaterial = true;
    u64 threat = board.calcThreatenedSquares<Set::WHITE, includeMaterial>();
    KingMask kingMask = board.calcKingMask(k, e6, {threat, 0});
    KingMask checkedMask;
    for (u8 i = 0; i < 8; ++i) {
        checkedMask.threats[i] = threat & kingMask.threats[i];
    }
    u64 result = board.calcAvailableMoves(f7, p, 0, {}, threat, checkedMask, kingMask);

    // validate
    EXPECT_EQ(expected, result);
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
TEST_F(BitboardFixture, CheckedMask_Black_PawnCanOnlyDoubleMoveToBlockCheck)
{
    Bitboard board;
    auto p = BLACKPAWN;
    auto k = BLACKKING;
    auto R = WHITEROOK;

    // setup
    board.PlacePiece(p, f7);
    board.PlacePiece(k, e5);
    board.PlacePiece(R, g5);

    u64 expected = ~universe;
    expected |= INT64_C(1) << f5.index();

    // do
    constexpr bool includeMaterial = true;
    u64 threat = board.calcThreatenedSquares<Set::WHITE, includeMaterial>();
    KingMask kingMask = board.calcKingMask(k, e5, {threat, 0});
    KingMask checkedMask;
    for (u8 i = 0; i < 8; ++i) {
        checkedMask.threats[i] = threat & kingMask.threats[i];
    }
    u64 result = board.calcAvailableMoves(f7, p, 0, {}, threat, checkedMask, kingMask);

    // validate
    EXPECT_EQ(expected, result);
}

TEST_F(BitboardFixture, White_Queen_Threaten_Blocked_by_Pawns)
{
    Bitboard board;
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
    u64 threat = board.calcThreatenedSquaresQueensBulk<Set::WHITE>();

    // validate
    EXPECT_EQ(expected, threat);
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
TEST_F(BitboardFixture, Black_Rook_Threaten_Starting_Pos)
{
    Bitboard board;
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
    u64 threat = board.calcThreatenedSquaresRookBulk<Set::BLACK>();

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
TEST_F(BitboardFixture, KingMask_Pawns)
{
    Bitboard board;
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

TEST_F(BitboardFixture, White_Knight_Move)
{
    Bitboard board;
    auto N = WHITEKNIGHT;

    // setup
    board.PlacePiece(N, c3);

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
    u64 result = board.calcAvailableMoves(c3, N, 0, {}, 0, KingMask(), KingMask());

    // validate
    EXPECT_EQ(expected, result);
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
TEST_F(BitboardFixture, CheckedMask_Black_BishopCanOnlyBlockOrCaptureToCancelCheck)
{
    Bitboard board;

    // setup
    board.PlacePiece(WHITEROOK, a8);
    board.PlacePiece(BLACKBISHOP, b7);
    board.PlacePiece(BLACKKING, e8);

    u64 expected = ~universe;
    expected |= INT64_C(1) << a8.index();
    expected |= INT64_C(1) << c8.index();

    constexpr bool includeMaterial = true;
    u64 threatMask = board.calcThreatenedSquares<Set::WHITE, includeMaterial>();
    KingMask kingMask = board.calcKingMask(BLACKKING, e8, {threatMask, 0});
    KingMask checkedMask;
    for (u8 i = 0; i < 8; ++i) {
        checkedMask.threats[i] = threatMask & kingMask.threats[i];
    }

    // do
    u64 result = board.calcAvailableMoves(b7, BLACKBISHOP, 0, {}, threatMask, checkedMask, kingMask);

    // validate
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
TEST_F(BitboardFixture, Castling_BlockedByOpponentPieceInBetween)
{
    Bitboard board;

    // setup
    board.PlacePiece(BLACKROOK, a8);
    board.PlacePiece(WHITEBISHOP, b8);
    board.PlacePiece(BLACKKING, e8);

    // queen side castling available
    byte castling = 8;

    u64 expected = ~universe;
    expected |= INT64_C(1) << d8.index();
    expected |= INT64_C(1) << f8.index();
    expected |= INT64_C(1) << d7.index();
    expected |= INT64_C(1) << e7.index();
    expected |= INT64_C(1) << f7.index();

    // do
    u64 result = board.calcAvailableMoves(e8, BLACKKING, castling, {}, 0, KingMask(), KingMask());

    // validate
    EXPECT_EQ(expected, result);
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
TEST_F(BitboardFixture, Castling_BlockedByOwnPieceInBetween)
{
    Bitboard board;

    // setup
    board.PlacePiece(BLACKROOK, a8);
    board.PlacePiece(BLACKKNIGHT, b8);
    board.PlacePiece(BLACKKING, e8);

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
    u64 result = board.calcAvailableMoves(e8, BLACKKING, castling, {}, 0, KingMask(), KingMask());

    // validate
    EXPECT_EQ(expected, result);
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
TEST_F(BitboardFixture, KingMask_BishopAndQueenNotThreatening)
{
    Bitboard board;

    // setup
    board.PlacePiece(BLACKBISHOP, g7);
    board.PlacePiece(BLACKQUEEN, f3);
    board.PlacePiece(WHITEPAWN, g2);
    board.PlacePiece(WHITEKING, g1);

    // expected to be empty
    u64 expected = ~universe;

    // build sliding mask
    constexpr bool includeMaterial = true;
    u64 diagonal = board.calcThreatenedSquaresDiagonal<Set::BLACK, includeMaterial>();
    u64 orthogonal = board.calcThreatenedSquaresOrthogonal<Set::BLACK, includeMaterial>();

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
TEST_F(BitboardFixture, ThreatenedMask_KingisPierced)
{
    Bitboard board;

    // setup
    board.PlacePiece(BLACKROOK, g7);
    board.PlacePiece(WHITEKING, g2);

    u64 orthogonal = ~universe;
    constexpr bool includeMaterial = true;
    constexpr bool kingisPierced = true;
    orthogonal |= board.calcThreatenedSquares<Set::BLACK, includeMaterial, kingisPierced>();

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

// 8 [ . ][ . ][ . ][ . ][ . ][ x ][ . ][ x ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ b ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ x ][ . ][ x ]
// 5 [ . ][ . ][ . ][ . ][ x ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ x ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ x ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ x ][ . ][ b ][ . ][ . ][ . ][ . ]
// 1 [ x ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(BitboardFixture, IsolateBishop_Black_OnlyOneBishopLeftInTheMask)
{
    Bitboard board;
    board.PlacePiece(BLACKBISHOP, g7);
    board.PlacePiece(BLACKBISHOP, d2);

    u64 expected = 0xa000a0512a140215;
    u64 movesbb = board.calcAvailableMovesBishopBulk<Set::BLACK>();
    EXPECT_EQ(expected, movesbb);
    {
        expected = 0xa000a01008040201;
        auto [moves, attacks] = board.isolatePiece<Set::BLACK, bishopId>(g7, movesbb);
        EXPECT_EQ(expected, moves);
    }

    {
        expected = 0x804122140014;
        auto [moves, attacks] = board.isolatePiece<Set::BLACK, bishopId>(d2, movesbb);
        EXPECT_EQ(expected, moves);
    }
}

TEST_F(BitboardFixture, IsolateBishop_Black_BishopsOnSameDiagonal)
{
    Bitboard board;
    board.PlacePiece(BLACKBISHOP, c4);
    board.PlacePiece(BLACKBISHOP, e2);

    u64 expected = 0x4020118a402a0128;
    u64 movesbb = board.calcAvailableMovesBishopBulk<Set::BLACK>();
    EXPECT_EQ(expected, movesbb);

    {
        expected = 0x4020110a000a0100;
        auto [moves, attks] = board.isolatePiece<Set::BLACK, bishopId>(c4, movesbb);
        EXPECT_EQ(expected, moves);
    }

    {
        expected = 0x8040280028;
        auto [moves, attks] = board.isolatePiece<Set::BLACK, bishopId>(e2, movesbb);
        EXPECT_EQ(expected, moves);
    }
}

TEST_F(BitboardFixture, IsolateRook_Black_OnlyOneRookLeftInMask)
{
    Bitboard board;
    board.PlacePiece(BLACKROOK, g7);
    board.PlacePiece(BLACKROOK, d2);

    u64 expected = 0x40bf404040404040;
    u64 movesbb = board.calcAvailableMovesRookBulk<Set::BLACK>();

    auto [moves, attks] = board.isolatePiece<Set::BLACK, rookId>(g7, movesbb);
    EXPECT_EQ(expected, moves);
}

TEST_F(BitboardFixture, IsolateRook_Black_RooksAreOnSameRank)
{
    Bitboard board;
    board.PlacePiece(BLACKROOK, g7);
    board.PlacePiece(BLACKROOK, d7);

    u64 movesbb = board.calcAvailableMovesRookBulk<Set::BLACK>();
    {
        u64 expected = 0x40b0404040404040;
        auto [moves, attks] = board.isolatePiece<Set::BLACK, rookId>(g7, movesbb);
        EXPECT_EQ(expected, moves);
    }
    {
        u64 expected = 0x837080808080808;
        auto [moves, attks] = board.isolatePiece<Set::BLACK, rookId>(d7, movesbb);
        EXPECT_EQ(expected, moves);
    }
}

TEST_F(BitboardFixture, IsolateRook_Black_RooksAreOnSameFile)
{
    Bitboard board;
    board.PlacePiece(BLACKROOK, d4);
    board.PlacePiece(BLACKROOK, d7);

    u64 expected = 0x8f70808f7080808;
    u64 movesbb = board.calcAvailableMovesRookBulk<Set::BLACK>();
    EXPECT_EQ(expected, movesbb);

    {
        expected = 0x808f7080808;
        auto [moves, attk] = board.isolatePiece<Set::BLACK, rookId>(d4, movesbb);
        EXPECT_EQ(expected, moves);
    }
    {
        expected = 0x8f7080800000000;
        auto [moves, attk] = board.isolatePiece<Set::BLACK, rookId>(d7, movesbb);
        EXPECT_EQ(expected, moves);
    }
}

TEST_F(BitboardFixture, IsolatePawn_White_PawnOnDifferentRankAndFile)
{
    Bitboard board;
    board.PlacePiece(WHITEPAWN, d4);
    board.PlacePiece(WHITEPAWN, c2);

    u64 expected = 0x804040000;
    u64 movesbb = board.calcAvailableMovesPawnsBulk<Set::WHITE>();
    EXPECT_EQ(expected, movesbb);

    {
        expected = 0x800000000;
        auto [moves, attks] = board.isolatePiece<Set::WHITE, pawnId>(d4, movesbb);
        EXPECT_EQ(expected, moves);
    }

    expected = 0x4040000;
    auto [moves, attks] = board.isolatePiece<Set::WHITE, pawnId>(c2, movesbb);
    EXPECT_EQ(expected, moves);
}

TEST_F(BitboardFixture, IsolatePawn_White_PawnOnSameRank)
{
    Bitboard board;
    board.PlacePiece(WHITEPAWN, d4);
    board.PlacePiece(WHITEPAWN, e4);

    u64 expected = 0x1800000000;
    u64 movesbb = board.calcAvailableMovesPawnsBulk<Set::WHITE>();
    EXPECT_EQ(expected, movesbb);

    {
        expected = 0x800000000;
        auto [moves, attks] = board.isolatePiece<Set::WHITE, pawnId>(d4, movesbb);
        EXPECT_EQ(expected, moves);
    }

    expected = 0x1000000000;
    auto [moves, attks] = board.isolatePiece<Set::WHITE, pawnId>(e4, movesbb);
    EXPECT_EQ(expected, moves);
}

TEST_F(BitboardFixture, IsolatePawn_White_PawnOnSameFile)
{
    Bitboard board;
    board.PlacePiece(WHITEPAWN, d4);
    board.PlacePiece(WHITEPAWN, d2);

    u64 expected = 0x800080000;
    u64 movesbb = board.calcAvailableMovesPawnsBulk<Set::WHITE>();
    EXPECT_EQ(expected, movesbb);

    {
        expected = 0x800000000;
        auto [moves, attks] = board.isolatePiece<Set::WHITE, pawnId>(d4, movesbb);
        EXPECT_EQ(expected, moves);
    }

    expected = 0x80000;
    auto [moves, attks] = board.isolatePiece<Set::WHITE, pawnId>(d2, movesbb);
    EXPECT_EQ(expected, moves);
}

}  // namespace ElephantTest