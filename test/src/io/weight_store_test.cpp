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
        "weight2 2",
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


WEIGHT(weight1, i64, 1);
MULTIPLIER(weight2, 2);
WEIGHT(w3, i64, 1);

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
    EXPECT_EQ(3.14, weight2);
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

TAPERED_WEIGHT(testTaperedWeight, i64, 1, 10);

TEST_F(WeightFixture, TaperedWeight_UpdatedValues) {
    EXPECT_EQ(1, testTaperedWeight * 0.f);
    EXPECT_EQ(10, testTaperedWeight * 1.f);
    WeightStore::get()->update("testTaperedWeight", "5 15");
    EXPECT_EQ(5, testTaperedWeight * 0.f);
    EXPECT_EQ(15, testTaperedWeight * 1.f);
}

}  // namespace ElephantTest