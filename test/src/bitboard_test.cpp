#include <bitboard/bitboard.hpp>
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
    Bitboard orig(0xFFAAFFAAFFAAFFAAULL);
    Bitboard bitwiseNot(~orig);
    u64 expected = 0x55005500550055ULL;
    EXPECT_EQ(bitwiseNot.read(), expected);
}

TEST_F(BitboardFixture, AssignmentOperator_ShouldBeEqualAfterAssignment)
{
    Bitboard orig(0xFFAAFFAAFFAAFFAAULL);
    Bitboard copy = orig;
    EXPECT_EQ(copy.read(), orig.read());
}

TEST_F(BitboardFixture, EqualsOperator_ShouldBeEqualAndNotEqual)
{
    Bitboard orig(0xFFAAFFAAFFAAFFAAULL);
    Bitboard copy = orig;
    EXPECT_TRUE(copy == orig);
    EXPECT_FALSE(copy != orig);

    Bitboard empty;
    EXPECT_FALSE(empty == orig);
    EXPECT_TRUE(empty != orig);
}

TEST_F(BitboardFixture, BitwiseOrOperator_ExpectingCorrectOr)
{
    Bitboard vertical(0x1818181818181818ULL);
    Bitboard horizontal(0xFFFF000000ULL);
    Bitboard empty;
    empty = vertical | empty;
    EXPECT_EQ(empty, vertical);
    Bitboard result = vertical | horizontal;
    EXPECT_EQ(result, 0x181818FFFF181818ULL);

    vertical |= horizontal;
    EXPECT_EQ(vertical, 0x181818FFFF181818ULL);
}

TEST_F(BitboardFixture, BitwiseAndOperator_ExpectingCorrectAnd)
{
    Bitboard vertical(0x1818181818181818ULL);
    Bitboard horizontal(0xFFFF000000ULL);
    Bitboard empty;
    empty = vertical & empty;
    EXPECT_EQ(empty, 0x0ULL);
    Bitboard result = vertical & horizontal;
    EXPECT_EQ(result, 0x1818000000ULL);

    vertical &= horizontal;
    EXPECT_EQ(vertical, 0x1818000000ULL);
}

TEST_F(BitboardFixture, BitwiseXorOperator_ExpectingCorrectXor)
{
    Bitboard vertical(0x1818181818181818ULL);
    Bitboard horizontal(0xFFFF000000ULL);
    Bitboard empty;
    empty = vertical ^ empty;
    EXPECT_EQ(empty, vertical);
    Bitboard result = vertical ^ horizontal;
    EXPECT_EQ(result, 0x181818E7E7181818ULL);

    vertical ^= horizontal;
    EXPECT_EQ(vertical, 0x181818E7E7181818ULL);
}

TEST_F(BitboardFixture, SquareBracketOperator_ReadingSpecificSquares)
{
    const Bitboard bb(0x8100001818000081ULL);
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
    Bitboard bb(0x8100001818000081ULL);
    Bitboard expected(0x8100001818000081ULL);
    EXPECT_TRUE(bb[Square::A1]);
    bb[Square::A1] = true;
    EXPECT_TRUE(bb[Square::A1]);
    EXPECT_EQ(bb, expected);

    bb[Square::A1] = false;
    EXPECT_FALSE(bb[Square::A1]);
    expected = 0x8100001818000080ULL;
    EXPECT_EQ(bb, expected);

    EXPECT_FALSE(bb[Square::F3]);
    bb[Square::F3] = false;
    EXPECT_FALSE(bb[Square::F3]);
    EXPECT_EQ(bb, expected);

    bb[Square::F3] = true;
    EXPECT_TRUE(bb[Square::F3]);
    expected = 0x8100001818200080ULL;
    EXPECT_EQ(bb, expected);
}

