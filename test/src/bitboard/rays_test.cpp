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