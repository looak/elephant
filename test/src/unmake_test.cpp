#include <gtest/gtest.h>
#include "chess_piece.h"
#include "chessboard.h"
#include "elephant_test_utils.h"
#include "move.h"

namespace ElephantTest {
////////////////////////////////////////////////////////////////
class UnmakeFixture : public ::testing::Test {
public:
    virtual void SetUp(){

    };
    virtual void TearDown(){};

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

    {
        const auto& whitePawns = m_chessboard.readMaterial(Set::WHITE).buildPlacementsOfPiece(WHITEPAWN);
        const auto& blackPawns = m_chessboard.readMaterial(Set::BLACK).buildPlacementsOfPiece(BLACKPAWN);

        // validate
        EXPECT_EQ(1, whitePawns.size());
        EXPECT_EQ(1, blackPawns.size());
        EXPECT_EQ(e2, whitePawns[0]);
        EXPECT_EQ(d4, blackPawns[0]);
    }

    // do
    bool result = m_chessboard.MakeMove(move);

    // validate
    EXPECT_TRUE(result);
    EXPECT_EQ(P, move.Piece);
    /**
     * En passant is a special pawn capture move in chess where a pawn captures an opposing pawn
     * that has just advanced two squares from its starting position, as if it had only advanced
     * one square. The capturing pawn moves diagonally to the square that the opposing pawn passed
     * over, and the captured pawn is removed from the board.
     */
    EXPECT_NE(MoveFlag::EnPassant, move.Flags & MoveFlag::EnPassant);
    EXPECT_FALSE(move.isEnPassant());

    EXPECT_EQ(e3, m_chessboard.readEnPassant());
    EXPECT_EQ(P, m_chessboard.readTile(e4).readPiece());
    EXPECT_EQ(p, m_chessboard.readTile(d4).readPiece());

    {
        const auto& whitePawns = m_chessboard.readMaterial(Set::WHITE).buildPlacementsOfPiece(WHITEPAWN);
        const auto& blackPawns = m_chessboard.readMaterial(Set::BLACK).buildPlacementsOfPiece(BLACKPAWN);

        EXPECT_EQ(1, whitePawns.size());
        EXPECT_EQ(1, blackPawns.size());
        EXPECT_EQ(e4, whitePawns[0]);
        EXPECT_EQ(d4, blackPawns[0]);
    }

    // setup
    Move epCapture(d4, e3);

    // do
    result = m_chessboard.MakeMove(epCapture);

    // validate
    EXPECT_TRUE(result);
    EXPECT_EQ(p, epCapture.Piece);
    EXPECT_EQ(MoveFlag::Capture, epCapture.Flags & MoveFlag::Capture);
    EXPECT_EQ(MoveFlag::EnPassant, epCapture.Flags & MoveFlag::EnPassant);

    ChessPiece exp;  // default, "empty" piece
    EXPECT_EQ(exp, m_chessboard.readTile(e4).readPiece());
    EXPECT_EQ(exp, m_chessboard.readTile(d4).readPiece());
    EXPECT_EQ(p, m_chessboard.readTile(e3).readPiece());

    {
        const auto& whitePawns = m_chessboard.readMaterial(Set::WHITE).buildPlacementsOfPiece(WHITEPAWN);
        const auto& blackPawns = m_chessboard.readMaterial(Set::BLACK).buildPlacementsOfPiece(BLACKPAWN);

        EXPECT_EQ(0, whitePawns.size());
        EXPECT_EQ(1, blackPawns.size());
        EXPECT_EQ(e3, blackPawns[0]);
    }

    // do
    result = m_chessboard.UnmakeMove(epCapture);

    // validate
    EXPECT_TRUE(result);
    EXPECT_EQ(e3, m_chessboard.readEnPassant());
    EXPECT_EQ(P, m_chessboard.readTile(e4).readPiece());
    EXPECT_EQ(p, m_chessboard.readTile(d4).readPiece());
    EXPECT_EQ(exp, m_chessboard.readTile(e3).readPiece());
    {
        const auto& whitePawns = m_chessboard.readMaterial(Set::WHITE).buildPlacementsOfPiece(WHITEPAWN);
        const auto& blackPawns = m_chessboard.readMaterial(Set::BLACK).buildPlacementsOfPiece(BLACKPAWN);

        EXPECT_EQ(1, whitePawns.size());
        EXPECT_EQ(1, blackPawns.size());
        EXPECT_EQ(e4, whitePawns[0]);
        EXPECT_EQ(d4, blackPawns[0]);
    }
    // setup
    auto moves = m_chessboard.GetAvailableMoves(Set::BLACK);
    bool found_e3 = false;
    bool found_d3 = false;

