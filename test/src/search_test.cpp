#include <gtest/gtest.h>
#include "elephant_test_utils.h"

#include "clock.hpp"
#include "fen_parser.h"
#include "game_context.h"
#include "move_generator.h"

namespace ElephantTest
{
////////////////////////////////////////////////////////////////
class SearchFixture : public ::testing::Test
{
public:
    virtual void SetUp()
    {

    };
    virtual void TearDown() {};

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
   // execute   
   Move mv = context.CalculateBestMove(params);

   EXPECT_EQ(g6, mv.TargetSquare);
   EXPECT_EQ(WHITEQUEEN, mv.Piece);
}

TEST_F(SearchFixture, BlackMateInTwo_ExpectQc4CheckAsFirstMove)
{
    // setup
    GameContext context;

    std::string fen("5k2/6pp/p1qN4/1p1p4/3P4/2PKP2Q/PP3r2/3R4 b - - 0 1");    
    FENParser::deserialize(fen.c_str(), context);

    SearchParameters params;
    params.SearchDepth = 4;

    //execute
    Move mv = context.CalculateBestMove(params);

    EXPECT_EQ(c4, mv.TargetSquare);
    EXPECT_EQ(BLACKQUEEN, mv.Piece);
}

} // namexpace ElephantTest
// 2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - - 0 1