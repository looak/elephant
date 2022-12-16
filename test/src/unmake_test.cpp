#include <gtest/gtest.h>
#include "elephant_test_utils.h"
#include "move.h"
#include "chessboard.h"
#include "chess_piece.h"

namespace ElephantTest
{
////////////////////////////////////////////////////////////////
class UnmakeFixture : public ::testing::Test
{
public:
    virtual void SetUp()
    {

    };
    virtual void TearDown() 
    {};

    Chessboard m_chessboard;
};
////////////////////////////////////////////////////////////////


// 8 [   ][   ][   ][   ][   ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][ P ][   ][   ][   ]
// 1 [   ][   ][   ][   ][   ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// 1. e3, unmake, e4, unmake.
TEST_F(UnmakeFixture, Pawn_SimpleMoves)
{
    auto P = WHITEPAWN;    
    m_chessboard.PlacePiece(P, e2);
    u64 orgHash = m_chessboard.readHash();

    {
        Move move(e2, e3);

        // do e3 move
        bool result = m_chessboard.MakeMove(move);

        // verify
        EXPECT_TRUE(result);
        EXPECT_EQ(P, move.Piece);
        EXPECT_EQ(MoveFlag::Zero, move.Flags);

        EXPECT_EQ(Notation(), m_chessboard.readEnPassant());
        EXPECT_EQ(P, m_chessboard.readTile(e3).readPiece());
        EXPECT_EQ(ChessPiece(), m_chessboard.readTile(e2).readPiece());
        EXPECT_NE(orgHash, m_chessboard.readHash());

        // unmake move    
        result = m_chessboard.UnmakeMove(move);

        // verify state of board
        EXPECT_TRUE(result);
        EXPECT_EQ(ChessPiece(), m_chessboard.readTile(e3).readPiece());
        EXPECT_EQ(P, m_chessboard.readTile(e2).readPiece());
        EXPECT_EQ(Notation(), m_chessboard.readEnPassant());
        EXPECT_EQ(orgHash, m_chessboard.readHash());
    }

    {
        Move move(e2, e4);

        // do e4 move
        bool result = m_chessboard.MakeMove(move);

        // verify
        EXPECT_TRUE(result);
        EXPECT_EQ(P, move.Piece);        

        EXPECT_EQ(e3, m_chessboard.readEnPassant());
        EXPECT_EQ(P, m_chessboard.readTile(e4).readPiece());
        EXPECT_EQ(ChessPiece(), m_chessboard.readTile(e2).readPiece());
        EXPECT_NE(orgHash, m_chessboard.readHash());

        // unmake move    
        result = m_chessboard.UnmakeMove(move);

        // verify state of board
        EXPECT_TRUE(result);
        EXPECT_EQ(ChessPiece(), m_chessboard.readTile(e4).readPiece());
        EXPECT_EQ(P, m_chessboard.readTile(e2).readPiece());
        EXPECT_EQ(Notation(), m_chessboard.readEnPassant());
        EXPECT_EQ(orgHash, m_chessboard.readHash());

    }
}

// 8 [   ][   ][   ][   ][   ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][ p ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][ P ][   ][   ][   ]
// 1 [   ][   ][   ][   ][   ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// Moves:
// 1. e4 dxe3 e.p.
TEST_F(UnmakeFixture, EnPassant_Captured_Unmake)
{
    auto P = WHITEPAWN;
    auto p = BLACKPAWN;
    m_chessboard.PlacePiece(P, e2);
    m_chessboard.PlacePiece(p, d4);
    Move move(e2, e4);

    // do
    bool result = m_chessboard.MakeMove(move);

    // validate
    EXPECT_TRUE(result);
    EXPECT_EQ(P, move.Piece);
    EXPECT_NE(MoveFlag::EnPassant, move.Flags & MoveFlag::EnPassant);

    EXPECT_EQ(e3, m_chessboard.readEnPassant());
    EXPECT_EQ(P, m_chessboard.readTile(e4).readPiece());
    EXPECT_EQ(p, m_chessboard.readTile(d4).readPiece());

    // setup
    Move epCapture(d4, e3);

    // do
    result = m_chessboard.MakeMove(epCapture);

    // validate
    EXPECT_TRUE(result);
    EXPECT_EQ(p, epCapture.Piece);
    EXPECT_EQ(MoveFlag::Capture, epCapture.Flags & MoveFlag::Capture);
    EXPECT_EQ(MoveFlag::EnPassant, epCapture.Flags & MoveFlag::EnPassant);

    ChessPiece exp; // default, "empty" piece
    EXPECT_EQ(exp, m_chessboard.readTile(e4).readPiece());
    EXPECT_EQ(exp, m_chessboard.readTile(d4).readPiece());
    EXPECT_EQ(p, m_chessboard.readTile(e3).readPiece());


    // do
    result = m_chessboard.UnmakeMove(epCapture);

    // validate
    EXPECT_TRUE(result);
    EXPECT_EQ(e3, m_chessboard.readEnPassant());
    EXPECT_EQ(P, m_chessboard.readTile(e4).readPiece());
    EXPECT_EQ(p, m_chessboard.readTile(d4).readPiece());
    EXPECT_EQ(exp, m_chessboard.readTile(e3).readPiece());

    // setup
    auto moves = m_chessboard.GetAvailableMoves(Set::BLACK);
    bool founde3 = false;
    bool foundd3 = false;

    // validate
    EXPECT_EQ(2, moves.size());
    for (auto mv : moves)
    {
        // do
        result = m_chessboard.MakeMove(mv);
        EXPECT_TRUE(result);

        // validate
        if (mv.TargetSquare == e3)
        {
            EXPECT_EQ(WHITEPAWN, mv.Capture);
            EXPECT_EQ(MoveFlag::Capture, mv.Flags & MoveFlag::Capture);
            EXPECT_EQ(MoveFlag::EnPassant, mv.Flags & MoveFlag::EnPassant);
            founde3 = true;
        }
        else
        {
            EXPECT_EQ(d3, mv.TargetSquare);
            EXPECT_EQ(exp, mv.Capture); // no capture
            EXPECT_EQ(MoveFlag::Zero, mv.Flags);
            foundd3 = true;
        }

        // do
        m_chessboard.UnmakeMove(mv);
        EXPECT_EQ(e3, m_chessboard.readEnPassant());
    }

    // validate
    EXPECT_TRUE(founde3);
    EXPECT_TRUE(foundd3);
}


// 8 [   ][   ][   ][   ][   ][   ][   ][   ]
// 7 [   ][   ][   ][ p ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][ P ][ P ][   ][   ][   ]
// 1 [   ][   ][   ][   ][   ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// 1. e4 d5 2. d3 dxe4 3. dxe4
TEST_F(UnmakeFixture, Pawn_SimpleMoves_ScandinavianDefense)
{
    auto P = WHITEPAWN;
    auto p = BLACKPAWN;
    m_chessboard.PlacePiece(P, e2);
    m_chessboard.PlacePiece(p, d7);
    Move mve4(e2, e4);
    Move mvd5(d7, d5);
    Move mvexd5(e4, d5);
    
    // do move sequence
    m_chessboard.MakeMove(mve4);
    m_chessboard.MakeMove(mvd5);
    m_chessboard.MakeMove(mvexd5);
}
////////////////////////////////////////////////////////////////

} // namespace ElephantTest