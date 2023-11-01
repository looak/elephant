#include "bitboard.hpp"
#include <gtest/gtest.h>

namespace ElephantTest {
////////////////////////////////////////////////////////////////

/**
 * @file bitboard_test.cpp
 * @brief Fixture for testing bitboard functionality.
 * Naming convention as of October 2023: <TestedFunctionality>_<ExpectedResult>
 * @author Alexander Loodin Ek  */
class BitboardFixture : public ::testing::Test {
public:
};
////////////////////////////////////////////////////////////////

TEST_F(BitboardFixture, EmptyBitboard_BitboardShouldBeZero)
{
    Bitboard bb;
    EXPECT_EQ(bb.read(), 0x0);
}

TEST_F(BitboardFixture, BitwiseNotOperator_BitboardShouldOppositeOfOriginal)
{
    Bitboard orig(0xFFAAFFAAFFAAFFAA);
    Bitboard bitwiseNot(~orig);
    u64 expected = 0x55005500550055;
    EXPECT_EQ(bitwiseNot.read(), expected);
}

TEST_F(BitboardFixture, AssignmentOperator_ShouldBeEqualAfterAssignment)
{
    Bitboard orig(0xFFAAFFAAFFAAFFAA);
    Bitboard copy = orig;
    EXPECT_EQ(copy.read(), orig.read());
}

TEST_F(BitboardFixture, EqualsOperator_ShouldBeEqualAndNotEqual)
{
    Bitboard orig(0xFFAAFFAAFFAAFFAA);
    Bitboard copy = orig;
    EXPECT_TRUE(copy == orig);
    EXPECT_FALSE(copy != orig);

    Bitboard empty;
    EXPECT_FALSE(empty == orig);
    EXPECT_TRUE(empty != orig);
}

TEST_F(BitboardFixture, BitwiseOrOperator_ExpectingCorrectOr)
{
    Bitboard vertical(0x1818181818181818);
    Bitboard horizontal(0xFFFF000000);
    Bitboard empty;
    empty = vertical | empty;
    EXPECT_EQ(empty, vertical);
    Bitboard result = vertical | horizontal;
    EXPECT_EQ(result, 0x181818FFFF181818);

    vertical |= horizontal;
    EXPECT_EQ(vertical, 0x181818FFFF181818);
}

TEST_F(BitboardFixture, BitwiseAndOperator_ExpectingCorrectAnd)
{
    Bitboard vertical(0x1818181818181818);
    Bitboard horizontal(0xFFFF000000);
    Bitboard empty;
    empty = vertical & empty;
    EXPECT_EQ(empty, 0x0);
    Bitboard result = vertical & horizontal;
    EXPECT_EQ(result, 0x1818000000);

    vertical &= horizontal;
    EXPECT_EQ(vertical, 0x1818000000);
}

TEST_F(BitboardFixture, BitwiseXorOperator_ExpectingCorrectXor)
{
    Bitboard vertical(0x1818181818181818);
    Bitboard horizontal(0xFFFF000000);
    Bitboard empty;
    empty = vertical ^ empty;
    EXPECT_EQ(empty, vertical);
    Bitboard result = vertical ^ horizontal;
    EXPECT_EQ(result, 0x181818E7E7181818);

    vertical ^= horizontal;
    EXPECT_EQ(vertical, 0x181818E7E7181818);
}

TEST_F(BitboardFixture, SquareBracketOperator_ReadingSpecificSquares)
{
    const Bitboard bb(0x8100001818000081);
    EXPECT_EQ(bb[Square::A1], true);
    EXPECT_EQ(bb[Square::A2], false);
    EXPECT_EQ(bb[Square::A3], false);
    EXPECT_EQ(bb[Square::A4], false);
    EXPECT_EQ(bb[Square::A5], false);
    EXPECT_EQ(bb[Square::A6], false);
    EXPECT_EQ(bb[Square::A7], false);
    EXPECT_EQ(bb[Square::A8], true);

    EXPECT_EQ(bb[Square::B1], false);
    EXPECT_EQ(bb[Square::B2], false);
    EXPECT_EQ(bb[Square::B3], false);
    EXPECT_EQ(bb[Square::B4], false);
    EXPECT_EQ(bb[Square::B5], false);
    EXPECT_EQ(bb[Square::B6], false);
    EXPECT_EQ(bb[Square::B7], false);
    EXPECT_EQ(bb[Square::B8], false);

    EXPECT_EQ(bb[Square::C1], false);
    EXPECT_EQ(bb[Square::C2], false);
    EXPECT_EQ(bb[Square::C3], false);
    EXPECT_EQ(bb[Square::C4], false);
    EXPECT_EQ(bb[Square::C5], false);
    EXPECT_EQ(bb[Square::C6], false);
    EXPECT_EQ(bb[Square::C7], false);
    EXPECT_EQ(bb[Square::C8], false);

    EXPECT_EQ(bb[Square::D1], false);
    EXPECT_EQ(bb[Square::D2], false);
    EXPECT_EQ(bb[Square::D3], false);
    EXPECT_EQ(bb[Square::D4], true);
    EXPECT_EQ(bb[Square::D5], true);
    EXPECT_EQ(bb[Square::D6], false);
    EXPECT_EQ(bb[Square::D7], false);
    EXPECT_EQ(bb[Square::D8], false);

    EXPECT_EQ(bb[Square::E1], false);
    EXPECT_EQ(bb[Square::E2], false);
    EXPECT_EQ(bb[Square::E3], false);
    EXPECT_EQ(bb[Square::E4], true);
    EXPECT_EQ(bb[Square::E5], true);
    EXPECT_EQ(bb[Square::E6], false);
    EXPECT_EQ(bb[Square::E7], false);
    EXPECT_EQ(bb[Square::E8], false);

    EXPECT_EQ(bb[Square::F1], false);
    EXPECT_EQ(bb[Square::F2], false);
    EXPECT_EQ(bb[Square::F3], false);
    EXPECT_EQ(bb[Square::F4], false);
    EXPECT_EQ(bb[Square::F5], false);
    EXPECT_EQ(bb[Square::F6], false);
    EXPECT_EQ(bb[Square::F7], false);
    EXPECT_EQ(bb[Square::F8], false);

    EXPECT_EQ(bb[Square::G1], false);
    EXPECT_EQ(bb[Square::G2], false);
    EXPECT_EQ(bb[Square::G3], false);
    EXPECT_EQ(bb[Square::G4], false);
    EXPECT_EQ(bb[Square::G5], false);
    EXPECT_EQ(bb[Square::G6], false);
    EXPECT_EQ(bb[Square::G7], false);
    EXPECT_EQ(bb[Square::G8], false);

    EXPECT_EQ(bb[Square::H1], true);
    EXPECT_EQ(bb[Square::H2], false);
    EXPECT_EQ(bb[Square::H3], false);
    EXPECT_EQ(bb[Square::H4], false);
    EXPECT_EQ(bb[Square::H5], false);
    EXPECT_EQ(bb[Square::H6], false);
    EXPECT_EQ(bb[Square::H7], false);
    EXPECT_EQ(bb[Square::H8], true);
}

TEST_F(BitboardFixture, SquareBracketOperator_WritingSpecificSquare)
{
    Bitboard bb(0x8100001818000081);
    Bitboard expected(0x8100001818000081);
    EXPECT_TRUE(bb[Square::A1]);
    bb[Square::A1] = true;
    EXPECT_TRUE(bb[Square::A1]);
    EXPECT_EQ(bb, expected);

    bb[Square::A1] = false;
    EXPECT_FALSE(bb[Square::A1]);
    expected = 0x8100001818000080;
    EXPECT_EQ(bb, expected);

    EXPECT_FALSE(bb[Square::F3]);
    bb[Square::F3] = false;
    EXPECT_FALSE(bb[Square::F3]);
    EXPECT_EQ(bb, expected);

    bb[Square::F3] = true;
    EXPECT_TRUE(bb[Square::F3]);
    expected = 0x8100001818200080;
    EXPECT_EQ(bb, expected);
}

////////////////////////////////////////////////////////////////

}  // namespace ElephantTest