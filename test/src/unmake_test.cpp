#include <gtest/gtest.h>
#include "chess_piece.h"
#include "chessboard.h"
#include "elephant_test_utils.h"
#include "game_context.h"
#include "log.h"
#include "move.h"
#include "move_generator.hpp"

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

// // 8 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 2 [   ][   ][   ][   ][ P ][   ][   ][   ]
// // 1 [   ][   ][   ][   ][   ][   ][   ][   ]
// //     A    B    C    D    E    F    G    H
// // 1. e3, unmake, e4, unmake.
// TEST_F(UnmakeFixture, Pawn_SimpleMoves)
// {
//     auto P = WHITEPAWN;
//     m_chessboard.PlacePiece(P, e2);
//     u64 orgHash = m_chessboard.readHash();

//     {
//         Move move(e2, e3);

//         // do e3 move
//         bool result = m_chessboard.MakeMove(move);

//         // verify
//         EXPECT_TRUE(result);
//         EXPECT_EQ(P, move.Piece);
//         EXPECT_EQ(MoveFlag::Zero, move.Flags);

//         EXPECT_EQ(Notation(), m_chessboard.readEnPassant());
//         EXPECT_EQ(P, m_chessboard.readTile(e3).readPiece());
//         EXPECT_EQ(ChessPiece(), m_chessboard.readTile(e2).readPiece());
//         EXPECT_NE(orgHash, m_chessboard.readHash());

//         // unmake move
//         result = m_chessboard.UnmakeMove(move);

//         // verify state of board
//         EXPECT_TRUE(result);
//         EXPECT_EQ(ChessPiece(), m_chessboard.readTile(e3).readPiece());
//         EXPECT_EQ(P, m_chessboard.readTile(e2).readPiece());
//         EXPECT_EQ(Notation(), m_chessboard.readEnPassant());
//         EXPECT_EQ(orgHash, m_chessboard.readHash());
//     }

//     {
//         Move move(e2, e4);

//         // do e4 move
//         bool result = m_chessboard.MakeMove(move);

//         // verify
//         EXPECT_TRUE(result);
//         EXPECT_EQ(P, move.Piece);

//         EXPECT_EQ(e3, m_chessboard.readEnPassant());
//         EXPECT_EQ(P, m_chessboard.readTile(e4).readPiece());
//         EXPECT_EQ(ChessPiece(), m_chessboard.readTile(e2).readPiece());
//         EXPECT_NE(orgHash, m_chessboard.readHash());

//         // unmake move
//         result = m_chessboard.UnmakeMove(move);

