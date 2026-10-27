#include <gtest/gtest.h>

#include <draw.hpp>
#include <polyglot.hpp>
#include <core/chessboard.hpp>
#include <io/fen_parser.hpp>
#include <io/pgn_parser.hpp>

#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace ElephantTest {
////////////////////////////////////////////////////////////////

/**
 * @file draw_test.cpp
 * @brief Testing phant-indexer --draw: the training positions drawn from games. Every position written has to be one
 * the game reached, in the game's own order of plies, and naturally quiet when asked for; the sample is seeded, a
 * position is written once in a run, and a game that didn't read or has no result gives nothing.
 * Naming convention as of October 2023: <TestedFunctionality>_<ExpectedResult>
 * @author Alexander Loodin Ek  */
class DrawFixture : public ::testing::Test {
public:
    indexer::GameIndex index(const std::string& pgn)
    {
        m_streams.push_back(std::make_unique<std::string>(pgn));
        io::pgn_parser::GameReader reader(*m_streams.back());
        std::optional<io::pgn_parser::Game> game = reader.next();
        EXPECT_TRUE(game);
        return indexer::index(*game, 1, false, true, true);
    }

    // the lines a Drawer writes for the games, one after another.
    std::string draw(indexer::Drawer& drawer, const std::vector<std::string>& games)
    {
        std::string out;
        for (const std::string& game : games)
            drawer.draw(index(game), out);
        return out;
    }

    static std::vector<std::string> lines(const std::string& out)
    {
        std::vector<std::string> result;
        std::istringstream stream(out);
        std::string line;
        while (std::getline(stream, line))
            result.push_back(line);
        return result;
    }

    // the FEN of a line, without its result.
    static std::set<std::string> asSet(const std::vector<std::string>& values) { return { values.begin(), values.end() }; }

    static std::string fenOf(const std::string& line) { return line.substr(0, line.rfind(' ')); }

    static u64 hashOfFen(const std::string& fen)
    {
        Chessboard board;
        EXPECT_TRUE(io::fen_parser::deserialize(fen, board)) << fen;
        return indexer::polyglotHash(board);
    }

    // the plies of a game whose position a written FEN is.
    static std::set<size_t> pliesWritten(const indexer::GameIndex& entry, const std::string& out)
    {
        std::set<size_t> plies;
        for (const std::string& line : lines(out)) {
            const u64 hash = hashOfFen(fenOf(line));
            for (size_t ply = 0; ply < entry.positions.size(); ++ply) {
                if (entry.positions[ply] == hash)
                    plies.insert(ply);
            }
        }
        return plies;
    }

    std::vector<std::unique_ptr<std::string>> m_streams;
};

// 1.e4 d5 2.exd5 Qxd5 3.Nc3 Qa5 4.d4 Nf6, a pawn on d5 to take after ply 2 and the queen to take it after ply 3.
static const std::string c_pawnTrade = "[Result \"1-0\"]\n\n1. e4 d5 2. exd5 Qxd5 3. Nc3 Qa5 4. d4 Nf6 1-0";

////////////////////////////////////////////////////////////////

TEST_F(DrawFixture, ResultLabels)
{
    EXPECT_EQ("[1.0]", indexer::resultLabel("1-0"));
    EXPECT_EQ("[0.5]", indexer::resultLabel("1/2-1/2"));
    EXPECT_EQ("[0.0]", indexer::resultLabel("0-1"));
    EXPECT_EQ("", indexer::resultLabel("*"));
}

// a position with a capture worth making isn't quiet: after 2.exd5 black is a pawn down and takes it back. Whether
// 2.exd5 itself, a pawn trade, beats standing pat after 1.e4 d5 is the evaluation's call, so ply 2 isn't checked.
TEST_F(DrawFixture, Quiet_LeavesOutPositionsWithACaptureToMake)
{
    indexer::Drawer drawer({ .perGame = 100, .minPly = 0, .quiet = true, .seed = 1 });
    const indexer::GameIndex entry = index(c_pawnTrade);
    std::string out;
    drawer.draw(entry, out);

    const std::set<size_t> plies = pliesWritten(entry, out);
    EXPECT_TRUE(plies.contains(0));     // the start position
    EXPECT_FALSE(plies.contains(3));    // Qxd5 wins the pawn back
    EXPECT_TRUE(plies.contains(4));
    EXPECT_EQ(plies.size(), lines(out).size());
    for (const std::string& line : lines(out))
        EXPECT_TRUE(line.ends_with(" [1.0]")) << line;
}

