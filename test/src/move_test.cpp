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
// 1. e4 d3
TEST_F(MoveFixture, EnPassant_Ignored_ResetFlag)
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

    Move pMove(d4,d3);

    result = m_chessboard.MakeMove(pMove);
    EXPECT_TRUE(result);
    EXPECT_EQ(p, pMove.Piece);
    EXPECT_EQ(MoveFlag::Zero, pMove.Flags);
    EXPECT_EQ(Notation(), m_chessboard.readEnPassant());

    ChessPiece exp; // default, "empty" piece
    EXPECT_EQ(P, m_chessboard.readTile(e4).readPiece());
    EXPECT_EQ(exp, m_chessboard.readTile(d4).readPiece());
    EXPECT_EQ(p, m_chessboard.readTile(d3).readPiece());
}



// 8 [   ][   ][   ][ n ][   ][   ][   ][   ]
// 7 [   ][   ][   ][   ][ P ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][   ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// Moves:
// e8=Q
TEST_F(MoveFixture, Pawn_Promotion)
{
    auto P = WHITEPAWN;
    auto n = BLACKKNIGHT;
    
    m_chessboard.PlacePiece(P, e7);
    m_chessboard.PlacePiece(n, d8);
    Move move(e7, e8); // promote
    move.Promote = WHITEQUEEN;
    
    // do
    bool result = m_chessboard.MakeMove(move);
    EXPECT_TRUE(result);
    EXPECT_EQ(P, move.Piece);
    EXPECT_EQ(MoveFlag::Promotion, move.Flags);

    auto Q = WHITEQUEEN;
    EXPECT_EQ(Q, m_chessboard.readTile(e8).readPiece());
}

// 8 [   ][   ][   ][ n ][   ][   ][   ][   ]
// 7 [   ][   ][   ][   ][ P ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][   ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// Moves:
// exd8=R
TEST_F(MoveFixture, Pawn_Promotion_Capture)
{
    auto P = WHITEPAWN;
    auto n = BLACKKNIGHT;
    
    m_chessboard.PlacePiece(P, e7);
    m_chessboard.PlacePiece(n, d8);
    Move move(e7, d8); // promote
    move.Promote = WHITEROOK;
    
    // do
    bool result = m_chessboard.MakeMove(move);
    EXPECT_TRUE(result);
    EXPECT_EQ(P, move.Piece);
    EXPECT_EQ(MoveFlag::Promotion, move.Flags & MoveFlag::Promotion);
    EXPECT_EQ(MoveFlag::Capture, move.Flags & MoveFlag::Capture);

    auto R = WHITEROOK;
    EXPECT_EQ(R, m_chessboard.readTile(d8).readPiece());
}

// 8 [   ][   ][   ][   ][   ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][ p ][   ][   ][   ]
// 1 [   ][   ][   ][ Q ][   ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// Moves:
// exd1=q
TEST_F(MoveFixture, Black_Pawn_Promotion_Capture)
{
    auto p = BLACKPAWN;
    auto Q = WHITEQUEEN;
    
    m_chessboard.PlacePiece(p, e2);
    m_chessboard.PlacePiece(Q, d1);
    Move move(e2, d1); // promote
    move.Promote = BLACKQUEEN;
    
    // do
    bool result = m_chessboard.MakeMove(move);
    EXPECT_TRUE(result);
    EXPECT_EQ(p, move.Piece);
    EXPECT_EQ(MoveFlag::Promotion, move.Flags & MoveFlag::Promotion);
    EXPECT_EQ(MoveFlag::Capture, move.Flags & MoveFlag::Capture);

    auto q = BLACKQUEEN;
    EXPECT_EQ(q, m_chessboard.readTile(d1).readPiece());
}

// 8 [ r ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][   ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// Moves:
// O-O-O
TEST_F(MoveFixture, Black_King_Castle_QueenSide)
{   
    auto k = BLACKKING;
    auto r = BLACKROOK;
    m_chessboard.editCastlingState() = 0x08; // black queen side castling available
    m_chessboard.PlacePiece(k, e8);
    m_chessboard.PlacePiece(r, a8);
    Move move(e8, c8); // castle

    // do
    bool result = m_chessboard.MakeMove(move);
    EXPECT_TRUE(result);
    EXPECT_EQ(k, move.Piece);
    EXPECT_EQ(MoveFlag::Castle, move.Flags);

    EXPECT_EQ(k, m_chessboard.readTile(c8).readPiece());
    EXPECT_EQ(r, m_chessboard.readTile(d8).readPiece());
}

// 8 [ r ][   ][   ][   ][ k ][   ][   ][ r ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [ R ][   ][   ][   ][ K ][   ][   ][ R ]
//     A    B    C    D    E    F    G    H
// Moves:
// 1. O-O-O Ra6
// 2. Rh3 O-O
//
// Result:
// 8 [   ][   ][   ][   ][   ][ r ][ k ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [ r ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][ R ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][ K ][ R ][   ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
TEST_F(MoveFixture, Castling)
{   
    auto k = BLACKKING;
    auto r = BLACKROOK;
    auto K = WHITEKING;
    auto R = WHITEROOK;

    byte expectedCastling = 0xf; // all castling available
    m_chessboard.editCastlingState() = expectedCastling;
    m_chessboard.PlacePiece(k, e8);
    m_chessboard.PlacePiece(r, a8);
    m_chessboard.PlacePiece(r, h8);

    m_chessboard.PlacePiece(K, e1);
    m_chessboard.PlacePiece(R, a1);
    m_chessboard.PlacePiece(R, h1);

    Move move(e1, c1); // castle

    // do
    bool result = m_chessboard.MakeMove(move);
    EXPECT_TRUE(result);
    EXPECT_EQ(K, move.Piece);
    EXPECT_EQ(MoveFlag::Castle, move.Flags);

    expectedCastling = 12;
    EXPECT_EQ(K, m_chessboard.readTile(c1).readPiece());
    EXPECT_EQ(R, m_chessboard.readTile(d1).readPiece());
    EXPECT_EQ(R, m_chessboard.readTile(h1).readPiece());
    EXPECT_EQ(expectedCastling, m_chessboard.readCastlingState()); 

    Move rMove(a8, a6);
    result = m_chessboard.MakeMove(rMove);
    EXPECT_TRUE(result);
    expectedCastling = 4;
    EXPECT_EQ(expectedCastling, m_chessboard.readCastlingState());

    Move RMove(h1, h3);
    result = m_chessboard.MakeMove(RMove);
    EXPECT_TRUE(result);
    expectedCastling = 4;
    EXPECT_EQ(expectedCastling, m_chessboard.readCastlingState());
    
    Move scndCastle(e8, g8);
    result = m_chessboard.MakeMove(scndCastle);
    EXPECT_TRUE(result);
    EXPECT_EQ(k, scndCastle.Piece);
    EXPECT_EQ(MoveFlag::Castle, scndCastle.Flags);
    expectedCastling = 0;
    EXPECT_EQ(expectedCastling, m_chessboard.readCastlingState());
    EXPECT_EQ(k, m_chessboard.readTile(g8).readPiece());
    EXPECT_EQ(r, m_chessboard.readTile(f8).readPiece());
}

}