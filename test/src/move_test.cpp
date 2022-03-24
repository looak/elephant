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

TEST_F(MoveFixture, InvalidMoves)
{
    bool result = false;
    result = m_chessboard.MakeMove(Notation(87), Notation(0));
    EXPECT_FALSE(result);
    result = m_chessboard.MakeMove(Notation(0), Notation(127));
    EXPECT_FALSE(result);

    // no piece on target tile
    result = m_chessboard.MakeMove(Notation(0), Notation(2));
    EXPECT_FALSE(result);
}

// TEST_F(MoveFixture, PawnMoves)
// {
//     // setup
//     m_chessboard.editTile(e4).editPiece() = WHITEPAWN;

//     // do
//     bool result = m_chessboard.MakeMove(e4, e5);
//     ASSERT_TRUE(result);
    
//     // verify
//     ChessPiece exp; // default, "empty" piece
//     EXPECT_EQ(exp, m_chessboard.readTile(e4).readPiece());
//     EXPECT_EQ(WHITEPAWN, m_chessboard.readTile(e5).readPiece());
// }

}