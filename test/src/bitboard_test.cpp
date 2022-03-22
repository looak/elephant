#include <gtest/gtest.h>
#include "bitboard.h"
#include "elephant_test_utils.h"
#include "notation.h"
#include "chess_piece.h"
#include <array>

namespace ElephantTest
{
////////////////////////////////////////////////////////////////
class BitboardFixture : public ::testing::Test
{
public:
    /*   virtual void SetUp()
    {

    };
    virtual void TearDown() {};
*/
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
    
    for (byte i = 0; i < 64; ++i)
    {
        n = Notation(i);
        result = Bitboard::IsValidSquare(n);
        EXPECT_TRUE(result) << Notation::toString(n) << "\n";
    }

    for (byte i = 64; i < 0x80; ++i)
    {
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
    expected |= 1 << d1.index();
    // f1 should be available for moving
    expected |= 1 << f1.index();
    // d2 should be available for moving
    expected |= 1 << d2.index();
    // e2 should be available for moving
    expected |= 1 << e2.index();
    // f2 should be available for moving
    expected |= 1 << f2.index();

    auto result = board.GetAvailableMoves(e1, K);
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

    u64 result = board.GetAvailableMoves(d4, K);
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
    
    // setup
    u64 expected = ~universe;
    expected |= INT64_C(1) << d8.index();
    expected |= INT64_C(1) << d7.index();
    expected |= INT64_C(1) << e7.index();
    expected |= INT64_C(1) << f8.index();
    expected |= INT64_C(1) << f7.index();

    u64 result = board.GetAvailableMoves(e8, k);
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
    
    // setup a1 corner
    u64 expected = ~universe;
    expected |= INT64_C(1) << a2.index();
    expected |= INT64_C(1) << b2.index();
    expected |= INT64_C(1) << b1.index();

    u64 result = board.GetAvailableMoves(a1, k);
    EXPECT_EQ(expected, result);

    // setup a8 corner
    expected = ~universe;
    expected |= INT64_C(1) << b8.index();
    expected |= INT64_C(1) << b7.index();
    expected |= INT64_C(1) << a7.index();

    result = board.GetAvailableMoves(a8, k);
    EXPECT_EQ(expected, result);

    // setup h8 corner
    expected = ~universe;
    expected |= INT64_C(1) << h7.index();
    expected |= INT64_C(1) << g8.index();
    expected |= INT64_C(1) << g7.index();

    result = board.GetAvailableMoves(h8, k);
    EXPECT_EQ(expected, result);

    // setup h1 corner
    expected = ~universe;
    expected |= INT64_C(1) << h2.index();
    expected |= INT64_C(1) << g1.index();
    expected |= INT64_C(1) << g2.index();

    result = board.GetAvailableMoves(h1, k);
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

    byte castling = 0xc; // black has not moved king nor rooks and should have all castling available.
    u64 result = board.GetAvailableMoves(e8, k, castling);
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

    u64 result = board.GetAvailableMoves(e8, k, 0x3);
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

    u64 result = board.GetAvailableMoves(e1, K, 0x3);
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

    u64 result = board.GetAvailableMoves(e1, K, 0x3);
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

    u64 result = board.GetAvailableMoves(d4, R);
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

    u64 result = board.GetAvailableMoves(b7, r);
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

    u64 result = board.GetAvailableMoves(c1, r);
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

    u64 result = board.GetAvailableMoves(a8, r);
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
TEST_F(BitboardFixture, Black_Bishop_Move_b7)
{
    Bitboard board;
    auto b = BLACKBISHOP;

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

    u64 result = board.GetAvailableMoves(b7, b);
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

    u64 result = board.GetAvailableMoves(f6, b);
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

    u64 result = board.GetAvailableMoves(c5, B);
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

    u64 result = board.GetAvailableMoves(b2, N);
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

    u64 result = board.GetAvailableMoves(b3, P);
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

    u64 result = board.GetAvailableMoves(b3, p);
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


    u64 result = board.GetAvailableMoves(b2, P);
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

    u64 result = board.GetAvailableMoves(e7, p);
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

    u64 result = board.GetAvailableMoves(b5, N);
    EXPECT_EQ(expected, result);
}

// TEST_F(BitboardFixture, Pawn_Attack) {
//     GambitEngine::Bitboard board;
//     EXPECT_EQ(true, board.PlacePiece(WHITE, PAWN, 'e', 2));
//     board.PlacePiece(WHITE, PAWN, 'f', 3);
//     board.PlacePiece(BLACK, PAWN, 'd', 3);

//     u64 attked = ~universe;
//     // d3 should be attacked
//     attked |= INT64_C(1) << INT64_C(19);
//     // e4 should be attacked
//     attked |= INT64_C(1) << INT64_C(28);
//     // g4 should be attacked
//     attked |= INT64_C(1) << INT64_C(30);
//         // f3 should be guarded
//     attked |= INT64_C(1) << INT64_C(21);

//     auto result = board.Attacked(WHITE);
//     EXPECT_EQ(~universe, result ^ attked);

//     attked = ~universe;
//     // e4 should be attacked
//     attked |= INT64_C(1) << INT64_C(10);
//     // g4 should be attacked
//     attked |= INT64_C(1) << INT64_C(12);

//     result = board.Attacked(BLACK);
//     EXPECT_FALSE(result ^ attked);
// }

// TEST_F(BitboardFixture, King_Available_Moves)
// {
//     GambitEngine::Bitboard board;
//     board.PlacePiece(WHITE, KING, 'e', 1);

//     u64 expectedAvaMv = ~universe;
//     expectedAvaMv |= INT64_C(1) << INT64_C(3);
//     expectedAvaMv |= INT64_C(1) << INT64_C(5);
//     expectedAvaMv |= INT64_C(1) << INT64_C(11);
//     expectedAvaMv |= INT64_C(1) << INT64_C(12);
//     expectedAvaMv |= INT64_C(1) << INT64_C(13);

//     u64 avaMoves = board.AvailableMovesSimple(WHITE, KING, 4);

//     EXPECT_EQ(expectedAvaMv, avaMoves);
// }

// TEST_F(BitboardFixture, King_Black_Available_Moves)
// {
//     GambitEngine::Bitboard board;
//     board.PlacePiece(BLACK, KING, 'e', 8);

//     u64 expectedAvaMv = ~universe;
//     expectedAvaMv |= INT64_C(1) << INT64_C(59);
//     expectedAvaMv |= INT64_C(1) << INT64_C(61);
//     expectedAvaMv |= INT64_C(1) << INT64_C(53);
//     expectedAvaMv |= INT64_C(1) << INT64_C(52);
//     expectedAvaMv |= INT64_C(1) << INT64_C(51);

//     byte prom = 0;
//     u64 avaMoves = board.AvailableMoves(BLACK, KING, 60, 0, 0, prom);

//     EXPECT_EQ(expectedAvaMv, avaMoves);
// }


// TEST_F(BitboardFixture, King_Black_Available_Moves_With_Rooks)
// {
//     GambitEngine::Bitboard board;
//     board.PlacePiece(BLACK, KING, 'e', 8);
//     board.PlacePiece(BLACK, ROOK, 'h', 8);
//     board.PlacePiece(BLACK, ROOK, 'a', 8);

//     u64 expectedAvaMv = ~universe;
//     expectedAvaMv |= INT64_C(1) << INT64_C(58);
//     expectedAvaMv |= INT64_C(1) << INT64_C(59);
//     expectedAvaMv |= INT64_C(1) << INT64_C(61);
//     expectedAvaMv |= INT64_C(1) << INT64_C(62);
//     expectedAvaMv |= INT64_C(1) << INT64_C(53);
//     expectedAvaMv |= INT64_C(1) << INT64_C(52);
//     expectedAvaMv |= INT64_C(1) << INT64_C(51);

//     byte prom = 0;
//     u64 avaMoves = board.AvailableMoves(BLACK, KING, 60, 0, 0xc, prom);

//     EXPECT_EQ(expectedAvaMv, avaMoves);
// }

// TEST_F(BitboardFixture, Knight_Available_Moves)
// {
//     GambitEngine::Bitboard board;
//     board.PlacePiece(BLACK, KNIGHT, 'd', 4);

//     u64 expectedAvaMv = ~universe;
//     expectedAvaMv |= INT64_C(1) << INT64_C(42);
//     expectedAvaMv |= INT64_C(1) << INT64_C(44);
//     expectedAvaMv |= INT64_C(1) << INT64_C(33);
//     expectedAvaMv |= INT64_C(1) << INT64_C(37);
//     expectedAvaMv |= INT64_C(1) << INT64_C(17);
//     expectedAvaMv |= INT64_C(1) << INT64_C(21);
//     expectedAvaMv |= INT64_C(1) << INT64_C(10);
//     expectedAvaMv |= INT64_C(1) << INT64_C(12);

//     byte prom = 0;
//     u64 avaMoves = board.AvailableMoves(BLACK, KNIGHT, 27, 0, 0, prom);
//     EXPECT_EQ(expectedAvaMv, avaMoves);

//     board.PlacePiece(BLACK, PAWN, 44);
//     avaMoves = board.AvailableMoves(BLACK, KNIGHT, 27, 0, 0, prom);

//     expectedAvaMv ^= INT64_C(1) << INT64_C(44);
//     EXPECT_EQ(expectedAvaMv, avaMoves);

//     board.PlacePiece(WHITE, PAWN, 37);
//     avaMoves = board.AvailableMoves(BLACK, KNIGHT, 27, 0, 0, prom);
//     EXPECT_EQ(expectedAvaMv, avaMoves);
// }

// TEST_F(BitboardFixture, Knight_Pinned_NoAvaMoves)
// {
//     GambitEngine::Bitboard board;
//     board.PlacePiece(BLACK, KNIGHT, 'd', 4);
//     board.PlacePiece(BLACK, KING, 'd', 8);

//     board.PlacePiece(WHITE, ROOK, 'd', 1);

//     u64 expectedAvaMv = ~universe;

//     byte prom = 0;
//     u64 avaMoves = board.AvailableMoves(BLACK, KNIGHT, 27, 0, 0, prom);
//     EXPECT_EQ(expectedAvaMv, avaMoves);
// }

// TEST_F(BitboardFixture, Rook_Pinned_LimitedMoves)
// {
//     GambitEngine::Bitboard board;
//     board.PlacePiece(BLACK, ROOK, 'd', 4);
//     board.PlacePiece(BLACK, KING, 'd', 8);

//     board.PlacePiece(WHITE, ROOK, 'd', 1);

//     u64 expectedAvaMv = ~universe;
//     expectedAvaMv |= INT64_C(1) << INT64_C(19);
//     expectedAvaMv |= INT64_C(1) << INT64_C(11);
//     expectedAvaMv |= INT64_C(1) << INT64_C(3);
//     expectedAvaMv |= INT64_C(1) << INT64_C(35);
//     expectedAvaMv |= INT64_C(1) << INT64_C(43);
//     expectedAvaMv |= INT64_C(1) << INT64_C(51);

//     byte prom = 0;
//     u64 avaMoves = board.AvailableMoves(BLACK, ROOK, 27, 0, 0, prom);
//     EXPECT_EQ(expectedAvaMv, avaMoves);
// }

// TEST_F(BitboardFixture, Rook_Pinned_KingCanMove)
// {
//     GambitEngine::Bitboard board;
//     board.PlacePiece(BLACK, ROOK, 'd', 4);
//     board.PlacePiece(BLACK, KING, 'd', 8);

//     board.PlacePiece(WHITE, ROOK, 'd', 1);

//     u64 expectedAvaMv = ~universe;
//     expectedAvaMv |= INT64_C(1) << INT64_C(58);
//     expectedAvaMv |= INT64_C(1) << INT64_C(60);
//     expectedAvaMv |= INT64_C(1) << INT64_C(50);
//     expectedAvaMv |= INT64_C(1) << INT64_C(51);
//     expectedAvaMv |= INT64_C(1) << INT64_C(52);

//     byte prom = 0;
//     u64 avaMoves = board.AvailableMoves(BLACK, KING, 59, 0, 0, prom);
//     EXPECT_EQ(expectedAvaMv, avaMoves);
// }

// TEST_F(BitboardFixture, Rook_Pinned_ButNot_TwoPieces)
// {
//     GambitEngine::Bitboard board;
//     board.PlacePiece(BLACK, ROOK, 'd', 4);
//     board.PlacePiece(BLACK, ROOK, 'd', 3);
//     board.PlacePiece(BLACK, KING, 'd', 8);

//     board.PlacePiece(WHITE, ROOK, 'd', 1);

//     u64 expectedAvaMv = ~universe;
//     expectedAvaMv |= INT64_C(1) << INT64_C(16);
//     expectedAvaMv |= INT64_C(1) << INT64_C(17);
//     expectedAvaMv |= INT64_C(1) << INT64_C(18);
//     expectedAvaMv |= INT64_C(1) << INT64_C(20);
//     expectedAvaMv |= INT64_C(1) << INT64_C(21);
//     expectedAvaMv |= INT64_C(1) << INT64_C(22);
//     expectedAvaMv |= INT64_C(1) << INT64_C(23);
//     expectedAvaMv |= INT64_C(1) << INT64_C(11);
//     expectedAvaMv |= INT64_C(1) << INT64_C(3);

//     byte prom = 0;
//     u64 avaMoves = board.AvailableMoves(BLACK, ROOK, 19, 0, 0, prom);
//     EXPECT_EQ(expectedAvaMv, avaMoves);
// }
////////////////////////////////////////////////////////////////

} // namespace ElephantTest