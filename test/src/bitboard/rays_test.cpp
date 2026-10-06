#include <gtest/gtest.h>

#include <bitboard/attacks/attacks.hpp>
#include <bitboard/rays/rays.hpp>
#include <core/square.hpp>

// class RaysTest : public ::testing::Test {
// protected:
//     void SetUp() override {
//         attacks::tables atable;
//     }

//     void TearDown() override {
//     }
// };



TEST(RaysTest, Rays) {
    Square from = Square::A1;
    Square to = Square::H8;

    u64 expected = 0x8040201008040200;

    u64 result = ray::getRay(*from, *to);
    EXPECT_EQ(expected, result);
}

TEST(RaysTest, LineDiagonal) {
    // a1-h8 diagonal, from two squares in the middle of it.
    u64 expected = 0x8040201008040201;
    EXPECT_EQ(expected, ray::getLine(*Square::C3, *Square::F6));
    EXPECT_EQ(expected, ray::getLine(*Square::F6, *Square::C3));
}

TEST(RaysTest, LineAntiDiagonal) {
    // h1-a8 anti diagonal.
    u64 expected = 0x0102040810204080;
    EXPECT_EQ(expected, ray::getLine(*Square::G2, *Square::D5));
}

TEST(RaysTest, LineFile) {
    // full e-file.
    u64 expected = 0x1010101010101010;
    EXPECT_EQ(expected, ray::getLine(*Square::E1, *Square::E2));
    EXPECT_EQ(expected, ray::getLine(*Square::E8, *Square::E1));
}

TEST(RaysTest, LineRank) {
    // full 4th rank.
    u64 expected = 0x00000000FF000000;
    EXPECT_EQ(expected, ray::getLine(*Square::B4, *Square::G4));
}

TEST(RaysTest, LineNotAligned) {
    EXPECT_EQ(0ull, ray::getLine(*Square::A1, *Square::B3));
    EXPECT_EQ(0ull, ray::getLine(*Square::E1, *Square::E1));
}