#include <gtest/gtest.h>

#include <search/bench.hpp>

namespace ElephantTest {
/**
 * @file bench_test.cpp
 * @brief What makes the bench a signature: nothing carries over between its positions. The number itself changes with
 * every search change, the commit hook records it.
 * Naming convention: <TestedFunction>_<Condition>_<ExpectedBehavior>    */
////////////////////////////////////////////////////////////////

namespace {
const std::string c_first = "r3k2r/2pb1ppp/2pp1q2/p7/1nP1B3/1P2P3/P2N1PPP/R2QK2R w KQkq a6 0 14";
const std::string c_second = "4rrk1/2p1b1p1/p1p3q1/4p3/2P2n1p/1P1NR2P/PB3PP1/3R1QK1 b - - 2 24";

u64 nodesOf(const std::vector<std::string>& fens)
{
    return bench::run(fens, 5, 1, noSearchListener()).nodes;
}
} // namespace

TEST(BenchTest, Run_PositionsDoNotAffectEachOther)
{
    const u64 first = nodesOf({ c_first });
    const u64 second = nodesOf({ c_second });

    EXPECT_GT(first, 0u);
    EXPECT_GT(second, 0u);
    EXPECT_EQ(first + second, nodesOf({ c_first, c_second }));
    EXPECT_EQ(first + second, nodesOf({ c_second, c_first }));
}

TEST(BenchTest, Run_FenThatDoesNotRead_IsSkipped)
{
    EXPECT_EQ(nodesOf({ c_first }), nodesOf({ "not a fen", c_first }));
    EXPECT_EQ(0u, nodesOf({ "not a fen" }));
}

TEST(BenchTest, Nps_UnderAMillisecond_CountsAsOne)
{
    const bench::Result result{ 500, 0 };
    EXPECT_EQ(500000u, result.nps());
}

////////////////////////////////////////////////////////////////

}  // namespace ElephantTest
