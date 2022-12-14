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