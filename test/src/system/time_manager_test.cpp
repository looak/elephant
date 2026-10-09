#include <gtest/gtest.h>

#include <search/search.hpp>
#include <system/time_manager.hpp>

#include <limits>

namespace ElephantTest {
/**
 * @file time_manager_test.cpp
 * @brief Fixture testing the time allocation of the TimeManager
 * Naming convention: <TestedFunction>_<Condition>_<ExpectedBehavior>
 * @author Alexander Loodin Ek *
 */
////////////////////////////////////////////////////////////////

TEST(TimeManagerTest, limits_IncrementLargerThanClock_NeverExceedClockMinusOverhead)
{
    // setup, 75% of the increment alone is far more than what is left on the clock.
    SearchParameters params;
    params.WhiteTimelimit = 500;
    params.WhiteTimeIncrement = 2000;
    TimeManager tm(params, Set::WHITE);
    tm.setMoveOverhead(50);

    // do
    tm.begin();

    // verify
    EXPECT_EQ(450u * c_maxClockUsage_pct / 100, tm.hardLimit());
    EXPECT_EQ(tm.hardLimit(), tm.softLimit());
}

TEST(TimeManagerTest, limits_ClockBelowOverhead_AllocateMinimalTime)
{
    // setup, the first iteration always completes so we still return a move.
    SearchParameters params;
    params.BlackTimelimit = 20;
    params.BlackTimeIncrement = 1000;
    TimeManager tm(params, Set::BLACK);
    tm.setMoveOverhead(50);

    // do
    tm.begin();

    // verify
    EXPECT_EQ(1u, tm.hardLimit());
    EXPECT_EQ(1u, tm.softLimit());
}

TEST(TimeManagerTest, limits_MoveTime_UseAllOfItMinusOverhead)
{
    // setup
    SearchParameters params;
    params.MoveTime = 1000;
    TimeManager tm(params, Set::WHITE);
    tm.setMoveOverhead(100);

    // do
    tm.begin();

    // verify
    EXPECT_EQ(900u, tm.hardLimit());
    EXPECT_EQ(900u, tm.softLimit());
}

TEST(TimeManagerTest, limits_PlentyOfTime_AreSharesOfTargetTime)
{
    // setup, the target is the clock spread over the default moves to go plus 75% of the increment.
    SearchParameters params;
    params.WhiteTimelimit = 60000;
    params.WhiteTimeIncrement = 1000;
    TimeManager tm(params, Set::WHITE);
    tm.setMoveOverhead(100);

    // do
    tm.begin();

    // verify
    const u64 target = 60000u / c_defaultMovesToGo + 750u;
    EXPECT_EQ(target * c_softLimit_pct / 100, tm.softLimit());
    EXPECT_EQ(target * c_hardLimitFactor, tm.hardLimit());
}

TEST(TimeManagerTest, limits_OneMoveToGo_KeepAShareOfTheClock)
{
    // setup
    SearchParameters params;
    params.WhiteTimelimit = 10000;
    params.MovesToGo = 1;
    TimeManager tm(params, Set::WHITE);
    tm.setMoveOverhead(10);

    // do
    tm.begin();

    // verify
    EXPECT_EQ(9990u * c_maxClockUsage_pct / 100, tm.hardLimit());
    EXPECT_EQ(10000u * c_softLimit_pct / 100, tm.softLimit());
}

TEST(TimeManagerTest, limits_DepthOnly_AreUnbounded)
{
    // setup
    SearchParameters params;
    params.SearchDepth = 5;
    TimeManager tm(params, Set::WHITE);

    // do
    tm.begin();

    // verify
    EXPECT_EQ(std::numeric_limits<u64>::max(), tm.hardLimit());
    EXPECT_EQ(std::numeric_limits<u64>::max(), tm.softLimit());
    EXPECT_TRUE(tm.continueIterativeDeepening());
}

TEST(TimeManagerTest, setMoveOverhead_AboveMaximum_IsClamped)
{
    // setup
    SearchParameters params;
    params.MoveTime = 10000;
    TimeManager tm(params, Set::WHITE);
    tm.setMoveOverhead(c_maxMoveOverhead_ms + 1000);

    // do
    tm.begin();

    // verify
    EXPECT_EQ(10000u - c_maxMoveOverhead_ms, tm.hardLimit());
}

}  // namespace ElephantTest
