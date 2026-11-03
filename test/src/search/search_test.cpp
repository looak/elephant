#include <gtest/gtest.h>
#include "elephant_test_logger.hpp"

#include <system/clock.hpp>
#include <io/fen_parser.hpp>
#include <core/game.hpp>
#include <move/generation/move_ordering_view.hpp>
#include <search/core/search_constants.hpp>
#include <search/core/search_policies.hpp>
#include <search/monad.hpp>
#include <search_cases.hpp>

namespace ElephantTest {
////////////////////////////////////////////////////////////////
class SearchFixture : public ::testing::Test {
public:
    virtual void SetUp(){
        testingParams.SearchDepth = 12;
        //testingParams.MoveTime = 30 * 1000; // 30 seconds

    };
    virtual void TearDown(){};

    SearchLimits testingParams;
};

////////////////////////////////////////////////////////////////

// One thread, so nothing here needs a lock.
class RecordingListener final : public SearchListener {
public:
    void onIteration(const IterationInfo& info) override {
        depths.push_back(info.depth);
        threadIds.push_back(info.threadId);
        lastNodes = info.nodes;
        lastMove = info.pv.moves[0];
    }

    std::vector<u32> depths;
    std::vector<u16> threadIds;
    u64 lastNodes = 0;
    PackedMove lastMove = PackedMove::NullMove();
};

TEST_F(SearchFixture, Listener_HearsEveryIterationOfTheSearch)
{
    // setup
    Game context;
    Monad monad(64);
    RecordingListener listener;
    testingParams.SearchDepth = 4;

    // execute
    SearchResult result = monad.search(makeSearchRequest(context, testingParams), listener);

    // verify
    EXPECT_EQ((std::vector<u32>{ 1, 2, 3, 4 }), listener.depths);
    EXPECT_EQ((std::vector<u16>{ 0, 0, 0, 0 }), listener.threadIds);
    EXPECT_GT(listener.lastNodes, 0u);
    EXPECT_EQ(result.move(), listener.lastMove);
}

TEST_F(SearchFixture, WhiteMateInThree_ExpectQg6AsFirstMove)
{
    // setup
    std::string fen("2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - - 0 1");
    Game context;

    context.loadFen(fen.c_str());

    Monad monad(64);
    
    // execute
    SearchResult result = monad.search(makeSearchRequest(context, testingParams));

    i32 mateScore = c_checkmateConstant - result.score;
    mateScore /= 2;

    EXPECT_EQ(2, mateScore);
    EXPECT_EQ(Square::G3, result.move().sourceSqr());
    EXPECT_EQ(Square::G6, result.move().targetSqr());
}

TEST_F(SearchFixture, BlackMateInTwo_ExpectQc4CheckAsFirstMove)
{
    // setup
    Game context;

    std::string fen("5k2/6pp/p1qN4/1p1p4/3P4/2PKP2Q/PP3r2/3R4 b - - 0 1");
    context.loadFen(fen.c_str());

    Monad monad(64);

    // execute
    SearchResult result = monad.search(makeSearchRequest(context, testingParams));

    i32 mateScore = c_checkmateConstant - result.score;
    mateScore /= 2;

    EXPECT_EQ(2, mateScore);
    EXPECT_EQ(Square::C6, result.move().sourceSqr());
    EXPECT_EQ(Square::C4, result.move().targetSqr());
}

TEST_F(SearchFixture, WhiteForcedMate)
{
    std::string fen("5k2/8/3N4/1p1p4/2qP4/2PKP3/5r2/8 w - - 1 2");
    Game context;
    context.loadFen(fen.c_str());

    Monad monad(64);
    SearchResult result = monad.search(makeSearchRequest(context, testingParams));
    EXPECT_TRUE(result.ForcedMate);
}

TEST_F(SearchFixture, MateAgainstSelf)
{
    std::string fen("r4b2/1p4p1/p5k1/2p5/6pK/4Pq2/P1n2P1P/3R3R w - - 6 34");
    Game context;
    context.loadFen(fen.c_str());

    Monad monad(64);
    SearchResult result = monad.search(makeSearchRequest(context, testingParams));
    EXPECT_NE(result.move(), PackedMove::NullMove());
}

TEST_F(SearchFixture, ExpectedMoveSearchCases) {
    for (const auto& searchCase : s_searchCases) {
        Game context;
        context.loadFen(searchCase.fen.c_str());

        Monad monad(64);
        SearchResult result = monad.search(makeSearchRequest(context, testingParams));

        EXPECT_EQ(searchCase.expectedMove, result.move().toString());
    }
}

TEST_F(SearchFixture, ExpectedMoveMateInThree) {
    for (const auto& searchCase : s_mateInThree) {
        Game context;
        context.loadFen(searchCase.fen.c_str());

        Monad monad(64);
        SearchResult result;
        result = monad.search(makeSearchRequest(context, testingParams));

        EXPECT_EQ(searchCase.expectedMove, result.move().toString());
        OUT() << "Tested position: " << searchCase.fen;
        OUT() << "Expected move:   " << searchCase.expectedMove;
        OUT() << "Found move:      " << result.move().toString();
        OUT() << "-----------------------------";
    }
}

// were disabled as too slow, both run in well under a second since 0.15.
TEST_F(SearchFixture, ExpectedMoveMateInFive) {
    for (const auto& searchCase : s_mateInFive) {
        Game context;
        context.loadFen(searchCase.fen.c_str());

        Monad monad(64);

        SearchResult result;
        result = monad.search(makeSearchRequest(context, testingParams));

        EXPECT_EQ(searchCase.expectedMove, result.move().toString());
        OUT() << "Tested position: " << searchCase.fen;
        OUT() << "Expected move:   " << searchCase.expectedMove;
        OUT() << "Found move:      " << result.move().toString();
        OUT() << "-----------------------------";
    }
}

TEST_F(SearchFixture, ExpectedMoveMateInEight) {
    for (const auto& searchCase : s_mateInEight) {
        Game context;
        testingParams.SearchDepth = 16; // increase depth for harder mates
        context.loadFen(searchCase.fen.c_str());

        Monad monad(64);

        SearchResult result;
        result = monad.search(makeSearchRequest(context, testingParams));

        EXPECT_EQ(searchCase.expectedMove, result.move().toString());
        OUT() << "Tested position: " << searchCase.fen;
        OUT() << "Expected move:   " << searchCase.expectedMove;
        OUT() << "Found move:      " << result.move().toString();
        OUT() << "-----------------------------";
    }
}

TEST_F(SearchFixture, NullMovePruning_ExpectedMove) {
    for (const auto& searchCase : s_nullMovePruning) {
        Game context;
        context.loadFen(searchCase.fen.c_str());

        Monad monad(64);

        // e1f1 is found at depth 9 & g5h6 at 8, the fixture's 12 leaves a margin. The deep d4e3 line is in
        // ElephantSuite, search_depth_test.cpp.
        SearchResult result;
        result = monad.search(makeSearchRequest(context, testingParams));

        EXPECT_EQ(searchCase.expectedMove, result.move().toString());
        OUT() << "Tested position: " << searchCase.fen;
        OUT() << "Expected move:   " << searchCase.expectedMove;
        OUT() << "Found move:      " << result.move().toString();
        OUT() << "-----------------------------";
    }
}

TEST_F(SearchFixture, SearchTimeManagement_InitialTest) {   
    Game context;
    std::string fen("r4rk1/2pb1p1p/p1n1p1p1/1p6/3Pn3/2P2N2/PP1BRPPP/5RK1 w - - 0 19");
    context.loadFen(fen.c_str());

    Monad monad(64);
    Clock testing_clock;
    testing_clock.Start();

    testingParams.WhiteTimeIncrement = 500; // 0.5s increment
    testingParams.WhiteTimelimit = 10000;
    testingParams.MoveTime = 0; // let time manager decide

    
    SearchResult result;    
    result = monad.search(makeSearchRequest(context, testingParams));    

    testing_clock.Stop();
    OUT() << "Search completed in " << testing_clock.getElapsedTime() << " ms.";
    EXPECT_LT(testing_clock.getElapsedTime(), 1000); // should finish within 1s

}

// A rook up, but every move makes the hundredth half move without a capture or pawn move and there's no mate in one.
TEST_F(SearchFixture, FiftyMoveRule_EveryMoveDraws_ScoresADraw) {
    Game context;
    context.loadFen("8/8/8/4k3/8/8/8/R3K3 w - - 99 120");

    Monad monad(64);
    testingParams.SearchDepth = 6;
    SearchResult result = monad.search(makeSearchRequest(context, testingParams));

    EXPECT_EQ(0, result.score);
    EXPECT_FALSE(result.move().isNull());
}

// One move short of the fifty move rule the rook still counts.
TEST_F(SearchFixture, FiftyMoveRule_ClockFarFromLimit_ScoresTheRook) {
    Game context;
    context.loadFen("8/8/8/4k3/8/8/8/R3K3 w - - 0 120");

    Monad monad(64);
    testingParams.SearchDepth = 6;
    SearchResult result = monad.search(makeSearchRequest(context, testingParams));

    EXPECT_GT(result.score, 300);
}

// --- Lazy SMP ---
TEST_F(SearchFixture, LazySmp_FourThreads_FindsTheMateAndReportsAllNodes)
{
    // setup, the mate in three from WhiteMateInThree_ExpectQg6AsFirstMove.
    std::string fen("2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - - 0 1");
    Game single;
    single.loadFen(fen.c_str());
    Game threaded;
    threaded.loadFen(fen.c_str());

    // do
    Monad singleMonad(64);
    SearchResult singleResult = singleMonad.search(makeSearchRequest(single, testingParams));

    Monad threadedMonad(64, 4);
    SearchResult threadedResult = threadedMonad.search(makeSearchRequest(threaded, testingParams));

    // verify, the main thread's move is played and the count holds the helpers' nodes too. Four threads can finish
    // the depth in fewer nodes than one, sharing the table, so the count is compared with the main thread's own.
    EXPECT_EQ(Square::G3, threadedResult.move().sourceSqr());
    EXPECT_EQ(Square::G6, threadedResult.move().targetSqr());
    EXPECT_EQ(singleResult.score, threadedResult.score);
    EXPECT_EQ(singleResult.count, singleResult.mainThreadCount);
    EXPECT_GT(threadedResult.count, threadedResult.mainThreadCount);
}

TEST_F(SearchFixture, LazySmp_DepthLimit_HelpersStopWithTheMainThread)
{
    // setup, without a time limit the helpers would search to the ply limit if the main thread didn't stop them.
    Game context;
    testingParams.SearchDepth = 6;

    // do
    Clock clock;
    clock.Start();
    Monad monad(64, 8);
    SearchResult result = monad.search(makeSearchRequest(context, testingParams));
    clock.Stop();

    // verify
    EXPECT_FALSE(result.move().isNull());
    EXPECT_LT(clock.getElapsedTime(), 5000);
}

TEST_F(SearchFixture, LazySmp_NodeLimit_CountsAllThreads)
{
    // setup
    Game context;
    testingParams.SearchDepth = 0;
    testingParams.NodeLimit = 200000;

    // do
    Monad monad(64, 4);
    SearchResult result = monad.search(makeSearchRequest(context, testingParams));

    // verify, each thread sees the others' nodes up to 1024 stop checks late, a few thousand nodes over at most.
    EXPECT_FALSE(result.move().isNull());
    EXPECT_GE(result.count, 200000u);
    EXPECT_LT(result.count, 200000u + 4u * 4096u);
}

namespace {
u8 lateQuietReduction(u8 depth, u16 moveIndex, i32 history) {
    return search_policies::LMR::reduction(depth, moveIndex, false, true, false, false, history);
}
} // namespace

TEST(LateMoveReduction, GoodHistoryReducesLess_BadHistoryMore)
{
    const u8 neutral = lateQuietReduction(12, 15, 0);
    ASSERT_GE(neutral, 2);

    EXPECT_EQ(lateQuietReduction(12, 15, lmr_params::historyDivisor), neutral - 1);
    EXPECT_EQ(lateQuietReduction(12, 15, -lmr_params::historyDivisor), neutral + 1);
}

TEST(LateMoveReduction, HistoryBelowOneStep_ChangesNothing)
{
    const u8 neutral = lateQuietReduction(12, 15, 0);

    EXPECT_EQ(lateQuietReduction(12, 15, lmr_params::historyDivisor - 1), neutral);
    EXPECT_EQ(lateQuietReduction(12, 15, -lmr_params::historyDivisor + 1), neutral);
}

TEST(LateMoveReduction, HistoryStaysWithinTheClamp)
{
    // the largest the three tables can sum to, either way.
    const i32 extreme = 3 * MoveOrderingView::historyMax;

    // never an extension.
    EXPECT_EQ(lateQuietReduction(12, 15, extreme), 0);
    // the reduced child stays at depth 1 or more, out of quiescence.
    EXPECT_EQ(lateQuietReduction(6, 15, -extreme), 4);
    EXPECT_EQ(lateQuietReduction(3, 3, -extreme), 1);
}

TEST(LateMoveReduction, HistoryDoesntReduceWhatWasNeverReduced)
{
    const i32 awful = -3 * MoveOrderingView::historyMax;

    EXPECT_EQ(search_policies::LMR::reduction(12, 15, false, false, false, false, awful), 0); // not quiet
    EXPECT_EQ(search_policies::LMR::reduction(12, 15, false, true, true, false, awful), 0);   // in check
    EXPECT_EQ(search_policies::LMR::reduction(12, 15, false, true, false, true, awful), 0);   // gives check
    EXPECT_EQ(search_policies::LMR::reduction(12, 2, false, true, false, false, awful), 0);   // early in the ordering
    EXPECT_EQ(search_policies::LMR::reduction(2, 15, false, true, false, false, awful), 0);   // too shallow
}

}  // namespace ElephantTest