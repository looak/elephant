#include <gtest/gtest.h>
#include <core/square_notation.hpp>

#include <sstream>

namespace ElephantTest {
////////////////////////////////////////////////////////////////

/**
 * @file square_notation_test.cpp
 * @brief A square's file, rank & index (square.hpp) and its algebraic name (square_notation.hpp).
 * Naming convention as of October 2023: <TestedFunctionality>_<ExpectedResult>  */

TEST(SquareNotationTest, FromSquare_FileRankAndIndex)
{
    EXPECT_EQ(0, to_file(Square::A1));
    EXPECT_EQ(0, to_rank(Square::A1));
    EXPECT_EQ(0, to_index(Square::A1));

    EXPECT_EQ(4, to_file(Square::E4));
    EXPECT_EQ(3, to_rank(Square::E4));
    EXPECT_EQ(28, to_index(Square::E4));

    EXPECT_EQ(7, to_file(Square::H8));
    EXPECT_EQ(7, to_rank(Square::H8));
    EXPECT_EQ(63, to_index(Square::H8));
}

TEST(SquareNotationTest, FromFileAndRank_SameSquare)
{
    EXPECT_EQ(Square::A1, to_square(0, 0));
    EXPECT_EQ(Square::E4, to_square(4, 3));
    EXPECT_EQ(Square::H1, to_square(7, 0));
    EXPECT_EQ(Square::A8, to_square(0, 7));
    EXPECT_EQ(Square::H8, to_square(7, 7));
}

TEST(SquareNotationTest, EverySquare_RoundTripsThroughFileAndRank)
{
    for (byte index = 0; index < 64; ++index) {
        const Square square = static_cast<Square>(index);
        EXPECT_EQ(index, to_index(square));
        EXPECT_EQ(index % 8, to_file(square));
        EXPECT_EQ(index / 8, to_rank(square));
        EXPECT_EQ(square, to_square(to_file(square), to_rank(square)));
    }
}

TEST(SquareNotationTest, ToString_AlgebraicName)
{
    EXPECT_EQ("a1", to_string(Square::A1));
    EXPECT_EQ("e4", to_string(Square::E4));
    EXPECT_EQ("h1", to_string(Square::H1));
    EXPECT_EQ("a8", to_string(Square::A8));
    EXPECT_EQ("h8", to_string(Square::H8));

    for (byte index = 0; index < 64; ++index) {
        const std::string name = to_string(static_cast<Square>(index));
        ASSERT_EQ(2u, name.size());
        EXPECT_EQ('a' + index % 8, name[0]);
        EXPECT_EQ('1' + index / 8, name[1]);
    }
}

TEST(SquareNotationTest, StreamOperator_WritesTheAlgebraicName)
{
    std::ostringstream stream;
    stream << Square::F7 << ' ' << Square::B2;
    EXPECT_EQ("f7 b2", stream.str());
}

} // namespace ElephantTest
