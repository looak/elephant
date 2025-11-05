#include <gtest/gtest.h>
#include "transposition_table.hpp"
#include "search_constants.hpp"

namespace ElephantTest {

TEST(TranspositionTest, SizeAndResize) {
    TranspositionTable table;
    EXPECT_EQ(table.readSize(), 1024 * 1024 * 8 / 16);
    EXPECT_EQ(table.readSizeMegaBytes(), 8);
}

TEST(TranspositionTest, CalculateIndexEntry) {
    TranspositionTable table;
    u64 hash = 0x1234567890abcdef;
    u64 index = table.entryIndex(hash);

    //u64 expected = ((i128)hash * (i128)(1024 * 1024 * 8 / 16)) >> 64;
    u64 expected = hash & ((1024 * 1024 * 8 / 16) - 1);

    EXPECT_EQ(expected, index);

    u64 otherHash = 0xabcdef1234567890;
    u64 otherIndex = table.entryIndex(otherHash);
    EXPECT_NE(index, otherIndex);
}

TEST(TranspositionTest, ReadEntryOfGivenHash_EmptyResult) {
    TranspositionTable table;
    u64 hash = 0x1234567890abcdef;
    const TranspositionEntry& entry = table.readEntry(hash);

    EXPECT_EQ(0, entry.hash);
    EXPECT_EQ(PackedMove::NullMove(), entry.move);
    EXPECT_EQ(0, entry.score);
    EXPECT_EQ(0, entry.depth);
    EXPECT_EQ(0, entry.age);
    EXPECT_FALSE(entry.valid());
}

TEST(TranspositionTest, EditEntryOfGivenHash_ReadModifiedEntry) {
    TranspositionTableImpl<u64> table;

    u64 hash = 0x1234567890abcdef;
    {
        u64& entry = table.editEntry(hash);
        entry = hash;
    }

    u64 stored = table.readEntry(hash);
    EXPECT_EQ(hash, stored);
}

// I don't quite understand the purpose of this yet - leaving it for now.
TEST(TranspositionEntryTest, DISABLED_CorrectingCheckmateScore) {
    EXPECT_TRUE(false);
}

TEST(TranspositionEntryTest, Flags)
{
    TranspositionEntry entry;
    entry.flag = TranspositionFlag::TTF_CUT_EXACT;
    EXPECT_TRUE(entry.exact());
    EXPECT_FALSE(entry.beta());
    EXPECT_FALSE(entry.alpha());

    entry.flag = TranspositionFlag::TTF_CUT_BETA;
    EXPECT_FALSE(entry.exact());
    EXPECT_TRUE(entry.beta());
    EXPECT_FALSE(entry.alpha());

    entry.flag = TranspositionFlag::TTF_CUT_ALPHA;
    EXPECT_FALSE(entry.exact());
    EXPECT_FALSE(entry.beta());
    EXPECT_TRUE(entry.alpha());
}

} // namespace ElephantTest
