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
            EXPECT_EQ(WHITEPAWN, mv.CapturedPiece);
            EXPECT_EQ(MoveFlag::Capture, mv.Flags & MoveFlag::Capture);
            EXPECT_EQ(MoveFlag::EnPassant, mv.Flags & MoveFlag::EnPassant);
            founde3 = true;
        }
        else
        {
            EXPECT_EQ(d3, mv.TargetSquare);
            EXPECT_EQ(exp, mv.CapturedPiece); // no capture
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
TEST_F(UnmakeFixture, Pawn_Promotion_Unmake)
{
    auto P = WHITEPAWN;
    auto n = BLACKKNIGHT;

    m_chessboard.PlacePiece(P, e7);
    m_chessboard.PlacePiece(n, d8);
    Move move(e7, e8); // promote
    move.PromoteToPiece = WHITEQUEEN;
    u64 hash = m_chessboard.readHash();

    // do
    bool result = m_chessboard.MakeMove(move);

    // validate
    EXPECT_TRUE(result);
    EXPECT_EQ(P, move.Piece);
    EXPECT_EQ(MoveFlag::Promotion, move.Flags);
    EXPECT_NE(hash, m_chessboard.readHash());

    auto Q = WHITEQUEEN;
    EXPECT_EQ(Q, m_chessboard.readTile(e8).readPiece());
     
    // undo
    result = m_chessboard.UnmakeMove(move);

    // validate
    EXPECT_TRUE(result);
    EXPECT_EQ(P, m_chessboard.readTile(e7).readPiece());
    EXPECT_EQ(ChessPiece::None(), m_chessboard.readTile(e8).readPiece());
    EXPECT_EQ(n, m_chessboard.readTile(d8).readPiece());
    EXPECT_EQ(hash, m_chessboard.readHash());

    // setup
    auto moves = m_chessboard.GetAvailableMoves(Set::WHITE);    
    std::map<Notation, int> moveMap {{e8, 0}, {d8, 0}};
    std::map<ChessPiece, int> promoteMap {{WHITEKNIGHT, 0}, {WHITEBISHOP, 0}, {WHITEROOK, 0}, {WHITEQUEEN, 0}};
    
    // validate
    EXPECT_EQ(8, moves.size());
    for (auto mv : moves)
    {
        // do
        result = m_chessboard.MakeMove(mv);
        EXPECT_TRUE(result);

        EXPECT_TRUE(moveMap.find(mv.TargetSquare) != moveMap.end());
        moveMap.at(mv.TargetSquare)++;

        EXPECT_TRUE(promoteMap.find(mv.PromoteToPiece) != promoteMap.end());
        promoteMap.at(mv.PromoteToPiece)++;

        // do
        m_chessboard.UnmakeMove(mv);
        EXPECT_EQ(BLACKKNIGHT, m_chessboard.readTile(d8).readPiece());
        EXPECT_EQ(WHITEPAWN, m_chessboard.readTile(e7).readPiece());
    }

    EXPECT_EQ(4, moveMap[d8]);
    EXPECT_EQ(4, moveMap[e8]);

    EXPECT_EQ(2, promoteMap[WHITEKNIGHT]);
    EXPECT_EQ(2, promoteMap[WHITEBISHOP]);
    EXPECT_EQ(2, promoteMap[WHITEROOK]);
    EXPECT_EQ(2, promoteMap[WHITEQUEEN]);


}
////////////////////////////////////////////////////////////////

} // namespace ElephantTest