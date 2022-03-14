#include <gtest/gtest.h>
#include "fen_parser.h"
#include "game_context.h"

namespace ElephantTest
{
////////////////////////////////////////////////////////////////
class FenParserFixture : public ::testing::Test
{
public:
    virtual void SetUp()
    {

    };
    virtual void TearDown() {};

    GameContext testContext;
};
////////////////////////////////////////////////////////////////
TEST_F(FenParserFixture, Initialize)
{
    std::string empty = "";
    bool result = FENParser::deserialize(empty.c_str(), testContext);
    EXPECT_FALSE(result);
}

TEST_F(FenParserFixture, StartingPosition)
{
    std::string startingPositionFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    bool result = FENParser::deserialize(startingPositionFen.c_str(), testContext);
    EXPECT_FALSE(result);
}
////////////////////////////////////////////////////////////////

} // namespace ElephantTest