#include <gtest/gtest.h>
#include "elephant_test_utils.h"
#include "fen_parser.h"
#include "game_context.h"
#include "move.h"
#include "chessboard.h"
#include "chess_piece.h"
#include "hash_zorbist.h"


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
    EXPECT_NE(MoveFlag::EnPassant, MoveFlag::EnPassant & move.Flags);
    
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
    EXPECT_NE(MoveFlag::EnPassant, MoveFlag::EnPassant & move.Flags);
    
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

    u64 hash = ZorbistHash::Instance().HashBoard(m_chessboard);
    EXPECT_EQ(hash, m_chessboard.readHash());


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
    EXPECT_NE(MoveFlag::EnPassant, move.Flags & MoveFlag::EnPassant);

    EXPECT_EQ(e3, m_chessboard.readEnPassant());
    EXPECT_EQ(P, m_chessboard.readTile(e4).readPiece());
    EXPECT_EQ(p, m_chessboard.readTile(d4).readPiece());

    Move epCapture(d4,e3);

    result = m_chessboard.MakeMove(epCapture);
    EXPECT_TRUE(result);
    EXPECT_EQ(p, epCapture.Piece);
    EXPECT_EQ(MoveFlag::Capture, epCapture.Flags & MoveFlag::Capture);
    EXPECT_EQ(MoveFlag::EnPassant, epCapture.Flags & MoveFlag::EnPassant);

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
    EXPECT_NE(MoveFlag::EnPassant, move.Flags & MoveFlag::EnPassant);

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
    move.PromoteToPiece = WHITEQUEEN;
    
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
    move.PromoteToPiece = WHITEROOK;
    
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
    move.PromoteToPiece = BLACKQUEEN;
    
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
    m_chessboard.setCastlingState(0x08); // black queen side castling available
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
    m_chessboard.setCastlingState(expectedCastling);
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

// 8 [ r ][ n ][ b ][ q ][ k ][ b ][ n ][ r ]
// 7 [ p ][ p ][ p ][   ][ p ][ p ][ p ][ p ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][ p ][ P ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [ P ][ P ][   ][   ][ P ][ P ][ P ][ P ]
// 1 [ R ][ N ][ B ][ Q ][ K ][ B ][ N ][ R ]
//     A    B    C    D    E    F    G    H
// 1.d4 d5 2.c4 dxc4
TEST_F(MoveFixture, BuildMoveSequence_QueensGambitAccepted)
{
    std::string movePNG = "1.d4 d5 2.c4 dxc4";

    std::vector<Move> moves;
    Move::ParsePNG(movePNG, moves);
}

