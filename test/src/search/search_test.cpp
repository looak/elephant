#include <gtest/gtest.h>
#include "elephant_test_logger.hpp"

#include <system/clock.hpp>
#include <io/fen_parser.hpp>
#include <core/game.hpp>
#include <search/search.hpp>
#include <search_cases.hpp>
#include <system/time_manager.hpp>

namespace ElephantTest {
////////////////////////////////////////////////////////////////
class SearchFixture : public ::testing::Test {
public:
    SearchFixture() :
        timeManager(testingParams, Set::WHITE)
    {}
    virtual void SetUp(){
        testingParams.SearchDepth = 12;
        //testingParams.MoveTime = 30 * 1000; // 30 seconds

    };
    virtual void TearDown(){};

    SearchLimits testingParams;
    TimeManager timeManager;
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
    context.NewGame();
    SearchContext searchContext;
    RecordingListener listener;
    TranspositionTable tt;
    Search searcher(searchContext, tt, listener);
    testingParams.SearchDepth = 4;

    // execute
    timeManager.applyTimeSettings(testingParams, Set::WHITE);
    SearchResult result = searcher.go(makeSearchRequest(context, testingParams), timeManager);

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

    io::fen_parser::deserialize(fen.c_str(), context.editChessboard());

    SearchContext searchContext;
    TranspositionTable tt;
    Search searcher(searchContext, tt);
    
    // execute
    timeManager.applyTimeSettings(testingParams, Set::WHITE);
    SearchResult result = searcher.go(makeSearchRequest(context, testingParams), timeManager);

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
    io::fen_parser::deserialize(fen.c_str(), context.editChessboard());

    SearchContext searchContext;
    TranspositionTable tt;
    Search searcher(searchContext, tt);

    // execute
    timeManager.applyTimeSettings(testingParams, Set::BLACK);
    SearchResult result = searcher.go(makeSearchRequest(context, testingParams), timeManager);

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
    io::fen_parser::deserialize(fen.c_str(), context.editChessboard());