// without --quiet every position with a move and not in check is a candidate.
TEST_F(DrawFixture, NotQuiet_KeepsThemAll)
{
    indexer::Drawer drawer({ .perGame = 100, .minPly = 0, .quiet = false, .seed = 1 });
    const indexer::GameIndex entry = index(c_pawnTrade);
    std::string out;
    drawer.draw(entry, out);

    EXPECT_EQ(entry.plies + 1, lines(out).size());
    EXPECT_EQ(entry.plies + 1, pliesWritten(entry, out).size());
}

// 3.Qh5+ puts black in check: a position the game reached, but not one to learn from.
TEST_F(DrawFixture, InCheck_NeverWritten)
{
    indexer::Drawer drawer({ .perGame = 100, .minPly = 0, .quiet = false, .seed = 1 });
    const indexer::GameIndex entry = index("[Result \"1/2-1/2\"]\n\n1. e4 f6 2. Qh5+ g6 1/2-1/2");
    std::string out;
    drawer.draw(entry, out);

    const std::set<size_t> plies = pliesWritten(entry, out);
    EXPECT_FALSE(plies.contains(3));
    EXPECT_EQ(4u, plies.size());
    for (const std::string& line : lines(out))
        EXPECT_TRUE(line.ends_with(" [0.5]")) << line;
}

// a checkmate has no move to be played from and no evaluation to learn.
TEST_F(DrawFixture, Checkmate_NeverWritten)
{
    indexer::Drawer drawer({ .perGame = 100, .minPly = 0, .quiet = false, .seed = 1 });
    const indexer::GameIndex entry = index("[Result \"0-1\"]\n\n1. f3 e5 2. g4 Qh4# 0-1");
    std::string out;
    drawer.draw(entry, out);

    EXPECT_EQ(std::set<size_t>({ 0, 1, 2, 3 }), pliesWritten(entry, out));
}

TEST_F(DrawFixture, MinPly_SkipsTheFirstPlies)
{
    indexer::Drawer drawer({ .perGame = 100, .minPly = 4, .quiet = false, .seed = 1 });
    const indexer::GameIndex entry = index(c_pawnTrade);
    std::string out;
    drawer.draw(entry, out);

    EXPECT_EQ(std::set<size_t>({ 4, 5, 6, 7, 8 }), pliesWritten(entry, out));
}

// a file's own minimum counts plies from where its games start, whatever their FEN's move number says.
TEST_F(DrawFixture, MinPlyForAFile_CountsFromWhereTheGameStarts)
{
    // a game from the 9th move of a book: the file's minimum of 2 is two of its own plies, not the global 18 or the move number.
    const std::string game =
        "[Result \"1/2-1/2\"]\n[FEN \"4k3/8/8/8/8/8/4P3/4K3 w - - 0 9\"]\n\n1. Kd2 Kd7 2. Kd3 Kd6 1/2-1/2";

    indexer::Drawer drawer({ .perGame = 100, .minPly = 18, .quiet = false, .seed = 1 });
    const indexer::GameIndex entry = index(game);
    std::string out;
    drawer.draw(entry, out, 2u);
    EXPECT_EQ(std::set<size_t>({ 2, 3, 4 }), pliesWritten(entry, out));

    indexer::Drawer plain({ .perGame = 100, .minPly = 20, .quiet = false, .seed = 1 });
    std::string other;
    plain.draw(entry, other, 0u);   // its own minimum of 0 beats --min-ply 20
    EXPECT_EQ(5u, lines(other).size());
    EXPECT_EQ(1u, plain.stats().ownMinimum);
}

