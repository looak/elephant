#include <gtest/gtest.h>
#include "elephant_test_utils.h"
#include "chessboard.h"

namespace ElephantTest
{
////////////////////////////////////////////////////////////////
class MoveFixture : public ::testing::Test
{
public:
    virtual void SetUp()
    {
    };
    virtual void TearDown() {};

    Chessboard m_chessboard;
};
////////////////////////////////////////////////////////////////

TEST_F(MoveFixture, PawnMoves)
{
    // setup
    m_chessboard.editTile(e4).editPiece() = WHITEPAWN;

    // do
    bool result = m_chessboard.MakeMove(e4, e5);
    
    // verify
    ChessPiece exp; // default, "empty" piece
    EXPECT_EQ(exp, m_chessboard.readTile(e4).readPiece());
    EXPECT_EQ(WHITEPAWN, m_chessboard.readTile(e5).readPiece());
}

}