//         // verify state of board
//         EXPECT_TRUE(result);
//         EXPECT_EQ(ChessPiece(), m_chessboard.readTile(e4).readPiece());
//         EXPECT_EQ(P, m_chessboard.readTile(e2).readPiece());
//         EXPECT_EQ(Notation(), m_chessboard.readEnPassant());
//         EXPECT_EQ(orgHash, m_chessboard.readHash());
//     }
// }

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

    const auto& whitePawns = m_chessboard.readPosition().readMaterial<Set::WHITE>().pawns();
    const auto& blackPawns = m_chessboard.readPosition().readMaterial<Set::BLACK>().pawns();

    // validate setup
    EXPECT_EQ(1, whitePawns.count());
    EXPECT_EQ(1, blackPawns.count());

    // move white pawn to e4
    PackedMove move;
    move.setSource(Square::E2);
    move.setTarget(Square::E4);
    auto undoUnit = m_chessboard.MakeMove<false>(move);

    // validate move
    EXPECT_EQ(undoUnit.move, move);

    /**
     * En passant is a special pawn capture move in chess where a pawn captures an opposing pawn
     * that has just advanced two squares from its starting position, as if it had only advanced
     * one square. The capturing pawn moves diagonally to the square that the opposing pawn passed
     * over, and the captured pawn is removed from the board.     */
    EXPECT_EQ(Square::E3, m_chessboard.readPosition().readEnPassant().readSquare());
    EXPECT_TRUE(m_chessboard.readPosition().readEnPassant());
    EXPECT_EQ(P, m_chessboard.readTile(e4).readPiece());
    EXPECT_EQ(p, m_chessboard.readTile(d4).readPiece());

    EXPECT_EQ(1, whitePawns.count());
    EXPECT_EQ(1, blackPawns.count());

    // setup ep capture move
    PackedMove epCapture(Square::D4, Square::E3);
    epCapture.setCapture(true);
    epCapture.setEnPassant(true);

    // do
    auto epUndo = m_chessboard.MakeMove<false>(epCapture);

    // validate
    EXPECT_EQ(epCapture, epUndo.move);
    EXPECT_EQ(Square::NullSQ, m_chessboard.readPosition().readEnPassant().readSquare());
    EXPECT_FALSE(m_chessboard.readPosition().readEnPassant());

    ChessPiece exp;  // default, "empty" piece
    EXPECT_EQ(exp, m_chessboard.readTile(e4).readPiece());
    EXPECT_EQ(exp, m_chessboard.readTile(d4).readPiece());
    EXPECT_EQ(p, m_chessboard.readTile(e3).readPiece());

    EXPECT_EQ(0, whitePawns.count());
    EXPECT_EQ(1, blackPawns.count());

    // do
    bool result = m_chessboard.UnmakeMove(epUndo);

    // validate
    EXPECT_TRUE(result);
    EXPECT_EQ(Square::E3, m_chessboard.readPosition().readEnPassant().readSquare());
    EXPECT_EQ(P, m_chessboard.readTile(e4).readPiece());
    EXPECT_EQ(p, m_chessboard.readTile(d4).readPiece());
    EXPECT_EQ(exp, m_chessboard.readTile(e3).readPiece());

    EXPECT_EQ(1, whitePawns.count());
    EXPECT_EQ(1, blackPawns.count());
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

    GameContext context;
    auto& board = context.editChessboard();

    board.PlacePiece(P, c2);
    board.PlacePiece(P, f2);
    board.PlacePiece(p, d4);
    board.PlacePiece(p, g4);
    board.PlacePiece(WHITEKING, e1);
    board.PlacePiece(BLACKKING, e8);

    const auto& whitePawns = board.readPosition().readMaterial<Set::WHITE>().pawns();
    const auto& blackPawns = board.readPosition().readMaterial<Set::BLACK>().pawns();

    // LOG_INFO() << board.toString();

    // do
    MoveGenerator whiteMoves(context);
    PackedMove wMove = whiteMoves.generateNextMove();
    while (wMove != PackedMove::NullMove()) {
        if (board.readPieceAt(wMove.sourceSqr()).isPawn() == false) {
            wMove = whiteMoves.generateNextMove();
            continue;
        }

        auto whiteUndo = board.MakeMove<false>(wMove);
        // LOG_INFO() << board.toString();

        context.editToPlay() = Set::BLACK;
        MoveGenerator blackMoves(context);
        PackedMove bMove = blackMoves.generateNextMove();
        while (bMove != PackedMove::NullMove()) {
            auto blackUndo = board.MakeMove<false>(bMove);
            // LOG_INFO() << board.toString();
            board.UnmakeMove(blackUndo);
            // LOG_INFO() << board.toString();

            bMove = blackMoves.generateNextMove();
        }

        board.UnmakeMove(whiteUndo);
        wMove = whiteMoves.generateNextMove();
    }
    // validate
    EXPECT_EQ(2, whitePawns.count());
    EXPECT_TRUE(whitePawns[Square::C2]);
    EXPECT_TRUE(whitePawns[Square::F2]);
    EXPECT_EQ(2, blackPawns.count());
    EXPECT_TRUE(blackPawns[Square::D4]);
    EXPECT_TRUE(blackPawns[Square::G4]);
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

    PackedMove move(Square::E7, Square::E8);
    move.setPromoteTo(WHITEQUEEN);

    u64 hash = m_chessboard.readHash();
    const auto& position = m_chessboard.readPosition();

    // do
    auto undounit = m_chessboard.MakeMove<false>(move);

    // validate
    EXPECT_TRUE(undounit.hash == hash);
    EXPECT_TRUE(undounit.move.isPromotion());
    EXPECT_EQ(undounit.move.readPromoteToPieceType(), static_cast<i32>(PieceType::QUEEN));

    EXPECT_NE(hash, m_chessboard.readHash());

    u64 pawnMask = position.readMaterial<Set::WHITE>().pawns().read();
    EXPECT_EQ(0, pawnMask);

    auto Q = WHITEQUEEN;
    EXPECT_EQ(Q, m_chessboard.readTile(e8).readPiece());
    i32 qCount = position.readMaterial<Set::WHITE>().queens().count();
    EXPECT_EQ(1, qCount);

    // undo
    bool result = m_chessboard.UnmakeMove(undounit);

    // validate
    EXPECT_TRUE(result);
    EXPECT_EQ(P, m_chessboard.readTile(e7).readPiece());
    EXPECT_EQ(ChessPiece::None(), m_chessboard.readTile(e8).readPiece());
    EXPECT_EQ(n, m_chessboard.readTile(d8).readPiece());
    EXPECT_EQ(hash, m_chessboard.readHash());

    pawnMask = position.readMaterial<Set::WHITE>().pawns().read();
    EXPECT_EQ(squareMaskTable[static_cast<i32>(Square::E7)], pawnMask);
    qCount = position.readMaterial<Set::WHITE>().queens().count();
    EXPECT_EQ(0, qCount);

    PackedMove capturePromote(Square::E7, Square::D8);
    capturePromote.setCapture(true);
    capturePromote.setPromoteTo(WHITEQUEEN);

    // check that there is a piece to be captured
    EXPECT_EQ(n, m_chessboard.readTile(d8).readPiece());
    const auto& knights = m_chessboard.readPosition().readMaterial<Set::BLACK>().knights();
    EXPECT_EQ(1, knights.count());
    EXPECT_EQ(squareMaskTable[static_cast<i32>(Square::D8)], knights.read());
    hash = m_chessboard.readHash();

    // do
    undounit = m_chessboard.MakeMove<false>(capturePromote);

    // validate
    EXPECT_EQ(hash, undounit.hash);
    EXPECT_NE(hash, m_chessboard.readHash());

    EXPECT_EQ(Q, m_chessboard.readTile(d8).readPiece());
    EXPECT_EQ(ChessPiece::None(), m_chessboard.readTile(e7).readPiece());
    EXPECT_EQ(0, knights.count());

    // undo
    result = m_chessboard.UnmakeMove(undounit);

    // validate
    EXPECT_TRUE(result);
    EXPECT_EQ(n, m_chessboard.readTile(d8).readPiece());
    EXPECT_EQ(P, m_chessboard.readTile(e7).readPiece());
    EXPECT_EQ(1, knights.count());
    EXPECT_EQ(squareMaskTable[static_cast<i32>(Square::D8)], knights.read());
    EXPECT_EQ(hash, m_chessboard.readHash());

    const auto& pawns = m_chessboard.readPosition().readMaterial<Set::WHITE>().pawns();
    EXPECT_EQ(squareMaskTable[static_cast<i32>(Square::E7)], pawns.read());
    EXPECT_EQ(1, pawns.count());
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
// O-O-O
TEST_F(UnmakeFixture, King_CastlingQueenSide_Black_Unmake)
{
    auto K = WHITEKING;
    auto R = WHITEROOK;
    auto k = BLACKKING;
    auto r = BLACKROOK;
    auto empty = ChessPiece();

    auto& castlingInfo = m_chessboard.editPosition().editCastling();
    castlingInfo.setAll();
    m_chessboard.PlacePieces(K, e1, R, a1, R, h1, k, e8, r, a8, r, h8);
    // LOG_INFO() << m_chessboard.toString();

    // do
    {
        PackedMove move(Square::E1, Square::C1);  // castle queen side
        move.setCastleQueenSide(true);

        auto undo = m_chessboard.MakeMove<false>(move);
        EXPECT_EQ(K, m_chessboard.readTile(c1).readPiece());
        EXPECT_EQ(R, m_chessboard.readTile(d1).readPiece());
        EXPECT_EQ(empty, m_chessboard.readTile(e1).readPiece());
        EXPECT_FALSE(castlingInfo.hasWhite());
        EXPECT_TRUE(castlingInfo.hasBlack());

        // LOG_INFO() << m_chessboard.toString();

        // undo
        bool result = m_chessboard.UnmakeMove(undo);
        EXPECT_TRUE(result);
        EXPECT_EQ(K, m_chessboard.readTile(e1).readPiece());
        EXPECT_EQ(R, m_chessboard.readTile(a1).readPiece());
        EXPECT_EQ(empty, m_chessboard.readTile(c1).readPiece());
        EXPECT_TRUE(castlingInfo.hasWhite());
        EXPECT_TRUE(castlingInfo.hasWhiteKingSide());
        EXPECT_TRUE(castlingInfo.hasWhiteQueenSide());
        EXPECT_TRUE(castlingInfo.hasAll());

        // LOG_INFO() << m_chessboard.toString();
    }

    // do castle king side
    {
        PackedMove move(Square::E1, Square::G1);  // castle king side
        move.setCastleKingSide(true);

        auto undo = m_chessboard.MakeMove<false>(move);
        EXPECT_EQ(K, m_chessboard.readTile(g1).readPiece());
        EXPECT_EQ(R, m_chessboard.readTile(f1).readPiece());
        EXPECT_EQ(empty, m_chessboard.readTile(e1).readPiece());
        EXPECT_FALSE(castlingInfo.hasWhite());
        EXPECT_TRUE(castlingInfo.hasBlack());

        // LOG_INFO() << m_chessboard.toString();

        // undo
        bool result = m_chessboard.UnmakeMove(undo);
        EXPECT_TRUE(result);
        EXPECT_EQ(K, m_chessboard.readTile(e1).readPiece());
        EXPECT_EQ(R, m_chessboard.readTile(h1).readPiece());
        EXPECT_EQ(empty, m_chessboard.readTile(f1).readPiece());
        EXPECT_TRUE(castlingInfo.hasAll());
    }

    // do black castling
    {
        PackedMove move(Square::E8, Square::C8);  // castle queen side
        move.setCastleQueenSide(true);

        auto undo = m_chessboard.MakeMove<false>(move);
        EXPECT_EQ(k, m_chessboard.readTile(c8).readPiece());
        EXPECT_EQ(r, m_chessboard.readTile(d8).readPiece());
        EXPECT_EQ(empty, m_chessboard.readTile(e8).readPiece());
        EXPECT_FALSE(castlingInfo.hasBlack());
        EXPECT_TRUE(castlingInfo.hasWhite());

        // LOG_INFO() << m_chessboard.toString();

        // undo
        bool result = m_chessboard.UnmakeMove(undo);
        EXPECT_TRUE(result);
        EXPECT_EQ(k, m_chessboard.readTile(e8).readPiece());
        EXPECT_EQ(r, m_chessboard.readTile(a8).readPiece());
        EXPECT_EQ(empty, m_chessboard.readTile(c8).readPiece());
        EXPECT_TRUE(castlingInfo.hasAll());
    }

    // do black king side caslting
    {
        PackedMove move(Square::E8, Square::G8);  // castle king side
        move.setCastleKingSide(true);

        auto undo = m_chessboard.MakeMove<false>(move);
        EXPECT_EQ(k, m_chessboard.readTile(g8).readPiece());
        EXPECT_EQ(r, m_chessboard.readTile(f8).readPiece());
        EXPECT_EQ(empty, m_chessboard.readTile(e8).readPiece());
        EXPECT_FALSE(castlingInfo.hasBlack());
        EXPECT_TRUE(castlingInfo.hasWhite());

        // LOG_INFO() << m_chessboard.toString();

        // undo
        bool result = m_chessboard.UnmakeMove(undo);
        EXPECT_TRUE(result);
        EXPECT_EQ(k, m_chessboard.readTile(e8).readPiece());
        EXPECT_EQ(r, m_chessboard.readTile(h8).readPiece());
        EXPECT_EQ(empty, m_chessboard.readTile(f8).readPiece());
        EXPECT_TRUE(castlingInfo.hasAll());
    }
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

    m_chessboard.editPosition().editCastling().setAll();
    m_chessboard.PlacePiece(k, e8);
    m_chessboard.PlacePiece(r, a8);
    m_chessboard.PlacePiece(r, h8);

    m_chessboard.PlacePiece(K, e1);
    m_chessboard.PlacePiece(R, a1);
    m_chessboard.PlacePiece(R, h1);

    auto undos = m_chessboard.MakeMoves("O-O-O", "Ra6", "Rh3", "O-O");
    LOG_INFO() << m_chessboard.toString();

    EXPECT_EQ(k, m_chessboard.readTile(g8).readPiece());

    // // O-O-O
    // PackedMove whiteQueensideCastle(Square::E1, Square::C1);
    // whiteQueensideCastle.setCastleQueenSide(true);
    // m_chessboard.MakeMove<false>(whiteQueensideCastle);

    // // Ra6
    // Move rMove(a8, a6);
    // m_chessboard.MakeMove(rMove);
    // EXPECT_NE(hash, m_chessboard.readHash());
    // hash = m_chessboard.readHash();

    // // Rh3
    // Move RMove(h1, h3);
    // m_chessboard.MakeMove(RMove);
    // EXPECT_NE(hash, m_chessboard.readHash());
    // hash = m_chessboard.readHash();

    // // O-O
    // Move scndCastle(e8, g8);
    // m_chessboard.MakeMove(scndCastle);
    // EXPECT_NE(hash, m_chessboard.readHash());
    // hash = m_chessboard.readHash();

    // m_chessboard.UnmakeMove(scndCastle);
    // EXPECT_NE(hash, m_chessboard.readHash());
    // hash = m_chessboard.readHash();

    // m_chessboard.UnmakeMove(RMove);
    // EXPECT_NE(hash, m_chessboard.readHash());
    // hash = m_chessboard.readHash();

    // m_chessboard.UnmakeMove(rMove);
    // EXPECT_NE(hash, m_chessboard.readHash());
    // hash = m_chessboard.readHash();

    // m_chessboard.UnmakeMove(move);
    // EXPECT_NE(hash, m_chessboard.readHash());
    // hash = m_chessboard.readHash();

    // EXPECT_EQ(expectedCastling, m_chessboard.readCastlingState().raw());
    // EXPECT_EQ(k, m_chessboard.readTile(e8).readPiece());
    // EXPECT_EQ(K, m_chessboard.readTile(e1).readPiece());
    // EXPECT_EQ(r, m_chessboard.readTile(a8).readPiece());
    // EXPECT_EQ(r, m_chessboard.readTile(h8).readPiece());
    // EXPECT_EQ(R, m_chessboard.readTile(a1).readPiece());
    // EXPECT_EQ(R, m_chessboard.readTile(h1).readPiece());
    // EXPECT_EQ(orgHash, hash);
}
// // 8 [ r ][   ][   ][   ][ k ][   ][   ][ r ]
// // 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 3 [   ][   ][   ][   ][   ][ b ][   ][   ]
// // 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 1 [ R ][   ][   ][   ][ K ][   ][   ][ R ]
// //     A    B    C    D    E    F    G    H
// TEST_F(UnmakeFixture, Castling_Captures)
// {
//     auto k = BLACKKING;
//     auto r = BLACKROOK;
//     auto b = BLACKBISHOP;
//     auto K = WHITEKING;
//     auto R = WHITEROOK;

