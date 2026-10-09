#include <gtest/gtest.h>

#include <search/search.hpp>
#include <system/time_manager.hpp>

namespace ElephantTest {
/**
 * @file time_manager_test.cpp
 * @brief Fixture testing the time allocation of the TimeManager
 * Naming convention: <TestedFunction>_<Condition>_<ExpectedBehavior>
 * @author Alexander Loodin Ek *
 */
////////////////////////////////////////////////////////////////

TEST(TimeManagerTest, allocatedTime_IncrementLargerThanClock_NeverExceedsClockMinusOverhead)
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
    EXPECT_LE(tm.allocatedTime(), 450u);
}

TEST(TimeManagerTest, allocatedTime_ClockBelowOverhead_AllocatesMinimalTime)
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
    EXPECT_EQ(1u, tm.allocatedTime());
}

TEST(TimeManagerTest, allocatedTime_MoveTime_OverheadIsSubtracted)
{
    // setup
    SearchParameters params;
    params.MoveTime = 1000;
    TimeManager tm(params, Set::WHITE);
    tm.setMoveOverhead(100);

    // do
    tm.begin();

    // verify
    EXPECT_EQ(900u, tm.allocatedTime());
}

TEST(TimeManagerTest, allocatedTime_PlentyOfTime_OverheadDoesNotChangeAllocation)
{
    // setup, 1/24th of the clock plus 75% of the increment, minus the 2% margin.
    SearchParameters params;
    params.WhiteTimelimit = 60000;
    params.WhiteTimeIncrement = 1000;
    TimeManager tm(params, Set::WHITE);
    tm.setMoveOverhead(100);

    // do
    tm.begin();

    // verify
    EXPECT_EQ((60000u / 24 + 750u) * 98 / 100, tm.allocatedTime());
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
    EXPECT_EQ(10000u - c_maxMoveOverhead_ms, tm.allocatedTime());
}

}  // namespace ElephantTest