// https://en.wikipedia.org/wiki/Portable_Game_Notation
// 1. e4 e5 2. Nf3 Nc6 3. Bb5 a6 {This opening is called the Ruy Lopez.}
// 4. Ba4 Nf6 5. O - O Be7
TEST_F(MoveFixture, BuildMoveSequence_FischerSpassky)
{
    std::string movePNG = "1. e4 e5 2. Nf3 Nc6 3. Bb5 a6 4. Ba4 Nf6";
    std::vector<Move> moves;
    Move::ParsePNG(movePNG, moves);

    EXPECT_EQ(moves.size(), 8);

    {
        const int index = 0;
        const auto& mv = moves[index];
        EXPECT_EQ(WHITEPAWN, mv.Piece);
        EXPECT_EQ(e4, mv.TargetSquare);
        EXPECT_EQ(&moves[index + 1], mv.NextMove);
        EXPECT_EQ(nullptr, mv.PrevMove);
    }

    {
        const int index = 1;
        const auto& mv = moves[index];
        EXPECT_EQ(BLACKPAWN, mv.Piece);
        EXPECT_EQ(e5, mv.TargetSquare);
        EXPECT_EQ(&moves[index + 1], mv.NextMove);
        EXPECT_EQ(&moves[index - 1], mv.PrevMove);
    }

    {
        const int index = 2;
        const auto& mv = moves[index];
        EXPECT_EQ(WHITEKNIGHT, mv.Piece);
        EXPECT_EQ(f3, mv.TargetSquare);
        EXPECT_EQ(&moves[index+1], mv.NextMove);
        EXPECT_EQ(&moves[index-1], mv.PrevMove);
    }

    {
        const int index = 3;
        const auto& mv = moves[index];
        EXPECT_EQ(BLACKKNIGHT, mv.Piece);
        EXPECT_EQ(c6, mv.TargetSquare);
        EXPECT_EQ(&moves[index + 1], mv.NextMove);
        EXPECT_EQ(&moves[index - 1], mv.PrevMove);
    }

    {
        const int index = 4;
        const auto& mv = moves[index];
        EXPECT_EQ(WHITEBISHOP, mv.Piece);
        EXPECT_EQ(b5, mv.TargetSquare);
        EXPECT_EQ(&moves[index + 1], mv.NextMove);
        EXPECT_EQ(&moves[index - 1], mv.PrevMove);
    }

    {
        const int index = 5;
        const auto& mv = moves[index];
        EXPECT_EQ(BLACKPAWN, mv.Piece);
        EXPECT_EQ(a6, mv.TargetSquare);
        EXPECT_EQ(&moves[index + 1], mv.NextMove);
        EXPECT_EQ(&moves[index - 1], mv.PrevMove);
    }

    {
        const int index = 6;
        const auto& mv = moves[index];
        EXPECT_EQ(WHITEBISHOP, mv.Piece);
        EXPECT_EQ(a4, mv.TargetSquare);
        EXPECT_EQ(&moves[index + 1], mv.NextMove);
        EXPECT_EQ(&moves[index - 1], mv.PrevMove);
    }

    {
        const int index = 7;
        const auto& mv = moves[index];
        EXPECT_EQ(BLACKKNIGHT, mv.Piece);
        EXPECT_EQ(f6, mv.TargetSquare);
        EXPECT_EQ(nullptr, mv.NextMove);
        EXPECT_EQ(&moves[index - 1], mv.PrevMove);
    }

}
TEST_F(MoveFixture, BuildMoveSequence_FischerSpassky_CommentAndCastling)
{
    std::string movePNG = "1. e4 { This is a comment. }e5 2. O-O-O O-O 3. Bxb5 a6{ This opening is called the Ruy Lopez. } 4. Ba4+ Nf6 5. O-O Be7 6. de5 Bxe5+";
    std::vector<Move> moves;
    auto comments = Move::ParsePNG(movePNG, moves);

    EXPECT_EQ(2, comments.size());
    EXPECT_EQ(12, moves.size());
    EXPECT_EQ(MoveFlag::Castle, moves[2].Flags & MoveFlag::Castle);
    EXPECT_EQ(MoveFlag::Castle, moves[3].Flags & MoveFlag::Castle);
    EXPECT_EQ(MoveFlag::Castle, moves[8].Flags & MoveFlag::Castle);
    EXPECT_EQ(MoveFlag::Castle, moves[3].Flags & MoveFlag::Castle);


    EXPECT_EQ(MoveFlag::Capture, moves[4].Flags & MoveFlag::Capture);
    EXPECT_EQ(MoveFlag::Capture, moves[10].Flags & MoveFlag::Capture);
    EXPECT_EQ(MoveFlag::Capture, moves[11].Flags & MoveFlag::Capture);

    EXPECT_EQ(MoveFlag::Check, moves[6].Flags & MoveFlag::Check);
    EXPECT_EQ(MoveFlag::Check, moves[11].Flags & MoveFlag::Check);

    EXPECT_EQ(WHITEPAWN, moves[10].Piece);
}

