#include <gtest/gtest.h>
#include <search/transposition_table.hpp>
#include <search/search_constants.hpp>

namespace ElephantTest {

namespace {
// positions sharing a bucket: the high bits pick the bucket, the low 16 bits are the key. With the low 32 bits clear
// the key can't move the hash into the next bucket.
constexpr u64 bucketBits = 0x1234567800000000ull;
constexpr u64 inBucket(u16 key) { return bucketBits | key; }

struct ProbeResult {
    bool hit = false;
    PackedMove move = PackedMove::NullMove();
    i16 score = 0;
    u8 depth = 0;
    TranspositionFlag bound = TTF_NONE;
};

ProbeResult probe(TranspositionTable& table, u64 hash) {
    ProbeResult result;
    result.hit = table.probe(hash, result.move, result.score, result.depth, result.bound);
    return result;
}
} // namespace

TEST(TranspositionTest, Size_BucketsOfEightEntriesFillTheMegabytes) {
    TranspositionTable table(8);
    EXPECT_EQ(8u * 1024 * 1024, table.size_bytes());
    EXPECT_EQ(8u * 1024 * 1024 / 8, table.size());
    EXPECT_EQ(8u, table.size_mb());
}

TEST(TranspositionTest, StoreThenProbe_ReturnsTheStoredEntry) {
    TranspositionTable table(1);
    const u64 hash = 0x1234567890abcdef;
    const PackedMove move(Square::E2, Square::E4);

    table.store(hash, move, -123, 7, TTF_CUT_BETA);
    ProbeResult result = probe(table, hash);

    EXPECT_TRUE(result.hit);
    EXPECT_EQ(move, result.move);
    EXPECT_EQ(-123, result.score);
    EXPECT_EQ(7, result.depth);
    EXPECT_EQ(TTF_CUT_BETA, result.bound);
}

TEST(TranspositionTest, Probe_EmptyTableAndOtherKeyMiss) {
    TranspositionTable table(1);
    EXPECT_FALSE(probe(table, inBucket(1)).hit);

    table.store(inBucket(1), PackedMove(Square::E2, Square::E4), 10, 3, TTF_CUT_EXACT);
    EXPECT_FALSE(probe(table, inBucket(2)).hit);
}

TEST(TranspositionTest, Bucket_HoldsEightPositions) {
    TranspositionTable table(1);
    for (u16 key = 1; key <= 8; ++key)
        table.store(inBucket(key), PackedMove(Square::E2, Square::E4), key, static_cast<u8>(key), TTF_CUT_EXACT);

    for (u16 key = 1; key <= 8; ++key) {
        ProbeResult result = probe(table, inBucket(key));
        EXPECT_TRUE(result.hit) << "key " << key;
        EXPECT_EQ(key, result.score);
    }
}

TEST(TranspositionTest, FullBucket_ReplacesTheShallowestEntry) {
    TranspositionTable table(1);
    // depths 10..17, key 4 is the shallowest at depth 2.
    for (u16 key = 1; key <= 8; ++key) {
        u8 depth = key == 4 ? 2 : static_cast<u8>(9 + key);
        table.store(inBucket(key), PackedMove(Square::E2, Square::E4), 0, depth, TTF_CUT_EXACT);
    }

    table.store(inBucket(9), PackedMove(Square::D2, Square::D4), 0, 1, TTF_CUT_ALPHA);

    EXPECT_TRUE(probe(table, inBucket(9)).hit);
    EXPECT_FALSE(probe(table, inBucket(4)).hit);
    for (u16 key : {u16{1}, u16{2}, u16{3}, u16{5}, u16{6}, u16{7}, u16{8}})
        EXPECT_TRUE(probe(table, inBucket(key)).hit) << "key " << key;
}

TEST(TranspositionTest, FullBucket_ReplacesEntriesFromOlderSearchesFirst) {
    TranspositionTable table(1);
    // key 1 is the deepest but two searches old.
    table.store(inBucket(1), PackedMove(Square::E2, Square::E4), 0, 20, TTF_CUT_EXACT);
    table.incrementAge();
    table.incrementAge();
    for (u16 key = 2; key <= 8; ++key)
        table.store(inBucket(key), PackedMove(Square::E2, Square::E4), 0, 6, TTF_CUT_EXACT);

    table.store(inBucket(9), PackedMove(Square::D2, Square::D4), 0, 1, TTF_CUT_ALPHA);

    EXPECT_TRUE(probe(table, inBucket(9)).hit);
    EXPECT_FALSE(probe(table, inBucket(1)).hit);
}

TEST(TranspositionTest, SamePosition_FailLowKeepsTheStoredMove) {
    TranspositionTable table(1);
    const u64 hash = inBucket(1);
    const PackedMove move(Square::G1, Square::F3);

    table.store(hash, move, 50, 4, TTF_CUT_BETA);
    table.store(hash, PackedMove::NullMove(), -20, 5, TTF_CUT_ALPHA);

    ProbeResult result = probe(table, hash);
    EXPECT_EQ(move, result.move);
    EXPECT_EQ(-20, result.score);
    EXPECT_EQ(5, result.depth);
    EXPECT_EQ(TTF_CUT_ALPHA, result.bound);
}

TEST(TranspositionTest, SamePosition_NewestResultReplaces) {
    TranspositionTable table(1);
    const u64 hash = inBucket(1);

    table.store(hash, PackedMove(Square::G1, Square::F3), 50, 12, TTF_CUT_BETA);
    table.store(hash, PackedMove(Square::E2, Square::E4), 10, 3, TTF_CUT_ALPHA);

    ProbeResult result = probe(table, hash);
    EXPECT_EQ(PackedMove(Square::E2, Square::E4), result.move);
    EXPECT_EQ(10, result.score);
    EXPECT_EQ(3, result.depth);
    EXPECT_EQ(TTF_CUT_ALPHA, result.bound);

    // the position has one entry, a different one still fits in the bucket's other slots.
    table.store(inBucket(2), PackedMove(Square::D2, Square::D4), 0, 1, TTF_CUT_EXACT);
    EXPECT_TRUE(probe(table, inBucket(2)).hit);
}

TEST(TranspositionTest, MateScores_RoundTrip) {
    TranspositionTable table(1);
    const i16 mate = static_cast<i16>(c_checkmateConstant - 5);

    table.store(inBucket(1), PackedMove(Square::E2, Square::E4), mate, 9, TTF_CUT_EXACT);
    table.store(inBucket(2), PackedMove(Square::E2, Square::E4), -mate, 9, TTF_CUT_EXACT);

    EXPECT_EQ(mate, probe(table, inBucket(1)).score);
    EXPECT_EQ(-mate, probe(table, inBucket(2)).score);
}

TEST(TranspositionTest, Clear_EmptiesTheTableAndResetsAge) {
    TranspositionTable table(1);
    table.incrementAge();
    table.store(inBucket(1), PackedMove(Square::E2, Square::E4), 10, 3, TTF_CUT_EXACT);

    table.clear();

    EXPECT_FALSE(probe(table, inBucket(1)).hit);
    EXPECT_EQ(0, table.getAge());
}

TEST(TranspositionTest, Size_AnyMegabyteCountIsUsed) {
    TranspositionTable table(100);
    EXPECT_EQ(100u * 1024 * 1024, table.size_bytes());
    EXPECT_EQ(100u * 1024 * 1024 / 8, table.size());

    table.resize(3);
    EXPECT_EQ(3u * 1024 * 1024, table.size_bytes());
}

TEST(TranspositionTest, Resize_StoresAndProbesAcrossTheWholeTable) {
    TranspositionTable table(3);
    // hashes spread over the 64-bit range land in every part of a table that isn't a power of two.
    for (u64 i = 0; i < 64; ++i) {
        const u64 hash = (i * 0x0400000000000000ull) | 0x5A5A;
        table.store(hash, PackedMove(Square::E2, Square::E4), static_cast<i16>(i), 5, TTF_CUT_EXACT);
    }
    for (u64 i = 0; i < 64; ++i) {
        const u64 hash = (i * 0x0400000000000000ull) | 0x5A5A;
        ProbeResult result = probe(table, hash);
        EXPECT_TRUE(result.hit) << "i " << i;
        EXPECT_EQ(static_cast<i16>(i), result.score);
    }
}

TEST(TranspositionTest, Clear_WithThreadsEmptiesEveryBucket) {
    TranspositionTable table(5);
    for (u64 i = 0; i < 64; ++i)
        table.store((i * 0x0400000000000000ull) | 0x77, PackedMove(Square::E2, Square::E4), 1, 3, TTF_CUT_EXACT);

    table.clear(7);

    for (u64 i = 0; i < 64; ++i)
        EXPECT_FALSE(probe(table, (i * 0x0400000000000000ull) | 0x77).hit) << "i " << i;
    EXPECT_EQ(0u, table.hashfull());
}

TEST(TranspositionTest, Hashfull_CountsEntriesOfTheCurrentSearch) {
    TranspositionTable table(1);
    table.incrementAge();
    EXPECT_EQ(0u, table.hashfull());

    // the first bucket, hash 0 up to the first bucket boundary, every key a different position.
    for (u16 key = 1; key <= 8; ++key)
        table.store(key, PackedMove(Square::E2, Square::E4), 0, 4, TTF_CUT_EXACT);
    EXPECT_EQ(8u, table.hashfull());

    // entries of an earlier search don't count.
    table.incrementAge();
    EXPECT_EQ(0u, table.hashfull());
}

} // namespace ElephantTest