//     byte expectedCastling = 0xf;  // all castling available
//     m_chessboard.setCastlingState(15);
//     EXPECT_EQ(expectedCastling, m_chessboard.readCastlingState().raw());
//     EXPECT_TRUE(m_chessboard.readCastlingState().hasAll());
//     auto& castlingState = m_chessboard.readPosition().refCastling();
//     m_chessboard.PlacePiece(k, e8);
//     m_chessboard.PlacePiece(r, a8);
//     m_chessboard.PlacePiece(r, h8);
//     m_chessboard.PlacePiece(b, f3);

//     m_chessboard.PlacePiece(K, e1);
//     m_chessboard.PlacePiece(R, a1);
//     m_chessboard.PlacePiece(R, h1);

//     u64 orgHash = m_chessboard.readHash();

//     Move Bxh1(f3, h1);
//     bool result = m_chessboard.MakeMove(Bxh1);
//     EXPECT_TRUE(result);
//     EXPECT_NE(orgHash, m_chessboard.readHash());
//     EXPECT_FALSE(castlingState.hasWhiteKingSide());
//     EXPECT_EQ(CastlingState::BLACK_ALL | CastlingState::WHITE_QUEENSIDE, m_chessboard.readCastlingState().raw());
//     EXPECT_EQ(BLACKBISHOP, m_chessboard.readPieceAt(h1));