TEST_F(MoveFixture, BuildMoveSequence_DisambiguatingMoves)
{
    std::string movePNG = "9. h3 R1a3 10. d4 Nbd7 23. Rae8 Ne5 24. Qh4e1 25. e1 e2 26. Ra1xa2+ e2";
    std::vector<Move> moves;
    Move::ParsePNG(movePNG, moves);

    { // h3
        const int index = 0;
        const auto& mv = moves[index];
        EXPECT_EQ(WHITEPAWN, mv.Piece);
        EXPECT_EQ(h3, mv.TargetSquare);        
    }

    { // R1a3
        const int index = 1;
        const auto& mv = moves[index];
        EXPECT_EQ(BLACKROOK, mv.Piece);
        EXPECT_EQ(a3, mv.TargetSquare);
        EXPECT_EQ(Notation(9, 0), mv.SourceSquare);
    }

    { // d4
        const int index = 2;
        const auto& mv = moves[index];
        EXPECT_EQ(WHITEPAWN, mv.Piece);
        EXPECT_EQ(d4, mv.TargetSquare);        
    }

    { // Nbd7
        const int index = 3;
        const auto& mv = moves[index];
        EXPECT_EQ(BLACKKNIGHT, mv.Piece);
        EXPECT_EQ(d7, mv.TargetSquare);
        EXPECT_EQ(Notation(1, 9), mv.SourceSquare);
    }

    { // Rae8
        const int index = 4;
        const auto& mv = moves[index];
        EXPECT_EQ(WHITEROOK, mv.Piece);
        EXPECT_EQ(e8, mv.TargetSquare);
        EXPECT_EQ(Notation(0, 9), mv.SourceSquare);
    }

    { // Ne5
        const int index = 5;
        const auto& mv = moves[index];
        EXPECT_EQ(BLACKKNIGHT, mv.Piece);
        EXPECT_EQ(e5, mv.TargetSquare);
    }

    { // Qh4e1
        const int index = 6;
        const auto& mv = moves[index];
        EXPECT_EQ(WHITEQUEEN, mv.Piece);
        EXPECT_EQ(e1, mv.TargetSquare);
        EXPECT_EQ(h4, mv.SourceSquare);
    }

    { // e1
        const int index = 7;
        const auto& mv = moves[index];
        EXPECT_EQ(WHITEPAWN, mv.Piece);
        EXPECT_EQ(e1, mv.TargetSquare);
    }

    { // e2
        const int index = 8;
        const auto& mv = moves[index];
        EXPECT_EQ(BLACKPAWN, mv.Piece);
        EXPECT_EQ(e2, mv.TargetSquare);
    }

    { // Ra1xa2+
        const int index = 9;
        const auto& mv = moves[index];
        EXPECT_EQ(WHITEROOK, mv.Piece);
        EXPECT_EQ(a2, mv.TargetSquare);
        EXPECT_EQ(a1, mv.SourceSquare);

        EXPECT_EQ(MoveFlag::Check, moves[index].Flags & MoveFlag::Check);
        EXPECT_EQ(MoveFlag::Capture, moves[index].Flags & MoveFlag::Capture);

    }

    { // e2
        const int index = 10;
        const auto& mv = moves[index];
        EXPECT_EQ(BLACKPAWN, mv.Piece);
        EXPECT_EQ(e2, mv.TargetSquare);
    }

}

TEST_F(MoveFixture, BuildMoveSequence_PawnPromotion)
{
    std::string movePNG = "9. e8=Q";
    std::vector<Move> moves;
    Move::ParsePNG(movePNG, moves);
}

TEST_F(MoveFixture, BrokenMove_QueenShouldBeAbleToCaptureBishopOne7)
{
    std::string fen = "r2q1rk1/1b1nBpp1/3p3p/2p1P3/1p2n3/1B3N1P/PP3PP1/RN1QR1K1 w - - 34 18";
    GameContext context;

    EXPECT_TRUE(FENParser::deserialize(fen.c_str(), context));

    Move move(d8, e7);
    context.editChessboard().MakeMove(move);
}

}