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
    expected |= INT64_C(1) << d1.index();
    // f1 should be available for moving
    expected |= INT64_C(1) << f1.index();
    // d2 should be available for moving
    expected |= INT64_C(1) << d2.index();
    // e2 should be available for moving
    expected |= INT64_C(1) << e2.index();
    // f2 should be available for moving
    expected |= INT64_C(1) << f2.index();

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

    u64 result = board.GetAvailableMoves(d5, K);
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

    u64 result = board.GetAvailableMoves(c5, q);
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

    u64 result = board.GetAvailableMoves(c2, P);
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

    u64 result = board.GetAttackedSquares(e3, N);
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

    u64 result = board.GetAttackedSquares(e3, N);
    EXPECT_EQ(expected, result);

    // clear piece
    board.ClearPiece(b, d5);

    expected = ~universe;
    result = board.GetAttackedSquares(e3, N);
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
    u64 result = board.GetThreatenedSquares(b3, P);
    // validate
    EXPECT_EQ(expected, result);

    // setup
    expected = ~universe;
    expected |= INT64_C(1) << a5.index();
    expected |= INT64_C(1) << c5.index();
    // do
    result = board.GetThreatenedSquares(b6, p);
    // validate
    EXPECT_EQ(expected, result);

    // setup
    expected = ~universe;
    expected |= INT64_C(1) << a5.index();
    // do
    result = board.GetAttackedSquares(b6, p);
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
    u64 result = board.GetAvailableMoves(f4, p, 0, enPassantSqr.index());
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
    u64 result = board.GetAvailableMoves(g5, P, 0, enPassantSqr.index());
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
    u64 result = board.GetThreatenedSquares(c3, r);
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
TEST_F(BitboardFixture, Black_Rook_Only_Available_Move_To_Block_Check)
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
    u64 threat = board.GetThreatenedSquares(e2, R);    
    u64 kingMask = board.GetKingMask(k, e7, threat);
    u64 result = board.GetAvailableMoves(c3, r, 0, 0, threat, true, kingMask);

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
TEST_F(BitboardFixture, Black_Rook_Only_Available_Move_To_Capture)
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
    //u64 threat = board.GetThreatenedSquares(e2, R);
    u64 threatWithmat = board.GetThreatenedSquaresWithMaterial(e2, R);
    u64 kingMask = board.GetKingMask(k, e7, threatWithmat);
    u64 result = board.GetAvailableMoves(c2, r, 0, 0, threatWithmat, true, kingMask);

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
TEST_F(BitboardFixture, Black_Rook_No_Available_Moves)
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
    u64 threat = board.GetThreatenedSquares(e2, R);
    u64 kingMask = board.GetKingMask(k, e7, 0);
    u64 result = board.GetAvailableMoves(c1, r, 0, 0, threat, true, kingMask);

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
TEST_F(BitboardFixture, Pawn_Pinned_No_Available_Moves)
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
    u64 threat = board.GetThreatenedSquares(h7, R);
    u64 kingMask = board.GetKingMask(k, e7, threat);
    u64 result = board.GetAvailableMoves(f7, p, 0, 0, threat, false, kingMask);

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
TEST_F(BitboardFixture, Pawn_Pinned_No_Available_Moves_White)
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
    u64 threat = board.GetThreatenedSquaresWithMaterial(h5, R);
    u64 kingMask = board.GetKingMask(k, e5, threat);
    u64 result = board.GetAvailableMoves(f5, p, 0, 0, threat, false, kingMask);

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
TEST_F(BitboardFixture, Pawn_Block_Check)
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
    u64 threat = board.GetThreatenedSquaresWithMaterial(h6, R);
    u64 kingMask = board.GetKingMask(k, e6, threat);
    u64 result = board.GetAvailableMoves(f7, p, 0, 0, threat, true, kingMask);

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
TEST_F(BitboardFixture, Pawn_Capture_Check)
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
    u64 threat = board.GetThreatenedSquaresWithMaterial(g6, R);
    u64 kingMask = board.GetKingMask(k, e6, threat);
    u64 result = board.GetAvailableMoves(f7, p, 0, 0, threat, true, kingMask);

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
TEST_F(BitboardFixture, Pawn_Block_Check_DoubleMove)
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
    u64 threat = board.GetThreatenedSquaresWithMaterial(g5, R);
    u64 kingMask = board.GetKingMask(k, e5, threat);
    u64 result = board.GetAvailableMoves(f7, p, 0, 0, threat, true, kingMask);

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
    u64 threat = board.GetThreatenedSquares(d1, Q);  

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
    // this test is not done yet wip. Had to leave.
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
    u64 threat = board.GetThreatenedSquares(a8, r);

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
    // this test is not done yet wip. Had to leave.
    Bitboard board;
    auto k = BLACKKING;
    auto P = WHITEPAWN;

    // setup
    board.PlacePiece(k, a8);
    board.PlacePiece(P, b7);

    u64 expected = ~universe;
    expected |= INT64_C(1) << b7.index();

    // do
    u64 kingMask = board.GetKingMask(k, a8, 0);

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
     u64 result = board.GetAvailableMoves(c3, N, 0, 0, 0, false, 0);

    // validate
    EXPECT_EQ(expected, result);
}

} // namespace ElephantTest