TEST_F(BitboardFixture, InclusiveFillWest_ExpectedToBeFilledFromGivenFileToWestEdge)
{
    Bitboard bb(0x0ULL);
    bb = bb.inclusiveFillWest(file_e);

    u64 expected = 0x1F1F1F1F1F1F1F1F;
    EXPECT_EQ(bb.read(), expected);
}

TEST_F(BitboardFixture, InclusiveFillRest_ExpectedToBeFilledFromGivenFileToEdge)
{
    Bitboard bb(0x0ULL);
    bb = bb.inclusiveFillEast(file_b);
    u64 expected = 0xFEFEFEFEFEFEFEFEULL;
    EXPECT_EQ(bb.read(), expected);

    i8 rank6 = 5;  // zero indexed;
    bb = bb.inclusiveFillNorth(rank6);
    expected = 0xFFFFFF0000000000ULL;
    EXPECT_EQ(bb.read(), expected);

    i8 rank8 = 7;  // zero indexed;
    bb = bb.inclusiveFillSouth(rank8);
    expected = 0xFFFFFFFFFFFFFFFFULL;
    EXPECT_EQ(bb.read(), expected);

    bb = bb.inclusiveFillSouth(rank_1);
    expected = 0xff;
    EXPECT_EQ(bb.read(), expected);

    bb = bb.inclusiveFillNorth(rank_8);
    expected = 0xff00000000000000ULL;
    EXPECT_EQ(bb.read(), expected);
}

TEST_F(BitboardFixture, InclusiveFillNorthEast_ExpectedToBeFilledFromGivenPositionToNorthEastCorner)
{
    Bitboard bb(0x0);

    bb = bb.inclusiveFillNorthEast(file_e, rank_6);

    u64 expected = 0xfcf8f0e0c0800000ULL;
    EXPECT_EQ(bb.read(), expected);

    bb = bb.inclusiveFillNorthEast(file_f, rank_5);
    EXPECT_EQ(bb.read(), expected);

    bb = bb.inclusiveFillNorthEast(file_b, rank_4);
    expected = 0xfffffffffefcf8f0ULL;
    EXPECT_EQ(bb.read(), expected);

    bb = bb.inclusiveFillNorthEast(file_h, rank_8);
    expected = 0x8000000000000000ULL;
    EXPECT_EQ(bb.read(), expected);
}

TEST_F(BitboardFixture, InclusiveFillSouthWest_ExpectedToBeFilledFromGivenPositionToSouthWestCorner)
{
    Bitboard bb(0x0);

    bb = bb.inclusiveFillSouthWest(file_c, rank_5);
    u64 expected = 0x103070f1f3f7fULL;
    EXPECT_EQ(bb.read(), expected);

    bb = bb.inclusiveFillSouthWest(file_c, rank_2);
    expected = 0x103070fULL;
    EXPECT_EQ(bb.read(), expected);
}

TEST_F(BitboardFixture, InclusiveFillForwardDiagonal_ExpectedToBeFilledFromGivenPositionToNorthWestCornerOrSouthEastCorner)
{
    Bitboard bb(0x0ULL);

    bb = bb.inclusiveFillNorthWest(file_c, rank_6);
    u64 expected = 0x1f0f070301000000ULL;
    EXPECT_EQ(bb.read(), expected);

    bb = bb.inclusiveFillNorthWest(file_b, rank_7);
    expected = 0x703010000000000ULL;
    EXPECT_EQ(bb.read(), expected);

    bb = bb.inclusiveFillSouthEast(file_h, rank_4);
    expected = 0x80c0e0f0ULL;
    EXPECT_EQ(bb.read(), expected);
}

// from the perspective of white player
TEST_F(BitboardFixture, ShiftsNorth_ExpectBitsToBeShiftedNorthOnBoard)
{
    Bitboard bb(0x1e000000ULL);
    u64 expected = 0x1e00000000ULL;

    bb = bb.shiftNorth();
    EXPECT_EQ(bb.read(), expected);

    bb = 0xff000000;
    expected = 0xff00000000ULL;
    bb = bb.shiftNorth();
    EXPECT_EQ(bb.read(), expected);
}

