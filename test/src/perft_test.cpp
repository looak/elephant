#include <gtest/gtest.h>
#include "game_context.h"
#include "fen_parser.h"
#include "move_generator.h"
#include "elephant_test_utils.h"

namespace ElephantTest
{
////////////////////////////////////////////////////////////////
class PerftFixture : public ::testing::Test
{
public:
    virtual void SetUp()
    {

    };
    virtual void TearDown() {};

    GameContext m_context;
    MoveGenerator m_moveGenerator;
};
////////////////////////////////////////////////////////////////

TEST_F(PerftFixture, InitialPosition)
{
    // setup
    char inputFen[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    FENParser::deserialize(inputFen, m_context);

    PrintBoard(m_context.readChessboard());
    // do
    auto moves = m_moveGenerator.GeneratePossibleMoves(m_context);

    // verify    
    auto count = m_moveGenerator.CountMoves(moves);
    EXPECT_EQ(20, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);
}
////////////////////////////////////////////////////////////////

} // namespace ElephantTest