//     m_chessboard.UnmakeMove(Bxh1);
//     EXPECT_EQ(orgHash, m_chessboard.readHash());
//     EXPECT_TRUE(castlingState.hasAll());
//     EXPECT_EQ(CastlingState::BLACK_ALL | CastlingState::WHITE_ALL, m_chessboard.readCastlingState().raw());
//     EXPECT_EQ(BLACKBISHOP, m_chessboard.readPieceAt(f3));
//     EXPECT_EQ(WHITEROOK, m_chessboard.readPieceAt(h1));

//     Move Rxa8(a1, a8);
//     result = m_chessboard.MakeMove(Rxa8);
//     EXPECT_TRUE(result);
//     EXPECT_NE(orgHash, m_chessboard.readHash());
//     EXPECT_TRUE(castlingState.hasWhiteKingSide());
//     EXPECT_FALSE(castlingState.hasWhiteQueenSide());
//     EXPECT_TRUE(castlingState.hasBlackKingSide());
//     EXPECT_FALSE(castlingState.hasBlackQueenSide());
//     EXPECT_EQ(CastlingState::BLACK_KINGSIDE | CastlingState::WHITE_KINGSIDE, m_chessboard.readCastlingState().raw());
//     EXPECT_EQ(WHITEROOK, m_chessboard.readPieceAt(a8));

