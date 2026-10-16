#include <gtest/gtest.h>

#include <io/weight_store.hpp>

#include <vector>
#include <string>

namespace ElephantTest {

////////////////////////////////////////////////////////////////

// Mock for FileReader
class MockFileReader : public FileReader {
public:
    bool openFile(const std::string&) override {
        return true;
    }

    bool readLine(std::string& out) override {
        if (line < lines.size()) {
            out = lines[line++];
            return true;
        }
        return false;
    }

    size_t line = 0;
    std::vector<std::string> lines = {
        "w3 10",
        "weight2 1.5",
        ""
    };
};

////////////////////////////////////////////////////////////////

class WeightFixture : public ::testing::Test {
public:
    virtual void SetUp() {
        WeightStore::get()->initialize(new MockFileReader());
    };
    virtual void TearDown() {};
};

////////////////////////////////////////////////////////////////


// TUNABLE_ so they are booked in the store whether or not the build is ELEPHANT_TUNABLE.
TUNABLE_WEIGHT(weight1, i64, 1)
TUNABLE_MULTIPLIER(weight2, 0)
TUNABLE_WEIGHT(w3, i64, 1)

TEST_F(WeightFixture, StoreHasWeight2) {

    EXPECT_EQ(1, weight1);
    WeightStore::get()->update("weight1", "20");
    EXPECT_EQ(20, weight1);
}

TEST_F(WeightFixture, StoreHasWeight1) {
    EXPECT_EQ(1, w3);
    EXPECT_EQ(0, weight2);
    WeightStore::get()->loadFromFile("test");

    EXPECT_EQ(10, w3);
    EXPECT_EQ(1536, weight2);  // 1.5 * 1024 (fixed-point representation)
}

TEST_F(WeightFixture, UnknownWeightInFile_ExpectErrorButContinue) {
    MockFileReader reader;
    reader.lines = {
        "unknown 42",
        "w3 11",
        ""
    };
    WeightStore::get()->initialize(&reader);
    WeightStore::get()->loadFromFile("test");

    EXPECT_EQ(11, w3);
}

TUNABLE_TAPERED_WEIGHT(testTaperedWeight, i64, 1, 10)

TEST_F(WeightFixture, TaperedWeight_UpdatedValues) {
    // midgame at full phase, endgame at 0.
    EXPECT_EQ(1, testTaperedWeight * evaluator_data::maxGamePhase);
    EXPECT_EQ(10, testTaperedWeight * 0);
    EXPECT_EQ(10, weight_testTaperedWeight * 0);
    WeightStore::get()->update("testTaperedWeight", "5 15");
    EXPECT_EQ(5, testTaperedWeight * evaluator_data::maxGamePhase);
    EXPECT_EQ(15, testTaperedWeight * 0);
}

constexpr i32 testTable_mg[3] = { 1, 2, 3 };
constexpr i32 testTable_eg[3] = { 10, 20, 30 };
TUNABLE_TAPERED_TABLE(testTable, i32, testTable_mg, testTable_eg)

TEST_F(WeightFixture, TaperedTable_ZippedFromPublishedTables) {
    EXPECT_EQ((Tapered<i32>{ 2, 20 }), testTable[1]);
}

TEST_F(WeightFixture, TaperedTable_FileLineSetsMidgameThenEndgame) {
    MockFileReader reader;
    reader.lines = {
        "testTable 4 5 6   40 50 60  ",
        ""
    };
    WeightStore::get()->initialize(&reader);
    WeightStore::get()->loadFromFile("test");

    EXPECT_EQ((Tapered<i32>{ 4, 40 }), testTable[0]);
    EXPECT_EQ((Tapered<i32>{ 6, 60 }), testTable[2]);
}

TEST_F(WeightFixture, TaperedTable_TooFewValues_TableUnchanged) {
    WeightStore::get()->update("testTable", "7 8 9 70 80 90");
    EXPECT_THROW(WeightStore::get()->update("testTable", "1 2 3 10 20"), ephant::io_error);
    EXPECT_EQ((Tapered<i32>{ 7, 70 }), testTable[0]);
}

TEST(Taper, MidgameAtFullPhase_EndgameAtZero_LinearBetween) {
    constexpr Tapered<i32> value{ 100, 300 };
    static_assert(value * evaluator_data::maxGamePhase == 100);
    static_assert(value * 0 == 300);
    static_assert(value * (evaluator_data::maxGamePhase / 2) == 200);
    // promotions push the phase past the maximum, it's capped there.
    static_assert(value * (evaluator_data::maxGamePhase + 4) == 100);
    EXPECT_EQ(evaluator_data::taper(100, 300, 6), value * 6);
}

}  // namespace ElephantTest