// the tag is how standard input carries a minimum, game by game, and it wins over the file's and over --min-ply.
TEST_F(DrawFixture, MinPlyTag_OverridesTheFileAndTheOptionAndSkipsThePhase)
{
    indexer::Drawer drawer({ .perGame = 100, .minPly = 16, .quiet = false, .seed = 1 });
    const indexer::GameIndex entry = index("[Result \"1-0\"]\n[DrawMinPly \"3\"]\n"
        "[FEN \"r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 0 1\"]\n\n"
        "1... Nf6 2. Nxe5 Nxe4 3. Nxc6 dxc6 4. d3 Nf6 1-0");
    std::string out;
    drawer.draw(entry, out, 6u);

    EXPECT_EQ(std::set<size_t>({ 3, 4, 5, 6, 7 }), pliesWritten(entry, out));   // the tag's 3, not the file's 6

    // a tag that isn't a number counts as none.
    indexer::Drawer bad({ .perGame = 100, .minPly = 0, .quiet = false, .seed = 1 });
    std::string text;
    const indexer::GameIndex plain = index("[Result \"1-0\"]\n[DrawMinPly \"four\"]\n\n1. e4 e5 1-0");
    bad.draw(plain, text);
    EXPECT_EQ(3u, lines(text).size());
    EXPECT_EQ(1u, bad.stats().badMinimumTags);
    EXPECT_EQ(0u, bad.stats().ownMinimum);
}

// --merge: a position drawn by several games is written once with the average of their results, white's.
TEST_F(DrawFixture, Merge_AveragesTheResultsOfTheGamesThatDrewAPosition)
{
    indexer::Drawer drawer({ .perGame = 100, .minPly = 0, .quiet = false, .merge = true, .seed = 1 });
    std::string out;
    for (const char* result : { "1-0", "1-0", "1-0", "0-1" })
        drawer.draw(index(std::string("[Result \"") + result + "\"]\n\n1. e4 " + result), out);
    drawer.draw(index("[Result \"1/2-1/2\"]\n\n1. d4 1/2-1/2"), out);
    EXPECT_TRUE(out.empty());   // written when the run ends
    drawer.finish(out);

    // each position once, in the order the seeded samples drew them, which is not the order of the plies.
    const std::vector<std::string> all = lines(out);
    const std::set<std::string> written(all.begin(), all.end());
    EXPECT_EQ(3u, all.size());
    EXPECT_EQ(std::set<std::string>({
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 [0.7]",         // 3 wins, a loss and a draw
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1 [0.75]",     // 3 wins and a loss
        "rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq - 0 1 [0.5]" }),   // the draw
        written);
    EXPECT_EQ(10u, drawer.stats().draws);
    EXPECT_EQ(3u, drawer.stats().written);
    EXPECT_EQ(2u, drawer.stats().merged);
    EXPECT_EQ(0u, drawer.stats().duplicates);
}