//     m_chessboard.UnmakeMove(Rxa8);
//     EXPECT_EQ(orgHash, m_chessboard.readHash());
//     EXPECT_TRUE(castlingState.hasAll());
//     EXPECT_EQ(CastlingState::BLACK_ALL | CastlingState::WHITE_ALL, m_chessboard.readCastlingState().raw());
//     EXPECT_EQ(WHITEROOK, m_chessboard.readPieceAt(a1));
//     EXPECT_EQ(WHITEROOK, m_chessboard.readPieceAt(h1));
//     EXPECT_EQ(BLACKROOK, m_chessboard.readPieceAt(a8));

//     Move Rxa1(a8, a1);
//     result = m_chessboard.MakeMove(Rxa1);
//     EXPECT_TRUE(result);
//     EXPECT_NE(orgHash, m_chessboard.readHash());
//     EXPECT_TRUE(castlingState.hasWhiteKingSide());
//     EXPECT_FALSE(castlingState.hasWhiteQueenSide());
//     EXPECT_TRUE(castlingState.hasBlackKingSide());
//     EXPECT_FALSE(castlingState.hasBlackQueenSide());

//     m_chessboard.UnmakeMove(Rxa1);
//     EXPECT_EQ(orgHash, m_chessboard.readHash());
//     EXPECT_TRUE(castlingState.hasAll());

