#include <gtest/gtest.h>

#include <game_index.hpp>
#include <io/pgn_parser.hpp>

#include <format>
#include <string>

namespace ElephantTest {
////////////////////////////////////////////////////////////////

/**
 * @file game_index_test.cpp
 * @brief Testing phant-indexer's entries & their JSON, the contract with nyayo. test/indexer_smoke.py compares them
 * with python-chess over many games, these pin the format & the facts on games picked for them.
 * Naming convention as of October 2023: <TestedFunctionality>_<ExpectedResult>
 * @author Alexander Loodin Ek  */
class GameIndexFixture : public ::testing::Test {
public:
    // the stream's first game, indexed.
    indexer::GameIndex index(const std::string& pgn, bool verbose = false)
    {
        m_stream = pgn;
        io::pgn_parser::GameReader reader(m_stream);
        std::optional<io::pgn_parser::Game> game = reader.next();
        EXPECT_TRUE(game);
        return game ? indexer::index(*game, 1, verbose) : indexer::GameIndex{};
    }

    std::string m_stream;
};

static const std::string c_foolsMate = "[Event \"t\"]\n[Result \"0-1\"]\n\n1. f3 e5 2. g4 Qh4# 0-1\n";

////////////////////////////////////////////////////////////////

TEST_F(GameIndexFixture, FoolsMate_JsonLine)
{
    indexer::GameIndex entry = index(c_foolsMate);

    // the game's length runs through its result, not the newline after it.
    const std::string expected = std::format(
        "{{\"indexer\":\"1\",\"game\":1,\"offset\":0,\"length\":{},\"ok\":true,"
        "\"tags\":{{\"Event\":\"t\",\"Result\":\"0-1\"}},\"result\":\"0-1\",\"plies\":4,\"ended\":\"checkmate\","
        "\"promotions\":{{\"q\":0,\"r\":0,\"b\":0,\"n\":0}},\"castled\":{{\"white\":null,\"black\":null}},"
        "\"captures\":0,\"material\":{{\"end\":0}}}}",
        c_foolsMate.size() - 1);
    EXPECT_EQ(expected, indexer::toJson(entry));
}

TEST_F(GameIndexFixture, Verbose_AddsMovesAndFinalPosition)
{
    indexer::GameIndex entry = index(c_foolsMate, true);

    const std::string json = indexer::toJson(entry, true);
    EXPECT_TRUE(json.ends_with(",\"moves\":[\"f2f3\",\"e7e5\",\"g2g4\",\"d8h4\"],"
                               "\"fen\":\"rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3\"}"))
        << json;
    // and only when verbose.
    EXPECT_EQ(std::string::npos, indexer::toJson(entry, false).find("moves"));
}

TEST_F(GameIndexFixture, Source_WrittenWhenSet)
{
    indexer::GameIndex entry = index(c_foolsMate);
    entry.source = "batches/7.pgn";

    EXPECT_TRUE(indexer::toJson(entry).starts_with("{\"indexer\":\"1\",\"source\":\"batches/7.pgn\",\"game\":1,"));
}

// two pawns could take on d8, the e-pawn does and becomes a knight, the c-pawn queens.
TEST_F(GameIndexFixture, PromotionsCapturesAndMaterial)
{
    indexer::GameIndex entry = index("[FEN \"3r3k/2P1P3/8/8/8/8/8/K7 w - - 0 1\"]\n\n1. exd8=N Kg7 2. c8=Q 1-0");

    EXPECT_TRUE(entry.ok);
    EXPECT_EQ(3u, entry.plies);
    EXPECT_EQ(1u, entry.captures);
    EXPECT_EQ((std::array<u32, 4>{ 1, 0, 0, 1 }), entry.promotions);
    EXPECT_EQ("1-0", entry.result);  // no Result tag, the movetext's
    ASSERT_EQ(1u, entry.material.size());
    EXPECT_EQ("end", entry.material[0].first);
    EXPECT_EQ(12, entry.material[0].second);
}

// the plies are counted in the game, from its FEN, not by the FEN's move number.
TEST_F(GameIndexFixture, Castling_PlyOfEachSide)
{
    indexer::GameIndex entry = index(
        "[FEN \"r3k2r/pppq1ppp/2npbn2/2b1p3/2B1P3/2NPBN2/PPPQ1PPP/R3K2R b KQkq - 4 8\"]\n\n8... 0-0-0 9. O-O h6 *");

    EXPECT_TRUE(entry.ok);
    EXPECT_EQ(1u, entry.castledBlack);
    EXPECT_EQ(2u, entry.castledWhite);
    EXPECT_NE(std::string::npos, indexer::toJson(entry).find("\"castled\":{\"white\":2,\"black\":1}"));
}

TEST_F(GameIndexFixture, Endings)
{
    struct TestCase {
        std::string pgn;
        indexer::Ending expected;
    };
    using enum indexer::Ending;
    const std::vector<TestCase> testCases = {
        { c_foolsMate, Checkmate },
        { "[FEN \"k7/8/8/1Q6/8/8/8/7K w - - 0 1\"]\n\n1. Qb6 1/2-1/2", Stalemate },
        { "[FEN \"4k3/8/8/8/8/8/3p4/4K3 w - - 0 1\"]\n\n1. Kxd2 1/2-1/2", InsufficientMaterial },
        { "[FEN \"8/8/8/4k3/8/8/8/R3K3 w - - 99 80\"]\n\n80. Ra2 1/2-1/2", FiftyMoves },
        { "1. Nf3 Nf6 2. Ng1 Ng8 3. Nf3 Nf6 4. Ng1 Ng8 1/2-1/2", Repetition },
        { "1. Nf3 Nf6 2. Ng1 Ng8 3. Nf3 Nf6 4. Ng1 1/2-1/2", None },
        { "1. e4 1-0", None },
    };

    for (const auto& testCase : testCases) {
        indexer::GameIndex entry = index(testCase.pgn);
        EXPECT_TRUE(entry.ok) << testCase.pgn;
        EXPECT_EQ(indexer::toString(testCase.expected), indexer::toString(entry.ending)) << testCase.pgn;
    }
}

TEST_F(GameIndexFixture, MaterialAtFixedPlies)
{
    std::string shuffle;
    for (int move = 1; move <= 10; ++move)
        shuffle += std::format("{}. Nf3 Nf6 {}. Ng1 Ng8 ", 2 * move - 1, 2 * move);

    indexer::GameIndex entry = index(shuffle + "1. e4 *");  // a stray move number, the move after it is the 41st ply
    ASSERT_TRUE(entry.ok);
    EXPECT_EQ(41u, entry.plies);
    EXPECT_NE(std::string::npos, indexer::toJson(entry).find("\"material\":{\"20\":0,\"40\":0,\"end\":0}"));
}

TEST_F(GameIndexFixture, IllegalMove_ReportsItsPly)
{
    indexer::GameIndex entry = index("[Result \"*\"]\n\n1. e4 e5 2. Ke2 Qh4 3. Kf3 Qf4 4. Ke3 *");

    EXPECT_FALSE(entry.ok);
    EXPECT_EQ(6u, entry.plies);
    EXPECT_EQ(7u, entry.errorPly);
    EXPECT_TRUE(entry.error.starts_with("Ke3: ")) << entry.error;

    const std::string json = indexer::toJson(entry);
    EXPECT_NE(std::string::npos, json.find("\"ok\":false")) << json;
    EXPECT_NE(std::string::npos, json.find("\"plies\":6,\"error\":{\"ply\":7,\"reason\":\"Ke3: ")) << json;
    // the facts are only for a game that read.
    EXPECT_EQ(std::string::npos, json.find("\"ended\"")) << json;
    EXPECT_EQ(std::string::npos, json.find("\"material\"")) << json;
}

// skipped, as python-chess skips it.
TEST_F(GameIndexFixture, TagThatDoesntRead_IsSkipped)
{
    indexer::GameIndex entry = index("[White unquoted]\n\n1. e4 *");

    EXPECT_TRUE(entry.ok);
    EXPECT_TRUE(entry.tags.empty());
    EXPECT_EQ(1u, entry.plies);
}

TEST_F(GameIndexFixture, FenThatDoesntRead_IsAnErrorBeforeTheMoves)
{
    indexer::GameIndex entry = index("[FEN \"not a position\"]\n\n1. e4 *");

    EXPECT_FALSE(entry.ok);
    EXPECT_EQ(0u, entry.plies);
    EXPECT_EQ(0u, entry.errorPly);
}

// the value as written, Elephant "herd" \ dev, escaped for JSON.
TEST_F(GameIndexFixture, TagValues_EscapedInJson)
{
    indexer::GameIndex entry = index("[White \"Elephant \"herd\" \\ dev\"]\n\n1. e4 *");

    EXPECT_EQ("Elephant \"herd\" \\ dev", entry.tags[0].value);
    EXPECT_NE(std::string::npos, indexer::toJson(entry).find("\"White\":\"Elephant \\\"herd\\\" \\\\ dev\""));
}

TEST_F(GameIndexFixture, Result_TagFirstThenMovetextThenUnknown)
{
    EXPECT_EQ("0-1", index("[Result \"0-1\"]\n\n1. e4 1-0").result);
    EXPECT_EQ("1/2-1/2", index("1. e4 1/2-1/2").result);
    EXPECT_EQ("*", index("1. e4").result);
}

} // namespace ElephantTest