TEST_F(DrawFixture, Merge_AverageLabels)
{
    indexer::Drawer drawer({ .perGame = 100, .minPly = 0, .quiet = false, .merge = true, .seed = 1 });
    std::string out;
    // the start position is every game's: 3 wins and a loss, then a win, a draw and a loss, then the only games alone.
    for (const char* result : { "1-0", "1-0", "1-0", "0-1" })
        drawer.draw(index(std::string("[Result \"") + result + "\"]\n\n1. e4 " + result), out);
    drawer.finish(out);

    EXPECT_EQ(std::set<std::string>({
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 [0.75]",
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1 [0.75]" }),
        asSet(lines(out)));

    indexer::Drawer thirds({ .perGame = 1, .minPly = 0, .quiet = false, .merge = true, .seed = 1 });
    std::string text;
    for (const char* result : { "1-0", "0-1", "0-1" })
        thirds.draw(index(std::string("[Result \"") + result + "\"]\n\n1. Nf3 " + result), text);
    thirds.finish(text);
    // each game gives one position, a different one by the seeded sample or the same, and the three starts may differ:
    // only the labels' form is pinned here, a third is 4 places.
    for (const std::string& line : lines(text)) {
        const std::string label = line.substr(line.rfind(' ') + 1);
        EXPECT_TRUE(label == "[1.0]" || label == "[0.0]" || label == "[0.5]" || label == "[0.3333]" || label == "[0.6667]"
            || label == "[0.25]" || label == "[0.75]") << line;
    }
}

// the same inputs, order and seed give the same merged lines, and the cap per game still applies before merging.
TEST_F(DrawFixture, Merge_RepeatsAndKeepsThePerGameCap)
{
    auto run = [&](u64 seed) {
        indexer::Drawer drawer({ .perGame = 2, .minPly = 0, .quiet = false, .merge = true, .seed = seed });
        std::string out;
        drawer.draw(index(c_pawnTrade), out);
        drawer.draw(index("[Result \"0-1\"]\n\n1. e4 d5 2. exd5 Qxd5 3. Nc3 Qa5 4. d4 Nf6 0-1"), out);
        drawer.finish(out);
        EXPECT_EQ(4u, drawer.stats().draws);
        return out;
    };
    EXPECT_EQ(run(5), run(5));
    EXPECT_NE(run(5), run(6));
}

TEST_F(DrawFixture, NoMerge_FinishWritesNothing)
{
    indexer::Drawer drawer({ .perGame = 100, .minPly = 0, .quiet = false, .seed = 1 });
    std::string out;
    drawer.draw(index(c_pawnTrade), out);
    const std::string before = out;
    drawer.finish(out);
    EXPECT_EQ(before, out);
}

// --min-ply is the game's own first plies, from the position it starts at: the FEN's move number, the book's depth and
// the counters having been reset change nothing, and there is no guessing from the pieces left.
TEST_F(DrawFixture, MinPly_CountsFromWhereTheGameStarts)
{
    for (const char* counters : { "0 9", "0 1", "7 30" }) {
        indexer::Drawer drawer({ .perGame = 100, .minPly = 2, .quiet = false, .seed = 1 });
        const indexer::GameIndex entry = index(std::string("[Result \"1/2-1/2\"]\n[FEN \"4k3/8/8/8/8/8/4P3/4K3 w - - ")
            + counters + "\"]\n\n1. Kd2 Kd7 2. Kd3 Kd6 1/2-1/2");
        std::string out;
        drawer.draw(entry, out);

        EXPECT_EQ(std::set<size_t>({ 2, 3, 4 }), pliesWritten(entry, out)) << counters;
    }

    // a full board with its counters reset isn't too early for it: the first position is the game's ply 0.
    indexer::Drawer drawer({ .perGame = 100, .minPly = 0, .quiet = false, .seed = 1 });
    const indexer::GameIndex entry = index("[Result \"1-0\"]\n"
        "[FEN \"r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 0 1\"]\n\n1... Nf6 2. Nxe5 1-0");
    std::string out;
    drawer.draw(entry, out);
    EXPECT_EQ(3u, lines(out).size());
}

// FILE:P, a colon in a Windows path isn't one.
TEST_F(DrawFixture, SplitInputMinimum)
{
    using Split = std::pair<std::string, std::optional<u32>>;
    EXPECT_EQ(Split("match60/1.pgn.zst", 4u), indexer::splitInputMinimum("match60/1.pgn.zst:4"));
    EXPECT_EQ(Split("endgames/1.pgn.zst", 0u), indexer::splitInputMinimum("endgames/1.pgn.zst:0"));
    EXPECT_EQ(Split("-", 4u), indexer::splitInputMinimum("-:4"));
    EXPECT_EQ(Split("a.pgn", std::nullopt), indexer::splitInputMinimum("a.pgn"));
    EXPECT_EQ(Split("C:\\games\\a.pgn", std::nullopt), indexer::splitInputMinimum("C:\\games\\a.pgn"));
    EXPECT_EQ(Split("C:\\games\\a.pgn", 12u), indexer::splitInputMinimum("C:\\games\\a.pgn:12"));
    EXPECT_EQ(Split("C", 4u), indexer::splitInputMinimum("C:4"));   // a file called C with a minimum of 4
    EXPECT_EQ(Split("a.pgn:", std::nullopt), indexer::splitInputMinimum("a.pgn:"));
    EXPECT_EQ(Split(":4", std::nullopt), indexer::splitInputMinimum(":4"));
    EXPECT_EQ(Split("a.pgn:x", std::nullopt), indexer::splitInputMinimum("a.pgn:x"));
}

// the statistics of a run, as JSON: pinned byte for byte on a run small enough to count by hand.
TEST_F(DrawFixture, StatsJson_ADrawnRun)
{
    const indexer::DrawOptions options{ .perGame = 2, .minPly = 0, .quiet = false, .seed = 1 };
    indexer::Drawer drawer(options);
    std::string out;
    std::vector<indexer::DrawInput> inputs = { { "a.pgn", 1u, 0, 0 }, { "b.pgn", std::nullopt, 0, 0 } };
    auto give = [&](size_t input, const std::string& pgn) {
        const u64 before = drawer.stats().drawn;
        drawer.draw(index(pgn), out, inputs[input].minPly);
        ++inputs[input].games;
        inputs[input].drawn += drawer.stats().drawn - before;
    };
    give(0, "[Result \"1-0\"]\n\n1. e4 e5 1-0");                    // 2 of its 3 positions past the minimum, 2 drawn
    give(0, "[Result \"0-1\"]\n\n1. e4 e5 0-1");                    // the same two: duplicates
    give(1, "1. e4 e5 *");                                           // no result
    give(1, "[Result \"1/2-1/2\"]\n\n1. Nf3 Ke2 1/2-1/2");          // the second move is illegal: it didn't read
    give(1, "[Result \"1/2-1/2\"]\n\n1. d4 d5 1/2-1/2");            // 3 positions, 2 drawn
    drawer.finish(out);

    EXPECT_EQ("{\"stats\":1,\"reader\":\"1.3.2\",\"options\":{\"draw\":2,\"quiet\":false,\"min_ply\":0,\"merge\":false,"
              "\"seed\":1},\"inputs\":[{\"file\":\"a.pgn\",\"min_ply\":1,\"games\":2,\"used\":2},"
              "{\"file\":\"b.pgn\",\"min_ply\":null,\"games\":3,\"used\":1}],"
              "\"games\":{\"read\":5,\"used\":3,\"skipped\":2,\"unread\":1,\"no_result\":1,\"null_moves\":0,"
              "\"own_minimum\":2,\"bad_minimum_tags\":0},"
              "\"positions\":{\"candidates\":7,\"quiet\":7,\"quiet_per_game\":{\"mean\":2.33,\"median\":2,\"p5\":2,\"min\":2},"
              "\"games_under_n\":0,\"written\":4,\"duplicates_dropped\":2,\"drawn\":null,\"merged\":null,"
              "\"results\":{\"white\":2,\"draw\":2,\"black\":0,\"mixed\":0}}}",
        indexer::statsJson(drawer.stats(), options, inputs, "1.3.2"));
}

// with merge the same run counts what was drawn and merged, and a position averaged is a mixed result.
TEST_F(DrawFixture, StatsJson_AMergedRun)
{
    const indexer::DrawOptions options{ .perGame = 5, .minPly = 0, .quiet = true, .merge = true, .seed = 7 };
    indexer::Drawer drawer(options);
    std::string out;
    drawer.draw(index("[Result \"1-0\"]\n\n1. e4 e5 1-0"), out);
    drawer.draw(index("[Result \"0-1\"]\n\n1. e4 e5 0-1"), out);
    drawer.draw(index("[Result \"1-0\"]\n\n1. e4 e5 1-0"), out);
    drawer.finish(out);

    const std::string json = indexer::statsJson(drawer.stats(), options, { { "-", std::nullopt, 2, 2 } }, "x\"y");
    EXPECT_NE(std::string::npos, json.find("\"reader\":\"x\\\"y\"")) << json;
    EXPECT_NE(std::string::npos, json.find("\"quiet\":true,\"min_ply\":0,\"merge\":true,\"seed\":7")) << json;
    EXPECT_NE(std::string::npos, json.find("\"written\":3,\"duplicates_dropped\":null,\"drawn\":9,\"merged\":3,"
                                           "\"results\":{\"white\":0,\"draw\":0,\"black\":0,\"mixed\":3}}}")) << json;
}

TEST_F(DrawFixture, PerGame_CapsTheSample)
{
    indexer::Drawer drawer({ .perGame = 3, .minPly = 0, .quiet = false, .seed = 1 });
    std::string out;
    drawer.draw(index(c_pawnTrade), out);

    EXPECT_EQ(3u, lines(out).size());
    EXPECT_EQ(9u, drawer.stats().candidates);
    EXPECT_EQ(3u, drawer.stats().written);
}

TEST_F(DrawFixture, Sample_RepeatsWithItsSeedAndChangesWithAnother)
{
    // one game, long enough that four of its positions are one of many choices.
    const std::string game = "[Result \"1-0\"]\n\n1. e4 e5 2. Nf3 Nc6 3. Bb5 a6 4. Ba4 Nf6 5. O-O Be7 6. Re1 b5 7. Bb3 d6 "
                             "8. c3 O-O 9. h3 Nb8 10. d4 Nbd7 11. c4 c6 12. cxb5 axb5 13. Nc3 Bb7 14. Bg5 b4 15. Nb1 h6 1-0";

    auto run = [&](u64 seed) {
        indexer::Drawer drawer({ .perGame = 4, .minPly = 0, .quiet = false, .seed = seed });
        std::string out;
        drawer.draw(index(game), out);
        return out;
    };
    EXPECT_EQ(run(7), run(7));
    EXPECT_NE(run(7), run(8));
    EXPECT_EQ(4u, lines(run(7)).size());
}

// a position is written once in a run, however many games reach it: here the same game twice, and two that share
// their first plies.
TEST_F(DrawFixture, Duplicates_WrittenOnce)
{
    indexer::Drawer drawer({ .perGame = 100, .minPly = 0, .quiet = false, .seed = 1 });
    std::string out;
    drawer.draw(index("[Result \"1-0\"]\n\n1. e4 e5 2. Nf3 1-0"), out);
    const size_t first = lines(out).size();
    EXPECT_EQ(4u, first);

    drawer.draw(index("[Result \"0-1\"]\n\n1. e4 e5 2. Nf3 0-1"), out);
    EXPECT_EQ(first, lines(out).size());
    EXPECT_EQ(4u, drawer.stats().duplicates);

    drawer.draw(index("[Result \"0-1\"]\n\n1. e4 e5 2. Bc4 0-1"), out);   // one new position after the shared three
    EXPECT_EQ(first + 1, lines(out).size());
    for (const std::string& line : lines(out)) {
        if (line.starts_with("rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b")) {
            EXPECT_TRUE(line.ends_with(" [1.0]")) << "the first game's label stays: " << line;
        }
    }
}

TEST_F(DrawFixture, NoResultOrNotRead_GivesNothing)
{
    indexer::Drawer drawer({ .perGame = 100, .minPly = 0, .quiet = false, .seed = 1 });
    std::string out;
    drawer.draw(index("1. e4 e5 2. Nf3 Nc6 *"), out);
    drawer.draw(index("[Result \"1-0\"]\n\n1. e4 e5 2. Ke2 Qh4 3. Kf3 Qf4 4. Ke3 1-0"), out);   // 4.Ke3 walks into the queen
    drawer.draw(index("[Result \"1-0\"]\n\n1. e4 e5 2. Ke5 1-0"), out);
    drawer.draw(index("[Result \"1-0\"]\n\n1. e4 -- 2. d4 1-0"), out);   // a null move was never played over the board

    EXPECT_TRUE(out.empty());
    EXPECT_EQ(4u, drawer.stats().games);
}

// a game from a FEN draws positions of its own, the counters carried.
TEST_F(DrawFixture, FromAFen)
{
    indexer::Drawer drawer({ .perGame = 100, .minPly = 0, .quiet = false, .seed = 1 });
    std::string out;
    drawer.draw(index("[Result \"1/2-1/2\"]\n[FEN \"4k3/8/8/8/8/8/4P3/4K3 w - - 0 1\"]\n\n1. Kd2 Kd7 1/2-1/2"), out);

    const std::vector<std::string> written = lines(out);
    ASSERT_EQ(3u, written.size());
    EXPECT_NE(std::string::npos, out.find("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1 [0.5]"));
    EXPECT_NE(std::string::npos, out.find("4k3/8/8/8/8/8/3KP3/8 b - - 1 1 [0.5]"));
    EXPECT_NE(std::string::npos, out.find("8/3k4/8/8/8/8/3KP3/8 w - - 2 2 [0.5]"));
}

} // namespace ElephantTest