//     Move Rxh1(h8, h1);
//     result = m_chessboard.MakeMove(Rxh1);
//     EXPECT_TRUE(result);
//     EXPECT_NE(orgHash, m_chessboard.readHash());
//     EXPECT_FALSE(castlingState.hasWhiteKingSide());
//     EXPECT_TRUE(castlingState.hasWhiteQueenSide());
//     EXPECT_FALSE(castlingState.hasBlackKingSide());
//     EXPECT_TRUE(castlingState.hasBlackQueenSide());

//     m_chessboard.UnmakeMove(Rxh1);
//     EXPECT_EQ(orgHash, m_chessboard.readHash());
//     EXPECT_TRUE(castlingState.hasAll());
// }

// TEST_F(UnmakeFixture, Unmake_BishopMove)
// {
//     auto b = BLACKBISHOP;
//     auto B = WHITEBISHOP;
//     auto empty = ChessPiece();

//     m_chessboard.PlacePiece(b, a8);
//     m_chessboard.PlacePiece(B, h1);

//     u64 hash = m_chessboard.readHash();

//     // Bxh1
//     Move move(a8, h1);
//     bool result = m_chessboard.MakeMove(move);
//     EXPECT_TRUE(result);
//     EXPECT_EQ(empty, m_chessboard.readPieceAt(a8));
//     EXPECT_EQ(b, m_chessboard.readPieceAt(h1));
//     EXPECT_NE(hash, m_chessboard.readHash());

