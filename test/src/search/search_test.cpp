#include <gtest/gtest.h>
#include "elephant_test_logger.hpp"

#include <system/clock.hpp>
#include <io/fen_parser.hpp>
#include <core/game_context.hpp>
#include <search/search.hpp>
#include <search_cases.hpp>

namespace ElephantTest {
////////////////////////////////////////////////////////////////
class SearchFixture : public ::testing::Test {
public:
    SearchFixture() :
        timeManager(testingParams, Set::WHITE)
    {}
    virtual void SetUp(){
        testingParams.SearchDepth = 12;
        testingParams.ThreadCount = 1;
        //testingParams.MoveTime = 30 * 1000; // 30 seconds

    };
    virtual void TearDown(){};

    SearchParameters testingParams;
    TimeManager timeManager;
};

////////////////////////////////////////////////////////////////

TEST_F(SearchFixture, WhiteMateInThree_ExpectQg6AsFirstMove)
{
    // setup
    std::string fen("2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - - 0 1");
    GameContext context;

    io::fen_parser::deserialize(fen.c_str(), context.editChessboard());

    Search searcher(context);
    
    // execute
    timeManager.applyTimeSettings(testingParams, Set::WHITE);
    SearchResult result = searcher.go<Set::WHITE>(testingParams, timeManager);

    i32 mateScore = c_checkmateConstant - result.score;
    mateScore /= 2;

    EXPECT_EQ(2, mateScore);
    EXPECT_EQ(Square::G3, result.move().sourceSqr());
    EXPECT_EQ(Square::G6, result.move().targetSqr());
}

TEST_F(SearchFixture, BlackMateInTwo_ExpectQc4CheckAsFirstMove)
{
    // setup
    GameContext context;

    std::string fen("5k2/6pp/p1qN4/1p1p4/3P4/2PKP2Q/PP3r2/3R4 b - - 0 1");
    io::fen_parser::deserialize(fen.c_str(), context.editChessboard());

    Search searcher(context);

    // execute
    timeManager.applyTimeSettings(testingParams, Set::BLACK);
    SearchResult result = searcher.go<Set::BLACK>(testingParams, timeManager);

    i32 mateScore = c_checkmateConstant - result.score;
    mateScore /= 2;

    EXPECT_EQ(2, mateScore);
    EXPECT_EQ(Square::C6, result.move().sourceSqr());
    EXPECT_EQ(Square::C4, result.move().targetSqr());
}

TEST_F(SearchFixture, WhiteForcedMate)
{
    std::string fen("5k2/8/3N4/1p1p4/2qP4/2PKP3/5r2/8 w - - 1 2");
    GameContext context;
    io::fen_parser::deserialize(fen.c_str(), context.editChessboard());

    Search searcher(context);
    timeManager.applyTimeSettings(testingParams, Set::WHITE);
    SearchResult result = searcher.go<Set::WHITE>(testingParams, timeManager);
    EXPECT_TRUE(result.ForcedMate);
}

TEST_F(SearchFixture, MateAgainstSelf)
{
    std::string fen("r4b2/1p4p1/p5k1/2p5/6pK/4Pq2/P1n2P1P/3R3R w - - 6 34");
    GameContext context;
    io::fen_parser::deserialize(fen.c_str(), context.editChessboard());

    Search searcher(context);
    timeManager.applyTimeSettings(testingParams, Set::WHITE);
    SearchResult result = searcher.go<Set::WHITE>(testingParams, timeManager);
    EXPECT_NE(result.move(), PackedMove::NullMove());
}

TEST_F(SearchFixture, ExpectedMoveSearchCases) {
    for (const auto& searchCase : s_searchCases) {
        GameContext context;
        io::fen_parser::deserialize(searchCase.fen.c_str(), context.editChessboard());

        Search searcher(context);
        timeManager.applyTimeSettings(testingParams, Set::WHITE);
        SearchResult result = searcher.go<Set::WHITE>(testingParams, timeManager);

        EXPECT_EQ(searchCase.expectedMove, result.move().toString());
    }
}

TEST_F(SearchFixture, ExpectedMoveMateInThree) {
    for (const auto& searchCase : s_mateInThree) {
        GameContext context;
        io::fen_parser::deserialize(searchCase.fen.c_str(), context.editChessboard());

        Search searcher(context);
        SearchResult result;
        if (context.readToPlay() == Set::BLACK) {
            timeManager.applyTimeSettings(testingParams, Set::BLACK);
            result = searcher.go<Set::BLACK>(testingParams, timeManager);
        }
        else {
            timeManager.applyTimeSettings(testingParams, Set::WHITE);
            result = searcher.go<Set::WHITE>(testingParams, timeManager);
        }

        EXPECT_EQ(searchCase.expectedMove, result.move().toString());
        OUT() << "Tested position: " << searchCase.fen;
        OUT() << "Expected move:   " << searchCase.expectedMove;
        OUT() << "Found move:      " << result.move().toString();
        OUT() << "-----------------------------";
    }
}

// using this to test performance of search.
TEST_F(SearchFixture, DISABLED_ExpectedMoveMateInFive) {
    for (const auto& searchCase : s_mateInFive) {
        GameContext context;
        io::fen_parser::deserialize(searchCase.fen.c_str(), context.editChessboard());

        Search searcher(context);

        SearchResult result;
        if (context.readToPlay() == Set::BLACK) {
            timeManager.applyTimeSettings(testingParams, Set::BLACK);
            result = searcher.go<Set::BLACK>(testingParams, timeManager);
        }
        else {
            timeManager.applyTimeSettings(testingParams, Set::WHITE);
            result = searcher.go<Set::WHITE>(testingParams, timeManager);
        }

        EXPECT_EQ(searchCase.expectedMove, result.move().toString());
        OUT() << "Tested position: " << searchCase.fen;
        OUT() << "Expected move:   " << searchCase.expectedMove;
        OUT() << "Found move:      " << result.move().toString();
        OUT() << "-----------------------------";
    }
}

TEST_F(SearchFixture, DISABLED_ExpectedMoveMateInEight) {
    for (const auto& searchCase : s_mateInEight) {
        GameContext context;
        testingParams.SearchDepth = 16; // increase depth for harder mates
        io::fen_parser::deserialize(searchCase.fen.c_str(), context.editChessboard());

        Search searcher(context);

        SearchResult result;
        if (context.readToPlay() == Set::BLACK) {
            timeManager.applyTimeSettings(testingParams, Set::BLACK);
            result = searcher.go<Set::BLACK>(testingParams, timeManager);
        }
        else {
            timeManager.applyTimeSettings(testingParams, Set::WHITE);
            result = searcher.go<Set::WHITE>(testingParams, timeManager);
        }

        EXPECT_EQ(searchCase.expectedMove, result.move().toString());
        OUT() << "Tested position: " << searchCase.fen;
        OUT() << "Expected move:   " << searchCase.expectedMove;
        OUT() << "Found move:      " << result.move().toString();
        OUT() << "-----------------------------";
    }
}

TEST_F(SearchFixture, NullMovePruning_ExpectedMove) {
    for (const auto& searchCase : s_nullMovePruning) {
        GameContext context;
        io::fen_parser::deserialize(searchCase.fen.c_str(), context.editChessboard());

        Search searcher(context);

        testingParams.SearchDepth = 10;
        SearchResult result;
        if (context.readToPlay() == Set::BLACK) {
            timeManager.applyTimeSettings(testingParams, Set::BLACK);
            result = searcher.go<Set::BLACK>(testingParams, timeManager);
        }
        else {
            timeManager.applyTimeSettings(testingParams, Set::WHITE);
            result = searcher.go<Set::WHITE>(testingParams, timeManager);
        }

        EXPECT_EQ(searchCase.expectedMove, result.move().toString());
        OUT() << "Tested position: " << searchCase.fen;
        OUT() << "Expected move:   " << searchCase.expectedMove;
        OUT() << "Found move:      " << result.move().toString();
        OUT() << "-----------------------------";
    }
}

TEST_F(SearchFixture, SearchTimeManagement_InitialTest) {   
    GameContext context;
    std::string fen("r4rk1/2pb1p1p/p1n1p1p1/1p6/3Pn3/2P2N2/PP1BRPPP/5RK1 w - - 0 19");
    io::fen_parser::deserialize(fen.c_str(), context.editChessboard());

    Search searcher(context);
    Clock testing_clock;
    testing_clock.Start();

    testingParams.WhiteTimeIncrement = 500; // 0.5s increment
    testingParams.WhiteTimelimit = 10000;
    testingParams.MoveTime = 0; // let time manager decide

    timeManager.applyTimeSettings(testingParams, Set::WHITE);
    
    SearchResult result;    
    result = searcher.go<Set::WHITE>(testingParams, timeManager);    

    testing_clock.Stop();
    OUT() << "Search completed in " << testing_clock.getElapsedTime() << " ms.";
    EXPECT_LT(testing_clock.getElapsedTime(), 1000); // should finish within 1s

}

}  // namespace ElephantTest