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

// a position with a capture worth making isn't quiet: after 1.e4 d5 white takes on d5, after 2.exd5 black does.
TEST_F(DrawFixture, Quiet_LeavesOutPositionsWithACaptureToMake)
{
    indexer::Drawer drawer({ .perGame = 100, .minPly = 0, .quiet = true, .seed = 1 });
    const indexer::GameIndex entry = index(c_pawnTrade);
    std::string out;
    drawer.draw(entry, out);

    const std::set<size_t> plies = pliesWritten(entry, out);
    EXPECT_TRUE(plies.contains(0));     // the start position
    EXPECT_FALSE(plies.contains(2));    // exd5 is there to be played
    EXPECT_FALSE(plies.contains(3));    // and Qxd5
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