//     // unmake
//     m_chessboard.UnmakeMove(move);
//     EXPECT_EQ(hash, m_chessboard.readHash());
//     EXPECT_EQ(b, m_chessboard.readTile(a8).readPiece());
//     EXPECT_EQ(B, m_chessboard.readTile(h1).readPiece());
// }

// TEST_F(UnmakeFixture, Unmake_Knight)
// {
//     auto k = BLACKKNIGHT;
//     auto K = WHITEKNIGHT;

//     m_chessboard.PlacePiece(k, e8);
//     m_chessboard.PlacePiece(K, e1);

//     u64 hash = m_chessboard.readHash();

//     // Nf6
//     Move move(e8, f6);
//     m_chessboard.MakeMove(move);
//     EXPECT_NE(hash, m_chessboard.readHash());
//     hash = m_chessboard.readHash();

//     // Nf3
//     Move scndMove(e1, f3);
//     m_chessboard.MakeMove(scndMove);
//     EXPECT_NE(hash, m_chessboard.readHash());
//     hash = m_chessboard.readHash();

//     m_chessboard.UnmakeMove(scndMove);
//     EXPECT_NE(hash, m_chessboard.readHash());
//     hash = m_chessboard.readHash();

//     m_chessboard.UnmakeMove(move);
//     EXPECT_NE(hash, m_chessboard.readHash());
//     hash = m_chessboard.readHash();

//     EXPECT_EQ(k, m_chessboard.readTile(e8).readPiece());
//     EXPECT_EQ(K, m_chessboard.readTile(e1).readPiece());
//     EXPECT_EQ(hash, m_chessboard.readHash());
// }

// TEST_F(UnmakeFixture, Unmake_KingMoves)
// {
//     auto k = BLACKKING;
//     auto K = WHITEKING;

//     m_chessboard.PlacePiece(k, e8);
//     m_chessboard.PlacePiece(K, e1);

//     u64 orgHash = m_chessboard.readHash();
//     u64 hash = m_chessboard.readHash();

//     // Ke7
//     Move move(e8, e7);
//     EXPECT_TRUE(m_chessboard.MakeMove(move));
//     EXPECT_NE(hash, m_chessboard.readHash());
//     EXPECT_EQ(e7, m_chessboard.readKingPosition(Set::BLACK));
//     hash = m_chessboard.readHash();

