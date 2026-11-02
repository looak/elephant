#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <optional>
#include <stdexcept>
#include <thread>

#include <core/game.hpp>
#include <io/fen_parser.hpp>
#include <search/monad.hpp>

namespace ElephantTest {
/**
 * @file monad_test.cpp
 * @brief What a Monad promises whoever drives it: how a search ends, when its answer comes and that two don't meet.
 * A broken stop shows up as a test that never ends, ctest's timeout catches it.
 * Naming convention: <TestedFunction>_<Condition>_<ExpectedBehavior>    */
////////////////////////////////////////////////////////////////

namespace {
constexpr const char* c_mateInOne = "6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1";

class AnswerListener final : public SearchListener {
public:
    void onIteration(const IterationInfo&) override {}
    void onBestMove(const SearchResult& result, u64 elapsedMs) override {
        move = result.move().toString();
        elapsed = elapsedMs;
        ++answers;
    }
    void onSearchFailed(const std::exception&) override { ++failures; }

    std::atomic<int> answers = 0;
    std::atomic<int> failures = 0;
    std::string move;
    u64 elapsed = 0;
};

SearchRequest requestFor(const char* fen, SearchLimits limits)
{
    Game game;
    io::fen_parser::deserialize(fen, game.editChessboard());
    return makeSearchRequest(game, std::move(limits));
}

SearchRequest startposRequest(SearchLimits limits)
{
    Game game;
    game.NewGame();
    return makeSearchRequest(game, std::move(limits));
}
} // namespace

TEST(MonadTest, Wait_DepthSearch_ReturnsTheResultAndTheListenerHearsIt)
{
    Monad monad;
    AnswerListener listener;
    SearchLimits limits;
    limits.SearchDepth = 3;

    monad.go(requestFor(c_mateInOne, limits), listener);
    SearchResult result = monad.wait();

    EXPECT_EQ("a1a8", result.move().toString());
    EXPECT_EQ("a1a8", listener.move);
    EXPECT_EQ(1, listener.answers);
    EXPECT_EQ(0, listener.failures);
}

TEST(MonadTest, Wait_WithoutASearch_IsAnEmptyResult)
{
    Monad monad;
    EXPECT_TRUE(monad.wait().pvLine.moves[0].isNull());
}

TEST(MonadTest, Go_InfiniteEndingOnItsOwn_AnswersOnlyAfterStop)
{
    Monad monad;
    AnswerListener listener;
    SearchLimits limits;
    limits.Infinite = true;

    monad.go(requestFor(c_mateInOne, limits), listener);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    EXPECT_EQ(0, listener.answers);

    monad.stop();
    SearchResult result = monad.wait();

    EXPECT_EQ(1, listener.answers);
    EXPECT_EQ("a1a8", result.move().toString());
    // measured when the search ended, the time spent waiting for stop isn't search.
    EXPECT_LT(listener.elapsed, 300u);
}

TEST(MonadTest, Stop_RightAfterGo_IsKept)
{
    Monad monad;
    AnswerListener listener;
    SearchLimits limits;
    limits.Infinite = true;

    for (int i = 0; i < 20; ++i) {
        monad.go(startposRequest(limits), listener);
        monad.stop();
        (void)monad.wait();
    }
    EXPECT_EQ(20, listener.answers);
}

TEST(MonadTest, Stop_FromAnEarlierSearch_DoesNotCarryOver)
{
    Monad monad;
    AnswerListener listener;
    SearchLimits limits;
    limits.SearchDepth = 4;

    monad.stop();
    monad.go(startposRequest(limits), listener);
    SearchResult result = monad.wait();

    EXPECT_FALSE(result.pvLine.moves[0].isNull());
    EXPECT_GT(result.count, 100u);
}

TEST(MonadTest, Go_WhileSearching_EndsTheOldSearchFirst)
{
    Monad monad;
    AnswerListener listener;
    SearchLimits infinite;
    infinite.Infinite = true;
    SearchLimits shallow;
    shallow.SearchDepth = 2;

    monad.go(startposRequest(infinite), listener);
    monad.go(startposRequest(shallow), listener);
    (void)monad.wait();

    EXPECT_EQ(2, listener.answers);
}

TEST(MonadTest, Setters_WhileSearching_EndTheSearchFirst)
{
    Monad monad;
    AnswerListener listener;
    SearchLimits infinite;
    infinite.Infinite = true;

    monad.go(startposRequest(infinite), listener);
    monad.setHash(2);
    EXPECT_EQ(1, listener.answers);

    monad.go(startposRequest(infinite), listener);
    monad.setThreads(2);
    EXPECT_EQ(2, listener.answers);

    monad.go(startposRequest(infinite), listener);
    monad.setMoveOverhead(5);
    EXPECT_EQ(3, listener.answers);

    monad.go(startposRequest(infinite), listener);
    monad.newGame();
    EXPECT_EQ(4, listener.answers);
}

TEST(MonadTest, Destructor_WhileSearching_StopsTheSearch)
{
    AnswerListener listener;
    SearchLimits limits;
    limits.Infinite = true;
    {
        Monad monad;
        monad.go(startposRequest(limits), listener);
    }
    EXPECT_EQ(1, listener.answers);
}

TEST(MonadTest, NewGame_ZeroesTheTable)
{
    Monad monad;
    AnswerListener listener;
    SearchLimits limits;
    limits.SearchDepth = 3;
    const SearchRequest request = startposRequest(limits);
    const u64 rootHash = request.root.read().hash();

    PackedMove move;
    Score score = 0;
    u8 depth = 0;
    TranspositionFlag flag = TTF_NONE;

    monad.go(request, listener);
    (void)monad.wait();
    ASSERT_TRUE(monad.editTranspositionTable().probe(rootHash, move, score, depth, flag));

    monad.newGame();
    EXPECT_FALSE(monad.editTranspositionTable().probe(rootHash, move, score, depth, flag));
}

TEST(MonadTest, Go_ListenerThrowing_IsReportedAsAFailedSearch)
{
    class ThrowingListener final : public SearchListener {
    public:
        void onIteration(const IterationInfo&) override {}
        void onBestMove(const SearchResult&, u64) override { throw std::runtime_error("out of luck"); }
        void onSearchFailed(const std::exception& e) override { failure = e.what(); }
        std::string failure;
    };

    Monad monad;
    ThrowingListener listener;
    SearchLimits limits;
    limits.SearchDepth = 2;

    monad.go(startposRequest(limits), listener);
    (void)monad.wait();

    EXPECT_EQ("out of luck", listener.failure);
}

// the same search gives the same nodes whether another Monad is searching next to it or not, nothing is shared.
TEST(MonadTest, TwoMonads_SearchingAtOnce_DoNotMeet)
{
    SearchLimits limits;
    limits.SearchDepth = 6;

    Monad alone;
    AnswerListener listener;
    alone.go(startposRequest(limits), listener);
    const u64 expected = alone.wait().count;

    Monad first;
    Monad second;
    AnswerListener firstListener;
    AnswerListener secondListener;
    first.go(startposRequest(limits), firstListener);
    second.go(startposRequest(limits), secondListener);

    EXPECT_EQ(expected, first.wait().count);
    EXPECT_EQ(expected, second.wait().count);
}

////////////////////////////////////////////////////////////////

}  // namespace ElephantTest