TEST_F(BitboardFixture, ShiftsSouth_ExpectShiftsToWorkProperly)
{
    Bitboard bb(0x1e000000ULL);
    u64 expected = 0x1e0000ULL;

    bb = bb.shiftSouth();
    EXPECT_EQ(bb.read(), expected);

    bb = 0xff000000;
    expected = 0xff0000ULL;
    bb = bb.shiftSouth();
    EXPECT_EQ(bb.read(), expected);
}

TEST_F(BitboardFixture, ShiftsEast_ExpectShiftsToWorkProperly)
{
    Bitboard bb(0x1e000000ULL);
    u64 expected = 0x3c000000ULL;

    bb = bb.shiftEast();
    EXPECT_EQ(bb.read(), expected);

    bb = 0xff000000;
    expected = 0x1fe000000ULL;
    bb = bb.shiftEast();
    EXPECT_EQ(bb.read(), expected);
}

TEST_F(BitboardFixture, ShiftsWest_ExpectShiftsToWorkProperly)
{
    Bitboard bb(0x3c000000ULL);
    u64 expected = 0x1e000000ULL;

    bb = bb.shiftWest();
    EXPECT_EQ(bb.read(), expected);

    bb = 0x1fe000000;
    expected = 0xff000000ULL;
    bb = bb.shiftWest();
    EXPECT_EQ(bb.read(), expected);
}

TEST_F(BitboardFixture, ShiftsNorthEast_ExpectShiftsToWorkProperly)
{
    Bitboard bb(0x1e000000ULL);
    u64 expected = 0x3c00000000ULL;

    bb = bb.shiftNorthEast();
    EXPECT_EQ(bb.read(), expected);
}

TEST_F(BitboardFixture, ShiftsNorthWest_ExpectShiftsToWorkProperly)
{
    Bitboard bb(0x1e000000ULL);
    u64 expected = 0xf00000000ULL;

    bb = bb.shiftNorthWest();
    EXPECT_EQ(bb.read(), expected);
}

TEST_F(BitboardFixture, ShiftsSouthEast_ExpectShiftsToWorkProperly)
{
    Bitboard bb(0x1e000000ULL);
    u64 expected = 0x3c0000ULL;

    bb = bb.shiftSouthEast();
    EXPECT_EQ(bb.read(), expected);
}

TEST_F(BitboardFixture, ShiftsSouthWest_ExpectShiftsToWorkProperly)
{
    Bitboard bb(0x1e000000ULL);
    u64 expected = 0xf0000ULL;

    bb = bb.shiftSouthWest();
    EXPECT_EQ(bb.read(), expected);
}

TEST_F(BitboardFixture, ShiftNorthRelative_ShiftsAccordingToGivenSet)
{
    Bitboard bb(0x1e000000ULL);
    u64 expected = 0x1e0000ULL;

    bb = bb.shiftNorthRelative<Set::BLACK>();
    EXPECT_EQ(bb, expected);

    expected = 0x1e000000ULL;
    bb = bb.shiftNorthRelative<Set::WHITE>();
    EXPECT_EQ(bb, expected);
}

TEST_F(BitboardFixture, CombineBitobards_VariadicTemplateCombineMethod)
{
    Bitboard bb1(0x40200000000ull);
    Bitboard bb2(0x81000000000ull);
    Bitboard bb3(0x40200ull);

    Bitboard expected(0xc1200040200ull);

    Bitboard result = bb1.combine(bb1, bb2, bb3);
    EXPECT_EQ(result, expected);

    result = bb1.combine(bb2, bb3);
    expected = 0x81000040200ull;
    EXPECT_EQ(result, expected);
}

////////////////////////////////////////////////////////////////

}  // namespace ElephantTest