    SearchContext searchContext;
    TranspositionTable tt;
    Search searcher(searchContext, tt);
    timeManager.applyTimeSettings(testingParams, Set::WHITE);
    SearchResult result = searcher.go(makeSearchRequest(context, testingParams), timeManager);
    EXPECT_TRUE(result.ForcedMate);
}

TEST_F(SearchFixture, MateAgainstSelf)
{
    std::string fen("r4b2/1p4p1/p5k1/2p5/6pK/4Pq2/P1n2P1P/3R3R w - - 6 34");
    Game context;
    io::fen_parser::deserialize(fen.c_str(), context.editChessboard());

    SearchContext searchContext;
    TranspositionTable tt;
    Search searcher(searchContext, tt);
    timeManager.applyTimeSettings(testingParams, Set::WHITE);
    SearchResult result = searcher.go(makeSearchRequest(context, testingParams), timeManager);
    EXPECT_NE(result.move(), PackedMove::NullMove());
}

TEST_F(SearchFixture, ExpectedMoveSearchCases) {
    for (const auto& searchCase : s_searchCases) {
        Game context;
        io::fen_parser::deserialize(searchCase.fen.c_str(), context.editChessboard());

        SearchContext searchContext;
        TranspositionTable tt;
        Search searcher(searchContext, tt);
        timeManager.applyTimeSettings(testingParams, Set::WHITE);
        SearchResult result = searcher.go(makeSearchRequest(context, testingParams), timeManager);

        EXPECT_EQ(searchCase.expectedMove, result.move().toString());
    }
}

TEST_F(SearchFixture, ExpectedMoveMateInThree) {
    for (const auto& searchCase : s_mateInThree) {
        Game context;
        io::fen_parser::deserialize(searchCase.fen.c_str(), context.editChessboard());

        SearchContext searchContext;
        TranspositionTable tt;
        Search searcher(searchContext, tt);
        SearchResult result;
        timeManager.applyTimeSettings(testingParams, context.readToPlay());
        result = searcher.go(makeSearchRequest(context, testingParams), timeManager);

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
        io::fen_parser::deserialize(searchCase.fen.c_str(), context.editChessboard());

        SearchContext searchContext;
        TranspositionTable tt;
        Search searcher(searchContext, tt);

        SearchResult result;
        timeManager.applyTimeSettings(testingParams, context.readToPlay());
        result = searcher.go(makeSearchRequest(context, testingParams), timeManager);

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
        io::fen_parser::deserialize(searchCase.fen.c_str(), context.editChessboard());

        SearchContext searchContext;
        TranspositionTable tt;
        Search searcher(searchContext, tt);

        SearchResult result;
        timeManager.applyTimeSettings(testingParams, context.readToPlay());
        result = searcher.go(makeSearchRequest(context, testingParams), timeManager);

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
        io::fen_parser::deserialize(searchCase.fen.c_str(), context.editChessboard());

        SearchContext searchContext;
        TranspositionTable tt;
        Search searcher(searchContext, tt);

        // e1f1 is found at depth 9 & g5h6 at 8, the fixture's 12 leaves a margin. The deep d4e3 line is in
        // ElephantSuite, search_depth_test.cpp.
        SearchResult result;
        timeManager.applyTimeSettings(testingParams, context.readToPlay());
        result = searcher.go(makeSearchRequest(context, testingParams), timeManager);

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
    io::fen_parser::deserialize(fen.c_str(), context.editChessboard());

    SearchContext searchContext;
    TranspositionTable tt;
    Search searcher(searchContext, tt);
    Clock testing_clock;
    testing_clock.Start();

    testingParams.WhiteTimeIncrement = 500; // 0.5s increment
    testingParams.WhiteTimelimit = 10000;
    testingParams.MoveTime = 0; // let time manager decide

    timeManager.applyTimeSettings(testingParams, Set::WHITE);
    
    SearchResult result;    
    result = searcher.go(makeSearchRequest(context, testingParams), timeManager);    

    testing_clock.Stop();
    OUT() << "Search completed in " << testing_clock.getElapsedTime() << " ms.";
    EXPECT_LT(testing_clock.getElapsedTime(), 1000); // should finish within 1s

}

// A rook up, but every move makes the hundredth half move without a capture or pawn move and there's no mate in one.
TEST_F(SearchFixture, FiftyMoveRule_EveryMoveDraws_ScoresADraw) {
    Game context;
    io::fen_parser::deserialize("8/8/8/4k3/8/8/8/R3K3 w - - 99 120", context.editChessboard());

    SearchContext searchContext;
    TranspositionTable tt;
    Search searcher(searchContext, tt);
    testingParams.SearchDepth = 6;
    timeManager.applyTimeSettings(testingParams, Set::WHITE);
    SearchResult result = searcher.go(makeSearchRequest(context, testingParams), timeManager);

    EXPECT_EQ(0, result.score);
    EXPECT_FALSE(result.move().isNull());
}

// One move short of the fifty move rule the rook still counts.
TEST_F(SearchFixture, FiftyMoveRule_ClockFarFromLimit_ScoresTheRook) {
    Game context;
    io::fen_parser::deserialize("8/8/8/4k3/8/8/8/R3K3 w - - 0 120", context.editChessboard());

    SearchContext searchContext;
    TranspositionTable tt;
    Search searcher(searchContext, tt);
    testingParams.SearchDepth = 6;
    timeManager.applyTimeSettings(testingParams, Set::WHITE);
    SearchResult result = searcher.go(makeSearchRequest(context, testingParams), timeManager);

    EXPECT_GT(result.score, 300);
}

// --- Lazy SMP ---
TEST_F(SearchFixture, LazySmp_FourThreads_FindsTheMateAndReportsAllNodes)
{
    // setup, the mate in three from WhiteMateInThree_ExpectQg6AsFirstMove.
    std::string fen("2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - - 0 1");
    Game single;
    io::fen_parser::deserialize(fen.c_str(), single.editChessboard());
    Game threaded;
    io::fen_parser::deserialize(fen.c_str(), threaded.editChessboard());

    // do
    SearchContext singleSearchContext;
    TranspositionTable singleTT;
    Search singleSearch(singleSearchContext, singleTT);
    timeManager.applyTimeSettings(testingParams, Set::WHITE);
    SearchResult singleResult = singleSearch.go(makeSearchRequest(single, testingParams), timeManager);

    SearchContext threadedSearchContext(4);
    TranspositionTable threadedTT;
    Search threadedSearch(threadedSearchContext, threadedTT);
    timeManager.applyTimeSettings(testingParams, Set::WHITE);
    SearchResult threadedResult = threadedSearch.go(makeSearchRequest(threaded, testingParams), timeManager);

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
    context.NewGame();
    testingParams.SearchDepth = 6;

    // do
    Clock clock;
    clock.Start();
    SearchContext searchContext(8);
    TranspositionTable tt;
    Search searcher(searchContext, tt);
    timeManager.applyTimeSettings(testingParams, Set::WHITE);
    SearchResult result = searcher.go(makeSearchRequest(context, testingParams), timeManager);
    clock.Stop();

    // verify
    EXPECT_FALSE(result.move().isNull());
    EXPECT_LT(clock.getElapsedTime(), 5000);
}

TEST_F(SearchFixture, LazySmp_NodeLimit_CountsAllThreads)
{
    // setup
    Game context;
    context.NewGame();
    testingParams.SearchDepth = 0;
    testingParams.NodeLimit = 200000;

    // do
    SearchContext searchContext(4);
    TranspositionTable tt;
    Search searcher(searchContext, tt);
    timeManager.applyTimeSettings(testingParams, Set::WHITE);
    SearchResult result = searcher.go(makeSearchRequest(context, testingParams), timeManager);

    // verify, each thread sees the others' nodes up to 1024 stop checks late, a few thousand nodes over at most.
    EXPECT_FALSE(result.move().isNull());
    EXPECT_GE(result.count, 200000u);
    EXPECT_LT(result.count, 200000u + 4u * 4096u);
}

}  // namespace ElephantTest