//     // Ke2
//     Move scndMove(e1, e2);
//     EXPECT_TRUE(m_chessboard.MakeMove(scndMove));
//     EXPECT_NE(hash, m_chessboard.readHash());
//     EXPECT_EQ(e2, m_chessboard.readKingPosition(Set::WHITE));
//     hash = m_chessboard.readHash();

//     EXPECT_TRUE(m_chessboard.UnmakeMove(scndMove));
//     EXPECT_NE(hash, m_chessboard.readHash());
//     EXPECT_EQ(e1, m_chessboard.readKingPosition(Set::WHITE));
//     hash = m_chessboard.readHash();

//     EXPECT_TRUE(m_chessboard.UnmakeMove(move));
//     EXPECT_NE(hash, m_chessboard.readHash());
//     EXPECT_EQ(e8, m_chessboard.readKingPosition(Set::BLACK));
//     hash = m_chessboard.readHash();

//     EXPECT_EQ(k, m_chessboard.readTile(e8).readPiece());
//     EXPECT_EQ(K, m_chessboard.readTile(e1).readPiece());
//     EXPECT_EQ(k, m_chessboard.readPieceAt(e8));
//     EXPECT_EQ(K, m_chessboard.readPieceAt(e1));
//     EXPECT_EQ(orgHash, m_chessboard.readHash());
// }

// // 8 [ r ][   ][   ][   ][ k ][   ][   ][   ]
// // 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 5 [   ][   ][   ][ B ][   ][   ][   ][   ]
// // 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 1 [   ][   ][   ][   ][   ][   ][   ][   ]
// //     A    B    C    D    E    F    G    H
// TEST_F(UnmakeFixture, BishopCaptureRookRemovesCastlingOption)
// {
//     m_chessboard.PlacePiece(WHITEBISHOP, d5);
//     m_chessboard.PlacePiece(BLACKKING, e8);
//     m_chessboard.PlacePiece(BLACKROOK, a8);
//     m_chessboard.setCastlingState(CastlingState::BLACK_QUEENSIDE);
//     const auto& blackKings = m_chessboard.readPosition().readMaterial<Set::BLACK>()[kingId];
//     const auto& blackRooks = m_chessboard.readPosition().readMaterial<Set::BLACK>()[rookId];
//     const auto& whiteBishops = m_chessboard.readPosition().readMaterial<Set::WHITE>()[bishopId];

//     u64 hash = m_chessboard.readHash();
//     EXPECT_EQ(1, blackKings.count());
//     EXPECT_EQ(1, blackRooks.count());
//     EXPECT_EQ(1, whiteBishops.count());

//     Move move(d5, a8);
//     bool result = m_chessboard.MakeMove(move);
//     EXPECT_TRUE(result);
//     EXPECT_TRUE(m_chessboard.readCastlingState().hasNone());
//     EXPECT_EQ(WHITEBISHOP, m_chessboard.readPieceAt(a8));
//     EXPECT_NE(hash, m_chessboard.readHash());
//     EXPECT_EQ(1, blackKings.count());
//     EXPECT_EQ(0, blackRooks.count());
//     EXPECT_EQ(1, whiteBishops.count());

//     EXPECT_TRUE(m_chessboard.UnmakeMove(move));
//     EXPECT_EQ(CastlingState::BLACK_QUEENSIDE, m_chessboard.readCastlingState().raw());
//     EXPECT_EQ(WHITEBISHOP, m_chessboard.readPieceAt(d5));
//     EXPECT_EQ(BLACKKING, m_chessboard.readPieceAt(e8));
//     EXPECT_EQ(BLACKROOK, m_chessboard.readPieceAt(a8));
//     EXPECT_EQ(hash, m_chessboard.readHash());
//     EXPECT_EQ(1, blackKings.count());
//     EXPECT_EQ(1, blackRooks.count());
//     EXPECT_EQ(1, whiteBishops.count());
// }

// ////////////////////////////////////////////////////////////////

}  // namespace ElephantTest