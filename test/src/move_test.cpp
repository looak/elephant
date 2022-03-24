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

TEST_F(MoveFixture, PawnMoves)
{
    // setup
    auto P = WHITEPAWN;
    m_chessboard.PlacePiece(P, e4);
    m_chessboard.PlacePiece(P, d2);

    // do
    bool result = m_chessboard.MakeMove(e4, e5);
    EXPECT_TRUE(result);
    
    // verify
    ChessPiece exp; // default, "empty" piece
    EXPECT_EQ(exp, m_chessboard.readTile(e4).readPiece());
    EXPECT_EQ(WHITEPAWN, m_chessboard.readTile(e5).readPiece());

    result = m_chessboard.MakeMove(d2, d4);
    EXPECT_TRUE(result);
    
    // verify
    EXPECT_EQ(exp, m_chessboard.readTile(d2).readPiece());
    EXPECT_EQ(WHITEPAWN, m_chessboard.readTile(d4).readPiece());
}

TEST_F(MoveFixture, InvalidMove_NoPiece)
{
    // do
    bool result = m_chessboard.MakeMove(e4, e5);
    EXPECT_FALSE(result);
}

TEST_F(MoveFixture, InvalidMove_OccupideSpace)
{    
    // setup
    auto P = WHITEPAWN;
    auto b = BLACKBISHOP;
    m_chessboard.PlacePiece(P, e4);
    m_chessboard.PlacePiece(b, e5);

    // do
    bool result = m_chessboard.MakeMove(e4, e5);
    EXPECT_FALSE(result);
    
    // verify
    EXPECT_EQ(P, m_chessboard.readTile(e4).readPiece());
    EXPECT_EQ(b, m_chessboard.readTile(e5).readPiece());
}

TEST_F(MoveFixture, KingMove_Capture)
{   
    auto K = WHITEKING;
    auto q = BLACKQUEEN;
    m_chessboard.PlacePiece(K, e4);
    m_chessboard.PlacePiece(q, d5);

    EXPECT_EQ(K, m_chessboard.readTile(e4).readPiece());
    EXPECT_EQ(q, m_chessboard.readTile(d5).readPiece());

    // do
    bool result = m_chessboard.MakeMove(e4, d5);
    EXPECT_TRUE(result);

    ChessPiece exp; // default, "empty" piece
    EXPECT_EQ(K, m_chessboard.readTile(d5).readPiece());
    EXPECT_EQ(exp, m_chessboard.readTile(e4).readPiece());
}

}