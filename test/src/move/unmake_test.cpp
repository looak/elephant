#include <gtest/gtest.h>
#include <stack>

#include <material/chess_piece.hpp>
#include <core/chessboard.hpp>
#include <core/game_context.hpp>
#include <debug/log.hpp>
#include <move/move.hpp>
#include <move/generation/move_generator.hpp>
#include <move/move_executor.hpp>

namespace ElephantTest {
////////////////////////////////////////////////////////////////
class UnmakeFixture : public ::testing::Test {
public:
    virtual void SetUp() {

    };
    virtual void TearDown() {};

    GameContext m_game;
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
    // setup
    PositionEditor editor = m_game.editChessPosition();    
    editor.placePiece(piece_constants::white_pawn, Square::E2);
    u64 orgHash = editor.hash();

    
    PackedMove move(Square::E2, Square::E3);

    // do e3 move
    m_game.MakeMove<true>(move);

    // verify
    EXPECT_NE(orgHash, editor.hash()); 
    EXPECT_FALSE(editor.enPassant());
    EXPECT_EQ(piece_constants::white_pawn, editor.pieceAt(Square::E3));
    EXPECT_EQ(ChessPiece::None(), editor.pieceAt(Square::E2));

    // unmake move
    bool unmakeResult = m_game.UnmakeMove();

    // verify state of board
    EXPECT_TRUE(unmakeResult);
    EXPECT_EQ(ChessPiece::None(), editor.pieceAt(Square::E3));
    EXPECT_EQ(piece_constants::white_pawn, editor.pieceAt(Square::E2));
    EXPECT_FALSE(editor.enPassant());
    EXPECT_EQ(orgHash, editor.hash());

    PackedMove moveE4(Square::E2, Square::E4);

    // do e4 move
    m_game.MakeMove<true>(moveE4);

    // verify
    EXPECT_EQ(Square::E3, editor.enPassant().readSquare());
    EXPECT_EQ(piece_constants::white_pawn, editor.pieceAt(Square::E4));
    EXPECT_EQ(ChessPiece::None(), editor.pieceAt(Square::E2));
    EXPECT_NE(orgHash, editor.hash());

    // unmake move
    unmakeResult = m_game.UnmakeMove();

    // verify state of board
    EXPECT_TRUE(unmakeResult);
    EXPECT_EQ(ChessPiece::None(), editor.pieceAt(Square::E4));
    EXPECT_EQ(piece_constants::white_pawn, editor.pieceAt(Square::E2));
    EXPECT_FALSE(editor.enPassant());
    EXPECT_EQ(orgHash, editor.hash());

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
    // setup    
    PositionEditor editor = m_game.editChessboard().editPosition();
    auto P = WHITEPAWN;
    auto p = BLACKPAWN;
    editor.placePieces(P, Square::E2, p, Square::D4);


    // sanity check that the material is on the board
    const auto& material = editor.material();
    EXPECT_EQ(1, material.whitePawns().count());
    EXPECT_EQ(1, material.blackPawns().count());

    // move white pawn to e4
    PackedMove move;
    move.setSource(Square::E2);
    move.setTarget(Square::E4);
    m_game.MakeMove<true>(move);
  
    /**
     * En passant is a special pawn capture move in chess where a pawn captures an opposing pawn
     * that has just advanced two squares from its starting position, as if it had only advanced
     * one square. The capturing pawn moves diagonally to the square that the opposing pawn passed
     * over, and the captured pawn is removed from the board.     */
    EXPECT_TRUE(editor.enPassant());
    EXPECT_EQ(Square::E3, editor.enPassant().readSquare());
    EXPECT_EQ(P, editor.pieceAt(Square::E4));
    EXPECT_EQ(p, editor.pieceAt(Square::D4));
    EXPECT_EQ(1, material.whitePawns().count());
    EXPECT_EQ(1, material.blackPawns().count());

    // setup ep capture move
    PackedMove epCapture(Square::D4, Square::E3);
    epCapture.setCapture(true);
    epCapture.setEnPassant(true);

    // do
    m_game.MakeMove<true>(epCapture);

