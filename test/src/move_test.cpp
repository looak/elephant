#include <gtest/gtest.h>
#include "elephant_test_utils.h"
#include "move.h"
#include "chessboard.h"
#include "chess_piece.h"


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
    auto move = Move(Notation(87), Notation(0));
    result = m_chessboard.MakeMove(move);
    EXPECT_FALSE(result);
    EXPECT_EQ(move.Flags, MoveFlag::Invalid);

    move.TargetSquare = Notation(127);
    move.SourceSquare = Notation(0);
    move.Flags = MoveFlag::Zero;
    result = m_chessboard.MakeMove(move);
    EXPECT_FALSE(result);
    EXPECT_EQ(move.Flags, MoveFlag::Invalid);
    EXPECT_EQ(ChessPiece(), move.Piece);

    move.TargetSquare = Notation(0);
    move.SourceSquare = Notation(2);
    move.Flags = MoveFlag::Zero;
    // no piece on target tile
    result = m_chessboard.MakeMove(move);
    EXPECT_FALSE(result);
    EXPECT_EQ(move.Flags, MoveFlag::Invalid);
}

TEST_F(MoveFixture, PawnMoves)
{
    // setup
    auto P = WHITEPAWN;
    m_chessboard.PlacePiece(P, e4);
    m_chessboard.PlacePiece(P, d2);
    auto move = Move(e4, e5);

    // do
    bool result = m_chessboard.MakeMove(move);
    EXPECT_TRUE(result);
    EXPECT_EQ(MoveFlag::Zero, move.Flags);
    EXPECT_EQ(P, move.Piece);
    
    // verify
    ChessPiece exp; // default, "empty" piece
    EXPECT_EQ(exp, m_chessboard.readTile(e4).readPiece());
    EXPECT_EQ(WHITEPAWN, m_chessboard.readTile(e5).readPiece());

    move.SourceSquare = d2;
    move.TargetSquare = d4;
    move.Piece = ChessPiece();
    result = m_chessboard.MakeMove(move);
    EXPECT_TRUE(result);
    EXPECT_EQ(P, move.Piece);
    
    // verify
    EXPECT_EQ(exp, m_chessboard.readTile(d2).readPiece());
    EXPECT_EQ(WHITEPAWN, m_chessboard.readTile(d4).readPiece());
}

TEST_F(MoveFixture, PawnMoves_enpassant_white)
{
    // setup
    auto P = WHITEPAWN;
    m_chessboard.PlacePiece(P, d2);
    Move move(d2, d4);

    // do
    bool result = m_chessboard.MakeMove(move);
    EXPECT_TRUE(result);
    EXPECT_EQ(MoveFlag::EnPassant, MoveFlag::EnPassant & move.Flags);
    
    // verify
    ChessPiece exp; // default, "empty" piece
    EXPECT_EQ(exp, m_chessboard.readTile(d2).readPiece());
    EXPECT_EQ(WHITEPAWN, m_chessboard.readTile(d4).readPiece());

    auto enPassantSqr = d3; // expected
    EXPECT_EQ(enPassantSqr, m_chessboard.readEnPassant());    
}

TEST_F(MoveFixture, PawnMoves_enpassant_black)
{
    // setup
    auto p = BLACKPAWN;
    m_chessboard.PlacePiece(p, d7);
    Move move(d7, d5);

    // do
    bool result = m_chessboard.MakeMove(move);
    EXPECT_TRUE(result);
    EXPECT_EQ(MoveFlag::EnPassant, MoveFlag::EnPassant & move.Flags);
    
    // verify
    ChessPiece exp; // default, "empty" piece
    EXPECT_EQ(exp, m_chessboard.readTile(d7).readPiece());
    EXPECT_EQ(BLACKPAWN, m_chessboard.readTile(d5).readPiece());

    auto enPassantSqr = d6; // expected
    EXPECT_EQ(enPassantSqr, m_chessboard.readEnPassant());    
}

TEST_F(MoveFixture, InvalidMove_NoPiece)
{
    Move move(e4, e5);

    // do
    bool result = m_chessboard.MakeMove(move);
    EXPECT_FALSE(result);
    EXPECT_EQ(MoveFlag::Invalid, move.Flags);

}

TEST_F(MoveFixture, InvalidMove_OccupideSpace)
{    
    // setup
    auto P = WHITEPAWN;
    auto b = BLACKBISHOP;
    m_chessboard.PlacePiece(P, e4);
    m_chessboard.PlacePiece(b, e5);
    Move move(e4, e5);

    // do
    bool result = m_chessboard.MakeMove(move);
    EXPECT_FALSE(result);
    EXPECT_EQ(MoveFlag::Invalid, move.Flags);
    EXPECT_EQ(ChessPiece(), move.Piece);
    
    // verify
    EXPECT_EQ(P, m_chessboard.readTile(e4).readPiece());
    EXPECT_EQ(b, m_chessboard.readTile(e5).readPiece());
}

// 8 [   ][   ][   ][   ][   ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][ q ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][ K ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][   ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// Moves:
// Kxd5
TEST_F(MoveFixture, KingMove_Capture)
{   
    auto K = WHITEKING;
    auto q = BLACKQUEEN;
    m_chessboard.PlacePiece(K, e4);
    m_chessboard.PlacePiece(q, d5);
    Move move(e4, d5);

    EXPECT_EQ(K, m_chessboard.readTile(e4).readPiece());
    EXPECT_EQ(q, m_chessboard.readTile(d5).readPiece());

    // do
    bool result = m_chessboard.MakeMove(move);
    EXPECT_TRUE(result);
    EXPECT_EQ(K, move.Piece);
    EXPECT_EQ(MoveFlag::Capture, move.Flags & MoveFlag::Capture);

    ChessPiece exp; // default, "empty" piece
    EXPECT_EQ(K, m_chessboard.readTile(d5).readPiece());
    EXPECT_EQ(exp, m_chessboard.readTile(e4).readPiece());
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
TEST_F(MoveFixture, EnPassant_Captured)
{   
    auto P = WHITEPAWN;
    auto p = BLACKPAWN;
    m_chessboard.PlacePiece(P, e2);
    m_chessboard.PlacePiece(p, d4);
    Move move(e2, e4);

    // do
    bool result = m_chessboard.MakeMove(move);
    EXPECT_TRUE(result);
    EXPECT_EQ(P, move.Piece);
    EXPECT_EQ(MoveFlag::EnPassant, move.Flags);

    EXPECT_EQ(e3, m_chessboard.readEnPassant());
    EXPECT_EQ(P, m_chessboard.readTile(e4).readPiece());
    EXPECT_EQ(p, m_chessboard.readTile(d4).readPiece());

    Move epCapture(d4,e3);

    result = m_chessboard.MakeMove(epCapture);
    EXPECT_TRUE(result);
    EXPECT_EQ(p, epCapture.Piece);
    EXPECT_EQ(MoveFlag::Capture, epCapture.Flags);

    ChessPiece exp; // default, "empty" piece
    EXPECT_EQ(exp, m_chessboard.readTile(e4).readPiece());
    EXPECT_EQ(exp, m_chessboard.readTile(d4).readPiece());
    EXPECT_EQ(p, m_chessboard.readTile(e3).readPiece());
}

}