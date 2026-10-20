#include <gtest/gtest.h>

#include <input.hpp>

#include <string>
#include <string_view>

namespace ElephantTest {
////////////////////////////////////////////////////////////////

/**
 * @file input_test.cpp
 * @brief Testing phant-indexer's input: PGN plain, bz2 or gzip, told by its first bytes. test/indexer_smoke.py reads
 * every fixture compressed both ways too.
 * Naming convention as of October 2023: <TestedFunctionality>_<ExpectedResult>
 * @author Alexander Loodin Ek  */

// "1. e4 *\n", by python's bz2.compress & gzip.compress.
static const char c_bz2[] = "\x42\x5a\x68\x39\x31\x41\x59\x26\x53\x59\x58\x47\x6c\xa2\x00\x00\x02\xd9\x00\x00\x10\x40"
                            "\x11\x24\x00\x02\x00\x20\x00\x22\x1a\x66\xa6\x21\x80\xe2\x40\xf7\x45\xdc\x91\x4e\x14\x24"
                            "\x16\x11\xdb\x28\x80";
static const char c_gzip[] = "\x1f\x8b\x08\x00\x00\x00\x00\x00\x02\xff\x33\xd4\x53\x48\x35\x51\xd0\xe2\x02\x00\x60\x3f"
                             "\x03\x2d\x08\x00\x00\x00";
static const std::string_view c_bz2Bytes(c_bz2, sizeof(c_bz2) - 1);
static const std::string_view c_gzipBytes(c_gzip, sizeof(c_gzip) - 1);

////////////////////////////////////////////////////////////////

TEST(IndexerInput, Detect_ByTheFirstBytes)
{
    EXPECT_EQ(indexer::Compression::Bzip2, indexer::detect(c_bz2Bytes));
    EXPECT_EQ(indexer::Compression::Gzip, indexer::detect(c_gzipBytes));
    EXPECT_EQ(indexer::Compression::None, indexer::detect("[Event \"BZh\"]"));
    EXPECT_EQ(indexer::Compression::None, indexer::detect("BZh"));
    EXPECT_EQ(indexer::Compression::None, indexer::detect(""));
}

TEST(IndexerInput, Decompress_EachWay)
{
    for (std::string_view bytes : { c_bz2Bytes, c_gzipBytes, std::string_view("1. e4 *\n") }) {
        std::string out, error;
        EXPECT_TRUE(indexer::decompress(bytes, indexer::detect(bytes), out, error)) << error;
        EXPECT_EQ("1. e4 *\n", out);
    }
}

// cutechess-ob's copies are compressed one after the other, a stream each.
TEST(IndexerInput, ConcatenatedStreams_AllRead)
{
    for (std::string_view bytes : { c_bz2Bytes, c_gzipBytes }) {
        const std::string twice = std::string(bytes) + std::string(bytes);
        std::string out, error;
        EXPECT_TRUE(indexer::decompress(twice, indexer::detect(twice), out, error)) << error;
        EXPECT_EQ("1. e4 *\n1. e4 *\n", out);
    }
}

TEST(IndexerInput, Truncated_IsAnError)
{
    for (std::string_view bytes : { c_bz2Bytes, c_gzipBytes }) {
        const std::string_view cut = bytes.substr(0, bytes.size() - 6);
        std::string out, error;
        EXPECT_FALSE(indexer::decompress(cut, indexer::detect(cut), out, error));
        EXPECT_FALSE(error.empty());
    }
}

} // namespace ElephantTest