    // validate
    EXPECT_EQ(Square::NullSQ, editor.enPassant().readSquare());
    EXPECT_FALSE(editor.enPassant());

    ChessPiece expected;  // default, "empty" piece
    EXPECT_EQ(expected, editor.pieceAt(Square::E4));
    EXPECT_EQ(expected, editor.pieceAt(Square::D4));
    EXPECT_EQ(p, editor.pieceAt(Square::E3));

    EXPECT_EQ(0, material.whitePawns().count());
    EXPECT_EQ(1, material.blackPawns().count());

    // do
    bool result = m_game.UnmakeMove();

    // validate
    EXPECT_TRUE(result);
    EXPECT_EQ(Square::E3, editor.enPassant().readSquare());
    EXPECT_EQ(P, editor.pieceAt(Square::E4));
    EXPECT_EQ(p, editor.pieceAt(Square::D4));
    EXPECT_EQ(expected, editor.pieceAt(Square::E3));

    EXPECT_EQ(1, material.whitePawns().count());
    EXPECT_EQ(1, material.blackPawns().count());
}

// /**
//  * 8 [   ][   ][   ][   ][   ][   ][   ][   ]
//  * 7 [   ][   ][   ][   ][   ][   ][   ][   ]
//  * 6 [   ][   ][   ][   ][   ][   ][   ][   ]
//  * 5 [   ][   ][   ][   ][   ][   ][   ][   ]
//  * 4 [   ][   ][   ][ p ][   ][   ][ p ][   ]
//  * 3 [   ][   ][   ][   ][   ][   ][   ][   ]
//  * 2 [   ][   ][ P ][   ][   ][ P ][   ][   ]
//  * 1 [   ][   ][   ][   ][   ][   ][   ][   ]
//  *     A    B    C    D    E    F    G    H */
// TEST_F(UnmakeFixture, UnmakeEnPassantMoves_VariousPositions_CorrectUndo)
// {
//     // setup
//     auto P = WHITEPAWN;
//     auto p = BLACKPAWN;

//     GameContext context;
//     auto& board = context.editChessboard();

//     board.PlacePiece(P, c2);
//     board.PlacePiece(P, f2);
//     board.PlacePiece(p, d4);
//     board.PlacePiece(p, g4);
//     board.PlacePiece(WHITEKING, e1);
//     board.PlacePiece(BLACKKING, e8);

//     const auto& material = board.readPosition().readMaterial();

//     // do
//     MoveGenerator whiteMoves(context);
//     PackedMove wMove = whiteMoves.pop().move;
//     while (wMove != PackedMove::NullMove()) {
//         if (board.readPieceAt(wMove.sourceSqr()).isPawn() == false) {
//             wMove = whiteMoves.pop().move;
//             continue;
//         }

//         auto whiteUndo = board.MakeMove<false>(wMove);
//         context.editChessboard().setToPlay(Set::BLACK);
//         MoveGenerator blackMoves(context);
//         PackedMove bMove = blackMoves.pop().move;
//         while (bMove != PackedMove::NullMove()) {
//             auto blackUndo = board.MakeMove<false>(bMove);
//             board.UnmakeMove(blackUndo);
//             bMove = blackMoves.pop().move;
//         }

//         board.UnmakeMove(whiteUndo);
//         wMove = whiteMoves.pop().move;
//     }
//     // validate
//     auto whitePawns = material.whitePawns();
//     EXPECT_EQ(2, material.whitePawns().count());
//     EXPECT_TRUE(whitePawns[Square::C2]);
//     EXPECT_TRUE(whitePawns[Square::F2]);

//     auto blackPawns = material.blackPawns();
//     EXPECT_EQ(2, material.blackPawns().count());
//     EXPECT_TRUE(blackPawns[Square::D4]);
//     EXPECT_TRUE(blackPawns[Square::G4]);
// }

// // 8 [   ][   ][   ][ n ][   ][   ][   ][   ]
// // 7 [   ][   ][   ][   ][ P ][   ][   ][   ]
// // 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 1 [   ][   ][   ][   ][   ][   ][   ][   ]
// //     A    B    C    D    E    F    G    H
// // Moves:
// // e8=Q
// TEST_F(UnmakeFixture, Pawn_Promotion_Unmake)
// {
//     auto P = WHITEPAWN;
//     auto n = BLACKKNIGHT;

//     m_chessboard.PlacePiece(P, e7);
//     m_chessboard.PlacePiece(n, d8);

//     PackedMove move(Square::E7, Square::E8);
//     move.setPromoteTo(WHITEQUEEN);

//     u64 hash = m_chessboard.readHash();
//     const auto& position = m_chessboard.readPosition();

//     // do
//     auto undounit = m_chessboard.MakeMove<false>(move);

//     // validate
//     EXPECT_TRUE(undounit.hash == hash);
//     EXPECT_TRUE(undounit.move.isPromotion());
//     EXPECT_EQ(undounit.move.readPromoteToPieceType(), static_cast<i32>(PieceType::QUEEN));

//     EXPECT_NE(hash, m_chessboard.readHash());

//     u64 pawnMask = position.readMaterial().pawns<Set::WHITE>().read();
//     EXPECT_EQ(0, pawnMask);

//     auto Q = WHITEQUEEN;
//     EXPECT_EQ(Q, m_chessboard.readPieceAt(Square::E8));
//     i32 qCount = position.readMaterial().queens<Set::WHITE>().count();
//     EXPECT_EQ(1, qCount);

//     // undo
//     bool result = m_chessboard.UnmakeMove(undounit);

//     // validate
//     EXPECT_TRUE(result);
//     EXPECT_EQ(P, m_chessboard.readPieceAt(Square::E7));
//     EXPECT_EQ(ChessPiece::None(), m_chessboard.readPieceAt(Square::E8));
//     EXPECT_EQ(n, m_chessboard.readPieceAt(Square::D8));
//     EXPECT_EQ(hash, m_chessboard.readHash());

//     pawnMask = position.readMaterial().pawns<Set::WHITE>().read();
//     EXPECT_EQ(squareMaskTable[static_cast<i32>(Square::E7)], pawnMask);
//     qCount = position.readMaterial().queens<Set::WHITE>().count();
//     EXPECT_EQ(0, qCount);

//     PackedMove capturePromote(Square::E7, Square::D8);
//     capturePromote.setCapture(true);
//     capturePromote.setPromoteTo(WHITEQUEEN);

//     // check that there is a piece to be captured
//     EXPECT_EQ(n, m_chessboard.readPieceAt(Square::D8));
//     auto knights = m_chessboard.readPosition().readMaterial().knights<Set::BLACK>();
//     EXPECT_EQ(1, knights.count());
//     EXPECT_EQ(squareMaskTable[static_cast<i32>(Square::D8)], knights.read());
//     hash = m_chessboard.readHash();

//     // do
//     undounit = m_chessboard.MakeMove<false>(capturePromote);

//     // validate
//     EXPECT_EQ(hash, undounit.hash);
//     EXPECT_NE(hash, m_chessboard.readHash());

//     knights = m_chessboard.readPosition().readMaterial().knights<Set::BLACK>();
//     EXPECT_EQ(Q, m_chessboard.readPieceAt(Square::D8));
//     EXPECT_EQ(ChessPiece::None(), m_chessboard.readPieceAt(Square::E7));
//     EXPECT_EQ(0, knights.count());

//     // undo
//     result = m_chessboard.UnmakeMove(undounit);

//     // validate
//     knights = m_chessboard.readPosition().readMaterial().knights<Set::BLACK>();

//     EXPECT_TRUE(result);
//     EXPECT_EQ(n, m_chessboard.readPieceAt(Square::D8));
//     EXPECT_EQ(P, m_chessboard.readPieceAt(Square::E7));
//     EXPECT_EQ(1, knights.count());
//     EXPECT_EQ(squareMaskTable[static_cast<i32>(Square::D8)], knights.read());
//     EXPECT_EQ(hash, m_chessboard.readHash());

//     const auto& pawns = m_chessboard.readPosition().readMaterial().pawns<Set::WHITE>();
//     EXPECT_EQ(squareMaskTable[static_cast<i32>(Square::E7)], pawns.read());
//     EXPECT_EQ(1, pawns.count());
// }

// // 8 [ r ][   ][   ][   ][ k ][   ][   ][ r ]
// // 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 1 [ R ][   ][   ][   ][ K ][   ][   ][ R ]
// //     A    B    C    D    E    F    G    H
// // Moves:
// // O-O-O
// // TEST_F(UnmakeFixture, King_CastlingQueenSide_Black_Unmake)
// // {
// //     auto K = WHITEKING;
// //     auto R = WHITEROOK;
// //     auto k = BLACKKING;
// //     auto r = BLACKROOK;
// //     auto empty = ChessPiece::None();

// //     auto& castlingInfo = m_chessboard.editPosition().editCastling();
// //     castlingInfo.setAll();
// //     m_chessboard.PlacePieces(K, e1, R, a1, R, h1, k, e8, r, a8, r, h8);
// //     // LOG_INFO() << m_chessboard.toString();

// //     // do
// //     {
// //         PackedMove move(Square::E1, Square::C1);  // castle queen side
// //         move.setCastleQueenSide(true);

// //         auto undo = m_chessboard.MakeMove<false>(move);
// //         EXPECT_EQ(K, m_chessboard.readPieceAt(Square::C1));
// //         EXPECT_EQ(R, m_chessboard.readPieceAt(Square::D1));
// //         EXPECT_EQ(empty, m_chessboard.readPieceAt(Square::E1));
// //         EXPECT_FALSE(castlingInfo.hasWhite());
// //         EXPECT_TRUE(castlingInfo.hasBlack());

// //         // LOG_INFO() << m_chessboard.toString();

// //         // undo
// //         bool result = m_chessboard.UnmakeMove(undo);
// //         EXPECT_TRUE(result);
// //         EXPECT_EQ(K, m_chessboard.readPieceAt(Square::E1));
// //         EXPECT_EQ(R, m_chessboard.readPieceAt(Square::A1));
// //         EXPECT_EQ(empty, m_chessboard.readPieceAt(Square::C1));
// //         EXPECT_TRUE(castlingInfo.hasWhite());
// //         EXPECT_TRUE(castlingInfo.hasWhiteKingSide());
// //         EXPECT_TRUE(castlingInfo.hasWhiteQueenSide());
// //         EXPECT_TRUE(castlingInfo.hasAll());

// //         // LOG_INFO() << m_chessboard.toString();
// //     }

// //     // do castle king side
// //     {
// //         PackedMove move(Square::E1, Square::G1);  // castle king side
// //         move.setCastleKingSide(true);

// //         auto undo = m_chessboard.MakeMove<false>(move);
// //         EXPECT_EQ(K, m_chessboard.readPieceAt(Square::G1));
// //         EXPECT_EQ(R, m_chessboard.readPieceAt(Square::F1));
// //         EXPECT_EQ(empty, m_chessboard.readPieceAt(Square::E1));
// //         EXPECT_FALSE(castlingInfo.hasWhite());
// //         EXPECT_TRUE(castlingInfo.hasBlack());

// //         // LOG_INFO() << m_chessboard.toString();

// //         // undo
// //         bool result = m_chessboard.UnmakeMove(undo);
// //         EXPECT_TRUE(result);
// //         EXPECT_EQ(K, m_chessboard.readPieceAt(Square::E1));
// //         EXPECT_EQ(R, m_chessboard.readPieceAt(Square::H1));
// //         EXPECT_EQ(empty, m_chessboard.readPieceAt(Square::F1));
// //         EXPECT_TRUE(castlingInfo.hasAll());
// //     }

// //     // do black castling
// //     {
// //         PackedMove move(Square::E8, Square::C8);  // castle queen side
// //         move.setCastleQueenSide(true);

// //         auto undo = m_chessboard.MakeMove<false>(move);
// //         EXPECT_EQ(k, m_chessboard.readPieceAt(Square::C8));
// //         EXPECT_EQ(r, m_chessboard.readPieceAt(Square::D8));
// //         EXPECT_EQ(empty, m_chessboard.readPieceAt(Square::E8));
// //         EXPECT_FALSE(castlingInfo.hasBlack());
// //         EXPECT_TRUE(castlingInfo.hasWhite());

// //         // LOG_INFO() << m_chessboard.toString();

// //         // undo
// //         bool result = m_chessboard.UnmakeMove(undo);
// //         EXPECT_TRUE(result);
// //         EXPECT_EQ(k, m_chessboard.readPieceAt(Square::E8));
// //         EXPECT_EQ(r, m_chessboard.readPieceAt(Square::A8));
// //         EXPECT_EQ(empty, m_chessboard.readPieceAt(Square::C8));
// //         EXPECT_TRUE(castlingInfo.hasAll());
// //     }

// //     // do black king side caslting
// //     {
// //         PackedMove move(Square::E8, Square::G8);  // castle king side
// //         move.setCastleKingSide(true);

// //         auto undo = m_chessboard.MakeMove<false>(move);
// //         EXPECT_EQ(k, m_chessboard.readPieceAt(Square::G8));
// //         EXPECT_EQ(r, m_chessboard.readPieceAt(Square::F8));
// //         EXPECT_EQ(empty, m_chessboard.readPieceAt(Square::E8));
// //         EXPECT_FALSE(castlingInfo.hasBlack());
// //         EXPECT_TRUE(castlingInfo.hasWhite());

// //         // LOG_INFO() << m_chessboard.toString();

// //         // undo
// //         bool result = m_chessboard.UnmakeMove(undo);
// //         EXPECT_TRUE(result);
// //         EXPECT_EQ(k, m_chessboard.readPieceAt(Square::E8));
// //         EXPECT_EQ(r, m_chessboard.readPieceAt(Square::H8));
// //         EXPECT_EQ(empty, m_chessboard.readPieceAt(Square::F8));
// //         EXPECT_TRUE(castlingInfo.hasAll());
// //     }
// // }

// // 8 [ r ][   ][   ][   ][ k ][   ][   ][ r ]
// // 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 1 [ R ][   ][   ][   ][ K ][   ][   ][ R ]
// //     A    B    C    D    E    F    G    H
// // Moves:
// // 1. O-O-O Ra6
// // 2. Rh3 O-O
// //
// // Result:
// // 8 [   ][   ][   ][   ][   ][ r ][ k ][   ]
// // 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 6 [ r ][   ][   ][   ][   ][   ][   ][   ]
// // 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 3 [   ][   ][   ][   ][   ][   ][   ][ R ]
// // 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// // 1 [   ][   ][ K ][ R ][   ][   ][   ][   ]
// //     A    B    C    D    E    F    G    H
// TEST_F(UnmakeFixture, Castling)
// {
//     auto k = BLACKKING;
//     auto r = BLACKROOK;
//     auto K = WHITEKING;
//     auto R = WHITEROOK;

//     m_chessboard.editPosition().editCastling().setAll();
//     m_chessboard.PlacePiece(k, e8);
//     m_chessboard.PlacePiece(r, a8);
//     m_chessboard.PlacePiece(r, h8);

//     m_chessboard.PlacePiece(K, e1);
//     m_chessboard.PlacePiece(R, a1);
//     m_chessboard.PlacePiece(R, h1);

//     auto undos = m_chessboard.MakeMoves("O-O-O", "Ra6", "Rh3", "O-O");
//     // LOG_INFO() << m_chessboard.toString();

//     EXPECT_EQ(k, m_chessboard.readPieceAt(Square::G8));
//     EXPECT_EQ(r, m_chessboard.readPieceAt(Square::F8));
//     EXPECT_EQ(r, m_chessboard.readPieceAt(Square::A6));
//     EXPECT_EQ(K, m_chessboard.readPieceAt(Square::C1));
//     EXPECT_EQ(R, m_chessboard.readPieceAt(Square::D1));
//     EXPECT_EQ(R, m_chessboard.readPieceAt(Square::H3));
//     EXPECT_TRUE(m_chessboard.readPosition().readCastling().hasNone());

//     // undo
//     auto itr = undos.rbegin();
//     while (itr != undos.rend()) {
//         auto undo = *itr;
//         bool result = m_chessboard.UnmakeMove(undo);
//         EXPECT_TRUE(result);
//         itr++;
//     }

//     EXPECT_TRUE(m_chessboard.readPosition().readCastling().hasAll());
//     EXPECT_EQ(k, m_chessboard.readPieceAt(Square::E8));
//     EXPECT_EQ(K, m_chessboard.readPieceAt(Square::E1));
//     EXPECT_EQ(r, m_chessboard.readPieceAt(Square::A8));
//     EXPECT_EQ(r, m_chessboard.readPieceAt(Square::H8));
//     EXPECT_EQ(R, m_chessboard.readPieceAt(Square::A1));
//     EXPECT_EQ(R, m_chessboard.readPieceAt(Square::H1));
// }
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

//     const auto& castlingState = m_chessboard.readPosition().refCastling();

//     m_chessboard.PlacePiece(k, e8);
//     m_chessboard.PlacePiece(r, a8);
//     m_chessboard.PlacePiece(r, h8);
//     m_chessboard.PlacePiece(b, f3);

//     m_chessboard.PlacePiece(K, e1);
//     m_chessboard.PlacePiece(R, a1);
//     m_chessboard.PlacePiece(R, h1);
//     m_chessboard.setCastlingState(CastlingState::WHITE_ALL | CastlingState::BLACK_ALL);
//     u64 orgHash = m_chessboard.readHash();

//     PackedMove Bxh1(Square::F3, Square::H1);
//     Bxh1.setCapture(true);
//     auto undo = m_chessboard.MakeMove<false>(Bxh1);
//     EXPECT_NE(orgHash, m_chessboard.readHash());
//     EXPECT_FALSE(castlingState.hasWhiteKingSide());
//     EXPECT_TRUE(castlingState.hasWhiteQueenSide());
//     EXPECT_TRUE(castlingState.hasBlack());
//     EXPECT_EQ(BLACKBISHOP, m_chessboard.readPieceAt(Square::H1));

//     m_chessboard.UnmakeMove(undo);
//     EXPECT_EQ(orgHash, m_chessboard.readHash());
//     EXPECT_TRUE(castlingState.hasAll());
//     EXPECT_EQ(BLACKBISHOP, m_chessboard.readPieceAt(Square::F3));
//     EXPECT_EQ(WHITEROOK, m_chessboard.readPieceAt(Square::H1));

//     // Move Rxa8(a1, a8);
//     // result = m_chessboard.MakeMove(Rxa8);
//     // EXPECT_TRUE(result);
//     // EXPECT_NE(orgHash, m_chessboard.readHash());
//     // EXPECT_TRUE(castlingState.hasWhiteKingSide());
//     // EXPECT_FALSE(castlingState.hasWhiteQueenSide());
//     // EXPECT_TRUE(castlingState.hasBlackKingSide());
//     // EXPECT_FALSE(castlingState.hasBlackQueenSide());
//     // EXPECT_EQ(CastlingState::BLACK_KINGSIDE | CastlingState::WHITE_KINGSIDE, m_chessboard.readCastlingState().raw());
//     // EXPECT_EQ(WHITEROOK, m_chessboard.readPieceAt(Square::A8));

//     // m_chessboard.UnmakeMove(Rxa8);
//     // EXPECT_EQ(orgHash, m_chessboard.readHash());
//     // EXPECT_TRUE(castlingState.hasAll());
//     // EXPECT_EQ(CastlingState::BLACK_ALL | CastlingState::WHITE_ALL, m_chessboard.readCastlingState().raw());
//     // EXPECT_EQ(WHITEROOK, m_chessboard.readPieceAt(Square::A1));
//     // EXPECT_EQ(WHITEROOK, m_chessboard.readPieceAt(Square::H1));
//     // EXPECT_EQ(BLACKROOK, m_chessboard.readPieceAt(Square::A8));

//     // Move Rxa1(a8, a1);
//     // result = m_chessboard.MakeMove(Rxa1);
//     // EXPECT_TRUE(result);
//     // EXPECT_NE(orgHash, m_chessboard.readHash());
//     // EXPECT_TRUE(castlingState.hasWhiteKingSide());
//     // EXPECT_FALSE(castlingState.hasWhiteQueenSide());
//     // EXPECT_TRUE(castlingState.hasBlackKingSide());
//     // EXPECT_FALSE(castlingState.hasBlackQueenSide());

//     // m_chessboard.UnmakeMove(Rxa1);
//     // EXPECT_EQ(orgHash, m_chessboard.readHash());
//     // EXPECT_TRUE(castlingState.hasAll());

//     // Move Rxh1(h8, h1);
//     // result = m_chessboard.MakeMove(Rxh1);
//     // EXPECT_TRUE(result);
//     // EXPECT_NE(orgHash, m_chessboard.readHash());
//     // EXPECT_FALSE(castlingState.hasWhiteKingSide());
//     // EXPECT_TRUE(castlingState.hasWhiteQueenSide());
//     // EXPECT_FALSE(castlingState.hasBlackKingSide());
//     // EXPECT_TRUE(castlingState.hasBlackQueenSide());

//     // m_chessboard.UnmakeMove(Rxh1);
//     // EXPECT_EQ(orgHash, m_chessboard.readHash());
//     // EXPECT_TRUE(castlingState.hasAll());
// }

// // TEST_F(UnmakeFixture, Unmake_BishopMove)
// // {
// //     auto b = BLACKBISHOP;
// //     auto B = WHITEBISHOP;
// //     auto empty = ChessPiece::None();

// //     m_chessboard.PlacePiece(b, a8);
// //     m_chessboard.PlacePiece(B, h1);

// //     u64 hash = m_chessboard.readHash();

// //     // Bxh1
// //     Move move(a8, h1);
// //     bool result = m_chessboard.MakeMove(move);
// //     EXPECT_TRUE(result);
// //     EXPECT_EQ(empty, m_chessboard.readPieceAt(Square::A8));
// //     EXPECT_EQ(b, m_chessboard.readPieceAt(Square::H1));
// //     EXPECT_NE(hash, m_chessboard.readHash());

// //     // unmake
// //     m_chessboard.UnmakeMove(move);
// //     EXPECT_EQ(hash, m_chessboard.readHash());
// //     EXPECT_EQ(b, m_chessboard.readPieceAt(Square::A8));
// //     EXPECT_EQ(B, m_chessboard.readPieceAt(Square::H1));
// // }

// // TEST_F(UnmakeFixture, Unmake_Knight)
// // {
// //     auto k = BLACKKNIGHT;
// //     auto K = WHITEKNIGHT;

// //     m_chessboard.PlacePiece(k, e8);
// //     m_chessboard.PlacePiece(K, e1);

// //     u64 hash = m_chessboard.readHash();

// //     // Nf6
// //     Move move(e8, f6);
// //     m_chessboard.MakeMove(move);
// //     EXPECT_NE(hash, m_chessboard.readHash());
// //     hash = m_chessboard.readHash();

// //     // Nf3
// //     Move scndMove(e1, f3);
// //     m_chessboard.MakeMove(scndMove);
// //     EXPECT_NE(hash, m_chessboard.readHash());
// //     hash = m_chessboard.readHash();

// //     m_chessboard.UnmakeMove(scndMove);
// //     EXPECT_NE(hash, m_chessboard.readHash());
// //     hash = m_chessboard.readHash();

// //     m_chessboard.UnmakeMove(move);
// //     EXPECT_NE(hash, m_chessboard.readHash());
// //     hash = m_chessboard.readHash();

// //     EXPECT_EQ(k, m_chessboard.readPieceAt(Square::E8));
// //     EXPECT_EQ(K, m_chessboard.readPieceAt(Square::E1));
// //     EXPECT_EQ(hash, m_chessboard.readHash());
// // }

// TEST_F(UnmakeFixture, Unmake_KingMoves)
// {
//     auto k = BLACKKING;
//     auto K = WHITEKING;
//     auto R = WHITEROOK;

//     m_chessboard.PlacePiece(k, e8);
//     m_chessboard.PlacePiece(K, e1);
//     m_chessboard.PlacePiece(R, a1);
//     // m_chessboard.editPosition().editCastling().setWhiteKingSide();
//     m_chessboard.setCastlingState(CastlingState::WHITE_KINGSIDE);

//     CastlingStateInfo expectedCastlingRights;
//     expectedCastlingRights.write(0x01);

//     EXPECT_EQ(expectedCastlingRights, m_chessboard.readPosition().readCastling());

//     u64 orgHash = m_chessboard.readHash();
//     u64 hash = m_chessboard.readHash();

//     std::stack<MoveUndoUnit> undos;
//     // Ke7
//     PackedMove move(Square::E8, Square::E7);
//     undos.push(m_chessboard.MakeMove<false>(move));
//     EXPECT_NE(hash, m_chessboard.readHash());
//     EXPECT_EQ(k, m_chessboard.readPieceAt(Square::E7));
//     hash = m_chessboard.readHash();

//     // Ke2
//     PackedMove Ke2(Square::E1, Square::E2);
//     undos.push(m_chessboard.MakeMove<false>(Ke2));
//     EXPECT_NE(hash, m_chessboard.readHash());
//     EXPECT_EQ(K, m_chessboard.readPieceAt(Square::E2));
//     EXPECT_FALSE(m_chessboard.readPosition().readCastling().hasAny());
//     hash = m_chessboard.readHash();

//     EXPECT_TRUE(m_chessboard.UnmakeMove(undos.top()));
//     undos.pop();
//     EXPECT_NE(hash, m_chessboard.readHash());
//     EXPECT_EQ(K, m_chessboard.readPieceAt(Square::E1));
//     EXPECT_EQ(expectedCastlingRights, m_chessboard.readPosition().readCastling());
//     hash = m_chessboard.readHash();

//     EXPECT_TRUE(m_chessboard.UnmakeMove(undos.top()));
//     undos.pop();
//     EXPECT_NE(hash, m_chessboard.readHash());
//     EXPECT_EQ(k, m_chessboard.readPieceAt(Square::E8));
//     hash = m_chessboard.readHash();

//     EXPECT_EQ(k, m_chessboard.readPieceAt(Square::E8));
//     EXPECT_EQ(K, m_chessboard.readPieceAt(Square::E1));
//     EXPECT_EQ(k, m_chessboard.readPieceAt(Square::E8));
//     EXPECT_EQ(K, m_chessboard.readPieceAt(Square::E1));
//     EXPECT_EQ(orgHash, m_chessboard.readHash());
//     EXPECT_EQ(expectedCastlingRights, m_chessboard.readPosition().readCastling());
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

//     const auto& material = m_chessboard.readPosition().readMaterial();

//     u64 hash = m_chessboard.readHash();
//     EXPECT_EQ(1, material.blackKing().count());
//     EXPECT_EQ(1, material.blackRooks().count());
//     EXPECT_EQ(1, material.whiteBishops().count());

//     PackedMove Bxa8(Square::D5, Square::A8);
//     Bxa8.setCapture(true);
//     auto undo = m_chessboard.MakeMove<false>(Bxa8);
//     EXPECT_TRUE(m_chessboard.readCastlingState().hasNone());
//     EXPECT_EQ(WHITEBISHOP, m_chessboard.readPieceAt(Square::A8));
//     EXPECT_NE(hash, m_chessboard.readHash());
//     EXPECT_EQ(1, material.blackKing().count());
//     EXPECT_EQ(0, material.blackRooks().count());
//     EXPECT_EQ(1, material.whiteBishops().count());

//     EXPECT_TRUE(m_chessboard.UnmakeMove(undo));
//     EXPECT_EQ(CastlingState::BLACK_QUEENSIDE, m_chessboard.readCastlingState().read());
//     EXPECT_EQ(WHITEBISHOP, m_chessboard.readPieceAt(Square::D5));
//     EXPECT_EQ(BLACKKING, m_chessboard.readPieceAt(Square::E8));
//     EXPECT_EQ(BLACKROOK, m_chessboard.readPieceAt(Square::A8));
//     EXPECT_EQ(hash, m_chessboard.readHash());
//     EXPECT_EQ(1, material.blackKing().count());
//     EXPECT_EQ(1, material.blackRooks().count());
//     EXPECT_EQ(1, material.whiteBishops().count());
// }

// ////////////////////////////////////////////////////////////////

}  // namespace ElephantTest