    // validate
    EXPECT_EQ(2, moves.size());
    for (auto mv : moves) {
        // do
        result = m_chessboard.MakeMove(mv);
        EXPECT_TRUE(result);

        // validate
        if (mv.TargetSquare == e3) {
            EXPECT_EQ(WHITEPAWN, mv.CapturedPiece);
            EXPECT_EQ(MoveFlag::Capture, mv.Flags & MoveFlag::Capture);
            EXPECT_EQ(MoveFlag::EnPassant, mv.Flags & MoveFlag::EnPassant);
            found_e3 = true;
        }
        else {
            EXPECT_EQ(d3, mv.TargetSquare);
            EXPECT_EQ(exp, mv.CapturedPiece);  // no capture
            EXPECT_EQ(MoveFlag::Zero, mv.Flags);
            found_d3 = true;
        }

        // do
        m_chessboard.UnmakeMove(mv);
        EXPECT_EQ(e3, m_chessboard.readEnPassant());
    }

    // validate
    EXPECT_TRUE(found_e3);
    EXPECT_TRUE(found_d3);
}
/**
 * 8 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 7 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 6 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 5 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 4 [   ][   ][   ][ p ][   ][   ][ p ][   ]
 * 3 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 2 [   ][   ][ P ][   ][   ][ P ][   ][   ]
 * 1 [   ][   ][   ][   ][   ][   ][   ][   ]
 *     A    B    C    D    E    F    G    H */
TEST_F(UnmakeFixture, UnmakeEnPassantMoves_VariousPositions_CorrectUndo)
{
    // setup
    auto P = WHITEPAWN;
    auto p = BLACKPAWN;

    m_chessboard.PlacePiece(P, c2);
    m_chessboard.PlacePiece(P, f2);
    m_chessboard.PlacePiece(p, d4);
    m_chessboard.PlacePiece(p, g4);

    const auto& whitePawns = m_chessboard.readMaterial(Set::WHITE).buildPlacementsOfPiece(WHITEPAWN);
    const auto& blackPawns = m_chessboard.readMaterial(Set::BLACK).buildPlacementsOfPiece(BLACKPAWN);

    // do
    auto whiteMoves = m_chessboard.GetAvailableMoves(Set::WHITE);

    for (auto wmv : whiteMoves) {
        m_chessboard.MakeMove(wmv);
        auto blackMoves = m_chessboard.GetAvailableMoves(Set::BLACK);

        for (auto bmv : blackMoves) {
            m_chessboard.MakeMove(bmv);
            m_chessboard.UnmakeMove(bmv);
        }

        m_chessboard.UnmakeMove(wmv);
    }

    // validate
    EXPECT_EQ(2, whitePawns.size());
    EXPECT_NE(whitePawns.end(), std::find(whitePawns.begin(), whitePawns.end(), c2));
    EXPECT_NE(whitePawns.end(), std::find(whitePawns.begin(), whitePawns.end(), f2));
    EXPECT_EQ(2, blackPawns.size());
    EXPECT_NE(blackPawns.end(), std::find(blackPawns.begin(), blackPawns.end(), d4));
    EXPECT_NE(blackPawns.end(), std::find(blackPawns.begin(), blackPawns.end(), g4));
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
    Move move(e7, e8);  // promote
    move.PromoteToPiece = WHITEQUEEN;
    u64 hash = m_chessboard.readHash();

    const auto& bitboard = m_chessboard.readBitboard();

    // do
    bool result = m_chessboard.MakeMove(move);

    // validate
    EXPECT_TRUE(result);
    EXPECT_EQ(P, move.Piece);
    EXPECT_EQ(MoveFlag::Promotion, move.Flags);
    EXPECT_NE(hash, m_chessboard.readHash());
    u64 pawnMask = bitboard.GetMaterial(WHITEPAWN).read();
    EXPECT_EQ(0, pawnMask);

    auto Q = WHITEQUEEN;
    EXPECT_EQ(Q, m_chessboard.readTile(e8).readPiece());
    EXPECT_NE(0, bitboard.GetMaterial(WHITEQUEEN).read());

    // undo
    result = m_chessboard.UnmakeMove(move);

    // validate
    EXPECT_TRUE(result);
    EXPECT_EQ(P, m_chessboard.readTile(e7).readPiece());
    EXPECT_EQ(ChessPiece::None(), m_chessboard.readTile(e8).readPiece());
    EXPECT_EQ(n, m_chessboard.readTile(d8).readPiece());
    EXPECT_EQ(hash, m_chessboard.readHash());

    EXPECT_EQ(0, bitboard.GetMaterial(WHITEQUEEN).read());
    EXPECT_NE(0, bitboard.GetMaterial(WHITEPAWN).read());

    // setup
    auto moves = m_chessboard.GetAvailableMoves(Set::WHITE);
    std::map<Notation, int> moveMap{{e8, 0}, {d8, 0}};
    std::map<ChessPiece, int> promoteMap{{WHITEKNIGHT, 0}, {WHITEBISHOP, 0}, {WHITEROOK, 0}, {WHITEQUEEN, 0}};

    // validate
    EXPECT_EQ(8, moves.size());
    for (auto mv : moves) {
        // do
        result = m_chessboard.MakeMove(mv);
        EXPECT_TRUE(result);

        EXPECT_TRUE(moveMap.find(mv.TargetSquare) != moveMap.end());
        moveMap.at(mv.TargetSquare)++;

        EXPECT_TRUE(promoteMap.find(mv.PromoteToPiece) != promoteMap.end());
        promoteMap.at(mv.PromoteToPiece)++;

        // do
        m_chessboard.UnmakeMove(mv);
        EXPECT_EQ(BLACKKNIGHT, m_chessboard.readPieceAt(d8));
        EXPECT_EQ(WHITEPAWN, m_chessboard.readPieceAt(e7));
    }

    EXPECT_EQ(4, moveMap[d8]);
    EXPECT_EQ(4, moveMap[e8]);

    EXPECT_EQ(2, promoteMap[WHITEKNIGHT]);
    EXPECT_EQ(2, promoteMap[WHITEBISHOP]);
    EXPECT_EQ(2, promoteMap[WHITEROOK]);
    EXPECT_EQ(2, promoteMap[WHITEQUEEN]);
}

// 8 [ r ][   ][   ][   ][ k ][   ][   ][ r ]
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
TEST_F(UnmakeFixture, King_CastlingQueenSide_Black_Unmake)
{
    auto k = BLACKKING;
    auto r = BLACKROOK;
    auto empty = ChessPiece();
    m_chessboard.setCastlingState(12);  // black king & queen side castling available
    m_chessboard.PlacePiece(k, e8);
    m_chessboard.PlacePiece(r, a8);
    m_chessboard.PlacePiece(r, h8);
    Move move(e8, c8);  // castle

    u64 hash = m_chessboard.readHash();

    // do
    bool result = m_chessboard.MakeMove(move);
    EXPECT_TRUE(result);
    EXPECT_EQ(k, move.Piece);
    EXPECT_EQ(MoveFlag::Castle, move.Flags);

    EXPECT_EQ(k, m_chessboard.readTile(c8).readPiece());
    EXPECT_EQ(r, m_chessboard.readTile(d8).readPiece());
    EXPECT_NE(hash, m_chessboard.readHash());

    // unmake
    result = m_chessboard.UnmakeMove(move);

    EXPECT_TRUE(result);
    EXPECT_EQ(12, m_chessboard.readCastlingState());
    EXPECT_EQ(k, m_chessboard.readTile(e8).readPiece());
    EXPECT_EQ(r, m_chessboard.readTile(a8).readPiece());
    EXPECT_EQ(empty, m_chessboard.readTile(c8).readPiece());
    EXPECT_EQ(empty, m_chessboard.readTile(d8).readPiece());
    EXPECT_EQ(hash, m_chessboard.readHash());

    // validate available moves
    auto moves = m_chessboard.GetAvailableMoves(e8, k, 0, KingMask(), KingMask());

    EXPECT_EQ(7, moves.size());

    Move rooKMove(a8, a1);
    m_chessboard.MakeMove(rooKMove);

    EXPECT_EQ(r, m_chessboard.readPieceAt(a1));
    EXPECT_EQ(empty, m_chessboard.readPieceAt(a8));
    EXPECT_EQ(4, m_chessboard.readCastlingState());

    moves = m_chessboard.GetAvailableMoves(e8, k, 0, KingMask(), KingMask());
    EXPECT_EQ(6, moves.size());

    // unmake
    result = m_chessboard.UnmakeMove(rooKMove);
    EXPECT_TRUE(result);
    EXPECT_EQ(12, m_chessboard.readCastlingState());
    EXPECT_EQ(r, m_chessboard.readPieceAt(a8));
    EXPECT_EQ(empty, m_chessboard.readPieceAt(a1));

    moves = m_chessboard.GetAvailableMoves(e8, k, 0, KingMask(), KingMask());
    EXPECT_EQ(7, moves.size());

    Move hrookMove(h8, h1);
    m_chessboard.MakeMove(hrookMove);

    EXPECT_EQ(r, m_chessboard.readPieceAt(h1));
    EXPECT_EQ(empty, m_chessboard.readPieceAt(h8));
    EXPECT_EQ(8, m_chessboard.readCastlingState());

    moves = m_chessboard.GetAvailableMoves(e8, k, 0, KingMask(), KingMask());
    EXPECT_EQ(6, moves.size());

    // unmake
    result = m_chessboard.UnmakeMove(hrookMove);
    EXPECT_TRUE(result);
    EXPECT_EQ(12, m_chessboard.readCastlingState());
    EXPECT_EQ(r, m_chessboard.readPieceAt(h8));
    EXPECT_EQ(empty, m_chessboard.readPieceAt(h1));

    moves = m_chessboard.GetAvailableMoves(e8, k, 0, KingMask(), KingMask());
    EXPECT_EQ(7, moves.size());
}

// 8 [   ][   ][   ][   ][   ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [ R ][   ][   ][   ][ K ][   ][   ][ R ]
//     A    B    C    D    E    F    G    H
// Moves:
// O-O-O
TEST_F(UnmakeFixture, King_CastlingQueenSide_White_Unmake)
{
    auto k = WHITEKING;
    auto r = WHITEROOK;
    auto empty = ChessPiece();
    m_chessboard.setCastlingState(3);  // black king & queen side castling available
    m_chessboard.PlacePiece(k, e1);
    m_chessboard.PlacePiece(r, a1);
    m_chessboard.PlacePiece(r, h1);
    Move move(e1, c1);  // castle

    u64 hash = m_chessboard.readHash();

    // do
    bool result = m_chessboard.MakeMove(move);
    EXPECT_TRUE(result);
    EXPECT_EQ(k, move.Piece);
    EXPECT_EQ(MoveFlag::Castle, move.Flags);

    EXPECT_EQ(k, m_chessboard.readTile(c1).readPiece());
    EXPECT_EQ(r, m_chessboard.readTile(d1).readPiece());
    EXPECT_NE(hash, m_chessboard.readHash());

    // unmake
    result = m_chessboard.UnmakeMove(move);

    EXPECT_TRUE(result);
    EXPECT_EQ(3, m_chessboard.readCastlingState());
    EXPECT_EQ(k, m_chessboard.readTile(e1).readPiece());
    EXPECT_EQ(r, m_chessboard.readTile(a1).readPiece());
    EXPECT_EQ(empty, m_chessboard.readTile(c1).readPiece());
    EXPECT_EQ(empty, m_chessboard.readTile(d1).readPiece());
    EXPECT_EQ(hash, m_chessboard.readHash());

    // validate available moves
    auto moves = m_chessboard.GetAvailableMoves(e1, k, 0, KingMask(), KingMask());

    EXPECT_EQ(7, moves.size());

    Move rooKMove(a1, a8);
    m_chessboard.MakeMove(rooKMove);

    EXPECT_EQ(r, m_chessboard.readPieceAt(a8));
    EXPECT_EQ(empty, m_chessboard.readPieceAt(a1));
    EXPECT_EQ(1, m_chessboard.readCastlingState());

    moves = m_chessboard.GetAvailableMoves(e1, k, 0, KingMask(), KingMask());
    EXPECT_EQ(6, moves.size());

    // unmake
    result = m_chessboard.UnmakeMove(rooKMove);
    EXPECT_TRUE(result);
    EXPECT_EQ(3, m_chessboard.readCastlingState());
    EXPECT_EQ(r, m_chessboard.readPieceAt(a1));
    EXPECT_EQ(empty, m_chessboard.readPieceAt(a8));

    moves = m_chessboard.GetAvailableMoves(e1, k, 0, KingMask(), KingMask());
    EXPECT_EQ(7, moves.size());

    Move hrookMove(h1, h8);
    m_chessboard.MakeMove(hrookMove);

    EXPECT_EQ(r, m_chessboard.readPieceAt(h8));
    EXPECT_EQ(empty, m_chessboard.readPieceAt(h1));
    EXPECT_EQ(2, m_chessboard.readCastlingState());

    moves = m_chessboard.GetAvailableMoves(e1, k, 0, KingMask(), KingMask());
    EXPECT_EQ(6, moves.size());

    // unmake
    result = m_chessboard.UnmakeMove(hrookMove);
    EXPECT_TRUE(result);
    EXPECT_EQ(3, m_chessboard.readCastlingState());
    EXPECT_EQ(r, m_chessboard.readPieceAt(h1));
    EXPECT_EQ(empty, m_chessboard.readPieceAt(h8));

    moves = m_chessboard.GetAvailableMoves(e1, k, 0, KingMask(), KingMask());
    EXPECT_EQ(7, moves.size());

    // move king
    Move kingMove(e1, e2);
    m_chessboard.MakeMove(kingMove);

    EXPECT_EQ(k, m_chessboard.readPieceAt(e2));
    EXPECT_EQ(empty, m_chessboard.readPieceAt(e1));
    EXPECT_EQ(0, m_chessboard.readCastlingState());

    m_chessboard.UnmakeMove(kingMove);
    EXPECT_EQ(k, m_chessboard.readPieceAt(e1));
    EXPECT_EQ(empty, m_chessboard.readPieceAt(e2));
    EXPECT_EQ(3, m_chessboard.readCastlingState());

    moves = m_chessboard.GetAvailableMoves(e1, k, 0, KingMask(), KingMask());
    EXPECT_EQ(7, moves.size());
}

TEST_F(UnmakeFixture, King_CastlingKingSide_UnmakeTest)
{
    auto k = BLACKKING;
    auto r = BLACKROOK;
    auto empty = ChessPiece();
    m_chessboard.setCastlingState(0x04);  // black king side castling available
    m_chessboard.PlacePiece(k, e8);
    m_chessboard.PlacePiece(r, h8);
    Move move(e8, g8);  // castle

    u64 hash = m_chessboard.readHash();

    // do
    bool result = m_chessboard.MakeMove(move);
    EXPECT_TRUE(result);
    EXPECT_EQ(k, move.Piece);
    EXPECT_EQ(MoveFlag::Castle, move.Flags);

    EXPECT_EQ(k, m_chessboard.readTile(g8).readPiece());
    EXPECT_EQ(r, m_chessboard.readTile(f8).readPiece());
    EXPECT_NE(hash, m_chessboard.readHash());

    // unmake
    result = m_chessboard.UnmakeMove(move);

    EXPECT_TRUE(result);
    EXPECT_EQ(4, m_chessboard.readCastlingState());
    EXPECT_EQ(k, m_chessboard.readTile(e8).readPiece());
    EXPECT_EQ(r, m_chessboard.readTile(h8).readPiece());
    EXPECT_EQ(empty, m_chessboard.readTile(g8).readPiece());
    EXPECT_EQ(empty, m_chessboard.readTile(f8).readPiece());
    EXPECT_EQ(hash, m_chessboard.readHash());
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
TEST_F(UnmakeFixture, Castling)
{
    auto k = BLACKKING;
    auto r = BLACKROOK;
    auto K = WHITEKING;
    auto R = WHITEROOK;

    byte expectedCastling = 0xf;  // all castling available
    m_chessboard.setCastlingState(expectedCastling);
    m_chessboard.PlacePiece(k, e8);
    m_chessboard.PlacePiece(r, a8);
    m_chessboard.PlacePiece(r, h8);

    m_chessboard.PlacePiece(K, e1);
    m_chessboard.PlacePiece(R, a1);
    m_chessboard.PlacePiece(R, h1);

    u64 orgHash = m_chessboard.readHash();
    u64 hash = orgHash;

    // O-O-O
    Move move(e1, c1);  // castle
    m_chessboard.MakeMove(move);
    EXPECT_NE(hash, m_chessboard.readHash());
    hash = m_chessboard.readHash();

    // Ra6
    Move rMove(a8, a6);
    m_chessboard.MakeMove(rMove);
    EXPECT_NE(hash, m_chessboard.readHash());
    hash = m_chessboard.readHash();

    // Rh3
    Move RMove(h1, h3);
    m_chessboard.MakeMove(RMove);
    EXPECT_NE(hash, m_chessboard.readHash());
    hash = m_chessboard.readHash();

    // O-O
    Move scndCastle(e8, g8);
    m_chessboard.MakeMove(scndCastle);
    EXPECT_NE(hash, m_chessboard.readHash());
    hash = m_chessboard.readHash();

    m_chessboard.UnmakeMove(scndCastle);
    EXPECT_NE(hash, m_chessboard.readHash());
    hash = m_chessboard.readHash();

    m_chessboard.UnmakeMove(RMove);
    EXPECT_NE(hash, m_chessboard.readHash());
    hash = m_chessboard.readHash();

    m_chessboard.UnmakeMove(rMove);
    EXPECT_NE(hash, m_chessboard.readHash());
    hash = m_chessboard.readHash();

    m_chessboard.UnmakeMove(move);
    EXPECT_NE(hash, m_chessboard.readHash());
    hash = m_chessboard.readHash();

    EXPECT_EQ(expectedCastling, m_chessboard.readCastlingState());
    EXPECT_EQ(k, m_chessboard.readTile(e8).readPiece());
    EXPECT_EQ(K, m_chessboard.readTile(e1).readPiece());
    EXPECT_EQ(r, m_chessboard.readTile(a8).readPiece());
    EXPECT_EQ(r, m_chessboard.readTile(h8).readPiece());
    EXPECT_EQ(R, m_chessboard.readTile(a1).readPiece());
    EXPECT_EQ(R, m_chessboard.readTile(h1).readPiece());
    EXPECT_EQ(orgHash, hash);
}
// 8 [ r ][   ][   ][   ][ k ][   ][   ][ r ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][ b ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [ R ][   ][   ][   ][ K ][   ][   ][ R ]
//     A    B    C    D    E    F    G    H
TEST_F(UnmakeFixture, Castling_Captures)
{
    auto k = BLACKKING;
    auto r = BLACKROOK;
    auto b = BLACKBISHOP;
    auto K = WHITEKING;
    auto R = WHITEROOK;

    byte expectedCastling = 0xf;  // all castling available
    m_chessboard.editCastlingState().setAll();
    EXPECT_EQ(expectedCastling, m_chessboard.readCastlingState());
    EXPECT_TRUE(m_chessboard.readCastlingStateInfo().hasAll());
    auto& castlingState = m_chessboard.editCastlingState();
    m_chessboard.PlacePiece(k, e8);
    m_chessboard.PlacePiece(r, a8);
    m_chessboard.PlacePiece(r, h8);
    m_chessboard.PlacePiece(b, f3);

    m_chessboard.PlacePiece(K, e1);
    m_chessboard.PlacePiece(R, a1);
    m_chessboard.PlacePiece(R, h1);

    u64 orgHash = m_chessboard.readHash();

    Move Bxh1(f3, h1);
    bool result = m_chessboard.MakeMove(Bxh1);
    EXPECT_TRUE(result);
    EXPECT_NE(orgHash, m_chessboard.readHash());
    EXPECT_FALSE(castlingState.hasWhiteKingSide());
    EXPECT_EQ(CastlingState::BLACK_ALL | CastlingState::WHITE_QUEENSIDE, m_chessboard.readCastlingState());
    EXPECT_EQ(BLACKBISHOP, m_chessboard.readPieceAt(h1));

    m_chessboard.UnmakeMove(Bxh1);
    EXPECT_EQ(orgHash, m_chessboard.readHash());
    EXPECT_TRUE(castlingState.hasAll());
    EXPECT_EQ(CastlingState::BLACK_ALL | CastlingState::WHITE_ALL, m_chessboard.readCastlingState());
    EXPECT_EQ(BLACKBISHOP, m_chessboard.readPieceAt(f3));
    EXPECT_EQ(WHITEROOK, m_chessboard.readPieceAt(h1));

    Move Rxa8(a1, a8);
    result = m_chessboard.MakeMove(Rxa8);
    EXPECT_TRUE(result);
    EXPECT_NE(orgHash, m_chessboard.readHash());
    EXPECT_TRUE(castlingState.hasWhiteKingSide());
    EXPECT_FALSE(castlingState.hasWhiteQueenSide());
    EXPECT_TRUE(castlingState.hasBlackKingSide());
    EXPECT_FALSE(castlingState.hasBlackQueenSide());
    EXPECT_EQ(CastlingState::BLACK_KINGSIDE | CastlingState::WHITE_KINGSIDE, m_chessboard.readCastlingState());
    EXPECT_EQ(WHITEROOK, m_chessboard.readPieceAt(a8));

    m_chessboard.UnmakeMove(Rxa8);
    EXPECT_EQ(orgHash, m_chessboard.readHash());
    EXPECT_TRUE(castlingState.hasAll());
    EXPECT_EQ(CastlingState::BLACK_ALL | CastlingState::WHITE_ALL, m_chessboard.readCastlingState());
    EXPECT_EQ(WHITEROOK, m_chessboard.readPieceAt(a1));
    EXPECT_EQ(WHITEROOK, m_chessboard.readPieceAt(h1));
    EXPECT_EQ(BLACKROOK, m_chessboard.readPieceAt(a8));

    Move Rxa1(a8, a1);
    result = m_chessboard.MakeMove(Rxa1);
    EXPECT_TRUE(result);
    EXPECT_NE(orgHash, m_chessboard.readHash());
    EXPECT_TRUE(castlingState.hasWhiteKingSide());
    EXPECT_FALSE(castlingState.hasWhiteQueenSide());
    EXPECT_TRUE(castlingState.hasBlackKingSide());
    EXPECT_FALSE(castlingState.hasBlackQueenSide());

    m_chessboard.UnmakeMove(Rxa1);
    EXPECT_EQ(orgHash, m_chessboard.readHash());
    EXPECT_TRUE(castlingState.hasAll());

    Move Rxh1(h8, h1);
    result = m_chessboard.MakeMove(Rxh1);
    EXPECT_TRUE(result);
    EXPECT_NE(orgHash, m_chessboard.readHash());
    EXPECT_FALSE(castlingState.hasWhiteKingSide());
    EXPECT_TRUE(castlingState.hasWhiteQueenSide());
    EXPECT_FALSE(castlingState.hasBlackKingSide());
    EXPECT_TRUE(castlingState.hasBlackQueenSide());

    m_chessboard.UnmakeMove(Rxh1);
    EXPECT_EQ(orgHash, m_chessboard.readHash());
    EXPECT_TRUE(castlingState.hasAll());
}

TEST_F(UnmakeFixture, Unmake_BishopMove)
{
    auto b = BLACKBISHOP;
    auto B = WHITEBISHOP;
    auto empty = ChessPiece();

    m_chessboard.PlacePiece(b, a8);
    m_chessboard.PlacePiece(B, h1);

    u64 hash = m_chessboard.readHash();

    // Bxh1
    Move move(a8, h1);
    bool result = m_chessboard.MakeMove(move);
    EXPECT_TRUE(result);
    EXPECT_EQ(empty, m_chessboard.readPieceAt(a8));
    EXPECT_EQ(b, m_chessboard.readPieceAt(h1));
    EXPECT_NE(hash, m_chessboard.readHash());

    // unmake
    m_chessboard.UnmakeMove(move);
    EXPECT_EQ(hash, m_chessboard.readHash());
    EXPECT_EQ(b, m_chessboard.readTile(a8).readPiece());
    EXPECT_EQ(B, m_chessboard.readTile(h1).readPiece());
}

TEST_F(UnmakeFixture, Unmake_Knight)
{
    auto k = BLACKKNIGHT;
    auto K = WHITEKNIGHT;

    m_chessboard.PlacePiece(k, e8);
    m_chessboard.PlacePiece(K, e1);

    u64 hash = m_chessboard.readHash();

    // Nf6
    Move move(e8, f6);
    m_chessboard.MakeMove(move);
    EXPECT_NE(hash, m_chessboard.readHash());
    hash = m_chessboard.readHash();

    // Nf3
    Move scndMove(e1, f3);
    m_chessboard.MakeMove(scndMove);
    EXPECT_NE(hash, m_chessboard.readHash());
    hash = m_chessboard.readHash();

    m_chessboard.UnmakeMove(scndMove);
    EXPECT_NE(hash, m_chessboard.readHash());
    hash = m_chessboard.readHash();

    m_chessboard.UnmakeMove(move);
    EXPECT_NE(hash, m_chessboard.readHash());
    hash = m_chessboard.readHash();

    EXPECT_EQ(k, m_chessboard.readTile(e8).readPiece());
    EXPECT_EQ(K, m_chessboard.readTile(e1).readPiece());
    EXPECT_EQ(hash, m_chessboard.readHash());
}

TEST_F(UnmakeFixture, Unmake_KingMoves)
{
    auto k = BLACKKING;
    auto K = WHITEKING;

    m_chessboard.PlacePiece(k, e8);
    m_chessboard.PlacePiece(K, e1);

    u64 orgHash = m_chessboard.readHash();
    u64 hash = m_chessboard.readHash();

    // Ke7
    Move move(e8, e7);
    EXPECT_TRUE(m_chessboard.MakeMove(move));
    EXPECT_NE(hash, m_chessboard.readHash());
    EXPECT_EQ(e7, m_chessboard.readKingPosition(Set::BLACK));
    hash = m_chessboard.readHash();

    // Ke2
    Move scndMove(e1, e2);
    EXPECT_TRUE(m_chessboard.MakeMove(scndMove));
    EXPECT_NE(hash, m_chessboard.readHash());
    EXPECT_EQ(e2, m_chessboard.readKingPosition(Set::WHITE));
    hash = m_chessboard.readHash();

    EXPECT_TRUE(m_chessboard.UnmakeMove(scndMove));
    EXPECT_NE(hash, m_chessboard.readHash());
    EXPECT_EQ(e1, m_chessboard.readKingPosition(Set::WHITE));
    hash = m_chessboard.readHash();

    EXPECT_TRUE(m_chessboard.UnmakeMove(move));
    EXPECT_NE(hash, m_chessboard.readHash());
    EXPECT_EQ(e8, m_chessboard.readKingPosition(Set::BLACK));
    hash = m_chessboard.readHash();

    EXPECT_EQ(k, m_chessboard.readTile(e8).readPiece());
    EXPECT_EQ(K, m_chessboard.readTile(e1).readPiece());
    EXPECT_EQ(k, m_chessboard.readPieceAt(e8));
    EXPECT_EQ(K, m_chessboard.readPieceAt(e1));
    EXPECT_EQ(orgHash, m_chessboard.readHash());
}

// 8 [ r ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][ B ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][   ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
TEST_F(UnmakeFixture, BishopCaptureRookRemovesCastlingOption)
{
    m_chessboard.PlacePiece(WHITEBISHOP, d5);
    m_chessboard.PlacePiece(BLACKKING, e8);
    m_chessboard.PlacePiece(BLACKROOK, a8);
    m_chessboard.setCastlingState(CastlingState::BLACK_QUEENSIDE);
    const auto& blackMaterial = m_chessboard.readMaterial(Set::BLACK);
    const auto& whiteMaterial = m_chessboard.readMaterial(Set::WHITE);

    u64 hash = m_chessboard.readHash();
    EXPECT_EQ(2, blackMaterial.getCount());
    EXPECT_EQ(1, blackMaterial.getPieceCount(BLACKKING));
    EXPECT_EQ(1, blackMaterial.getPieceCount(BLACKROOK));
    EXPECT_EQ(1, whiteMaterial.getPieceCount(WHITEBISHOP));
    EXPECT_EQ(1, whiteMaterial.getCount());

    Move move(d5, a8);
    bool result = m_chessboard.MakeMove(move);
    EXPECT_TRUE(result);
    EXPECT_EQ(CastlingState::NONE, m_chessboard.readCastlingState());
    EXPECT_EQ(WHITEBISHOP, m_chessboard.readPieceAt(a8));
    EXPECT_NE(hash, m_chessboard.readHash());
    EXPECT_EQ(1, blackMaterial.getCount());
    EXPECT_EQ(1, blackMaterial.getPieceCount(BLACKKING));
    EXPECT_EQ(0, blackMaterial.getPieceCount(BLACKROOK));
    EXPECT_EQ(1, whiteMaterial.getPieceCount(WHITEBISHOP));
    EXPECT_EQ(1, whiteMaterial.getCount());

    EXPECT_TRUE(m_chessboard.UnmakeMove(move));
    EXPECT_EQ(CastlingState::BLACK_QUEENSIDE, m_chessboard.readCastlingState());
    EXPECT_EQ(WHITEBISHOP, m_chessboard.readPieceAt(d5));
    EXPECT_EQ(BLACKKING, m_chessboard.readPieceAt(e8));
    EXPECT_EQ(BLACKROOK, m_chessboard.readPieceAt(a8));
    EXPECT_EQ(hash, m_chessboard.readHash());
    EXPECT_EQ(2, blackMaterial.getCount());
    EXPECT_EQ(1, blackMaterial.getPieceCount(BLACKKING));
    EXPECT_EQ(1, blackMaterial.getPieceCount(BLACKROOK));
    EXPECT_EQ(1, whiteMaterial.getPieceCount(WHITEBISHOP));
    EXPECT_EQ(1, whiteMaterial.getCount());
}

////////////////////////////////////////////////////////////////

}  // namespace ElephantTest