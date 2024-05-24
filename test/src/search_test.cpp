#include <gtest/gtest.h>
#include "elephant_test_utils.h"

#include "clock.hpp"
#include "fen_parser.h"
#include "game_context.h"
#include "search.hpp"

namespace ElephantTest {
////////////////////////////////////////////////////////////////
class SearchFixture : public ::testing::Test {
public:
    virtual void SetUp(){

    };
    virtual void TearDown(){};
};

////////////////////////////////////////////////////////////////

TEST_F(SearchFixture, WhiteMateInThree_ExpectQg6AsFirstMove)
{
    // setup
    std::string fen("2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - - 0 1");
    GameContext context;

    FENParser::deserialize(fen.c_str(), context);

    SearchParameters params;
    params.SearchDepth = 4;
    params.Infinite = true;
    // execute
    SearchResult result = context.CalculateBestMove(params);

    i32 mateScore = 24000 - result.score;
    mateScore /= 2;

    EXPECT_EQ(2, mateScore);
    EXPECT_EQ(Square::G3, result.move.sourceSqr());
    EXPECT_EQ(Square::G6, result.move.targetSqr());
}

TEST_F(SearchFixture, BlackMateInTwo_ExpectQc4CheckAsFirstMove)
{
    // setup
    GameContext context;

    std::string fen("5k2/6pp/p1qN4/1p1p4/3P4/2PKP2Q/PP3r2/3R4 b - - 0 1");
    FENParser::deserialize(fen.c_str(), context);

    SearchParameters params;
    params.SearchDepth = 4;
    params.Infinite = true;

    // execute
    SearchResult result = context.CalculateBestMove(params);

    i32 mateScore = 24000 - result.score;
    mateScore /= 2;

    EXPECT_EQ(2, mateScore);
    EXPECT_EQ(Square::C6, result.move.sourceSqr());
    EXPECT_EQ(Square::C4, result.move.targetSqr());
}

TEST_F(SearchFixture, WhiteForcedMate)
{
    std::string fen("5k2/8/3N4/1p1p4/2qP4/2PKP3/5r2/8 w - - 1 2");
    GameContext context;
    FENParser::deserialize(fen.c_str(), context);

    SearchParameters params;
    params.SearchDepth = 3;
    params.Infinite = true;

    SearchResult result = context.CalculateBestMove(params);
    EXPECT_TRUE(result.ForcedMate);
}

}  // namespace ElephantTest
   // 2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - - 0 1