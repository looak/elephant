#include <gtest/gtest.h>
#include <core/square_notation.hpp>

#include <sstream>

namespace ElephantTest {
////////////////////////////////////////////////////////////////

/**
 * @file square_notation_test.cpp
 * @brief SquareNotation, a square's file, rank, index & algebraic name.
 * Naming convention as of October 2023: <TestedFunctionality>_<ExpectedResult>  */

TEST(SquareNotationTest, FromSquare_FileRankAndIndex)
{
    SquareNotation a1(Square::A1);
    EXPECT_EQ(0, a1.file());
    EXPECT_EQ(0, a1.rank());
    EXPECT_EQ(0, a1.index());

    SquareNotation e4(Square::E4);
    EXPECT_EQ(4, e4.file());
    EXPECT_EQ(3, e4.rank());
    EXPECT_EQ(28, e4.index());

    SquareNotation h8(Square::H8);
    EXPECT_EQ(7, h8.file());
    EXPECT_EQ(7, h8.rank());
    EXPECT_EQ(63, h8.index());
}

TEST(SquareNotationTest, FromFileAndRank_SameSquare)
{
    EXPECT_EQ(Square::A1, SquareNotation(0, 0).toSquare());
    EXPECT_EQ(Square::E4, SquareNotation(4, 3).toSquare());
    EXPECT_EQ(Square::H1, SquareNotation(7, 0).toSquare());
    EXPECT_EQ(Square::A8, SquareNotation(0, 7).toSquare());
    EXPECT_EQ(Square::H8, SquareNotation(7, 7).toSquare());
}

TEST(SquareNotationTest, EverySquare_RoundTripsThroughFileAndRank)
{
    for (byte index = 0; index < 64; ++index) {
        const Square square = static_cast<Square>(index);
        SquareNotation notation(square);
        EXPECT_EQ(index, notation.index());
        EXPECT_EQ(index % 8, notation.file());
        EXPECT_EQ(index / 8, notation.rank());
        EXPECT_EQ(square, SquareNotation(notation.file(), notation.rank()).toSquare());
    }
}

TEST(SquareNotationTest, ToString_AlgebraicName)
{
    EXPECT_EQ("a1", SquareNotation(Square::A1).toString());
    EXPECT_EQ("e4", SquareNotation(Square::E4).toString());
    EXPECT_EQ("h1", SquareNotation(Square::H1).toString());
    EXPECT_EQ("a8", SquareNotation(Square::A8).toString());
    EXPECT_EQ("h8", SquareNotation(Square::H8).toString());

    for (byte index = 0; index < 64; ++index) {
        const std::string name = SquareNotation(static_cast<Square>(index)).toString();
        ASSERT_EQ(2u, name.size());
        EXPECT_EQ('a' + index % 8, name[0]);
        EXPECT_EQ('1' + index / 8, name[1]);
    }
}

TEST(SquareNotationTest, Equality_ComparesSquares)
{
    EXPECT_EQ(SquareNotation(Square::E4), SquareNotation(4, 3));
    EXPECT_TRUE(SquareNotation(Square::E4) == Square::E4);
    EXPECT_FALSE(SquareNotation(Square::E4) == SquareNotation(Square::E5));
    EXPECT_FALSE(SquareNotation(Square::D4) == Square::E4);

    SquareNotation copy = SquareNotation(Square::C7);
    copy = SquareNotation(Square::G2);
    EXPECT_EQ(Square::G2, copy.toSquare());
    EXPECT_EQ(6, copy.file());
    EXPECT_EQ(1, copy.rank());
}

TEST(SquareNotationTest, StreamOperators_WriteTheAlgebraicName)
{
    std::ostringstream fromSquare;
    fromSquare << Square::F7;
    EXPECT_EQ("f7", fromSquare.str());

    std::ostringstream fromNotation;
    fromNotation << SquareNotation(Square::B2);
    EXPECT_EQ("b2", fromNotation.str());
}

} // namespace ElephantTest
