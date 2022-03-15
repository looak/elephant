#include "chess_piece.h"

namespace ElephantTest
{ 
    ChessPiece blackPawn(PieceSet::BLACK, PieceType::PAWN);
    ChessPiece blackKnight(PieceSet::BLACK, PieceType::KNIGHT);
    ChessPiece blackBishop(PieceSet::BLACK, PieceType::BISHOP);
    ChessPiece blackRook(PieceSet::BLACK, PieceType::ROOK);
    ChessPiece blackQueen(PieceSet::BLACK, PieceType::QUEEN);
    ChessPiece blackKing(PieceSet::BLACK, PieceType::KING);

    ChessPiece whitePawn(PieceSet::WHITE, PieceType::PAWN);
    ChessPiece whiteKnight(PieceSet::WHITE, PieceType::KNIGHT);
    ChessPiece whiteBishop(PieceSet::WHITE, PieceType::BISHOP);
    ChessPiece whiteRook(PieceSet::WHITE, PieceType::ROOK);    
    ChessPiece whiteQueen(PieceSet::WHITE, PieceType::QUEEN);
    ChessPiece whiteKing(PieceSet::WHITE, PieceType::KING);

    auto a1 = Notation::BuildPosition('a', 1);
    auto a2 = Notation::BuildPosition('a', 2);
    auto a3 = Notation::BuildPosition('a', 3);
    auto a4 = Notation::BuildPosition('a', 4);
    auto a5 = Notation::BuildPosition('a', 5);
    auto a6 = Notation::BuildPosition('a', 6);
    auto a7 = Notation::BuildPosition('a', 7);
    auto a8 = Notation::BuildPosition('a', 8);

    auto b1 = Notation::BuildPosition('b', 1);
    auto b2 = Notation::BuildPosition('b', 2);
    auto b3 = Notation::BuildPosition('b', 3);
    auto b4 = Notation::BuildPosition('b', 4);
    auto b5 = Notation::BuildPosition('b', 5);
    auto b6 = Notation::BuildPosition('b', 6);
    auto b7 = Notation::BuildPosition('b', 7);
    auto b8 = Notation::BuildPosition('b', 8);

    auto c1 = Notation::BuildPosition('c', 1);
    auto c2 = Notation::BuildPosition('c', 2);
    auto c3 = Notation::BuildPosition('c', 3);
    auto c4 = Notation::BuildPosition('c', 4);
    auto c5 = Notation::BuildPosition('c', 5);
    auto c6 = Notation::BuildPosition('c', 6);
    auto c7 = Notation::BuildPosition('c', 7);
    auto c8 = Notation::BuildPosition('c', 8);

    auto d1 = Notation::BuildPosition('d', 1);
    auto d2 = Notation::BuildPosition('d', 2);
    auto d3 = Notation::BuildPosition('d', 3);
    auto d4 = Notation::BuildPosition('d', 4);
    auto d5 = Notation::BuildPosition('d', 5);
    auto d6 = Notation::BuildPosition('d', 6);
    auto d7 = Notation::BuildPosition('d', 7);
    auto d8 = Notation::BuildPosition('d', 8);

    auto e1 = Notation::BuildPosition('e', 1);
    auto e2 = Notation::BuildPosition('e', 2);
    auto e3 = Notation::BuildPosition('e', 3);
    auto e4 = Notation::BuildPosition('e', 4);
    auto e5 = Notation::BuildPosition('e', 5);
    auto e6 = Notation::BuildPosition('e', 6);
    auto e7 = Notation::BuildPosition('e', 7);
    auto e8 = Notation::BuildPosition('e', 8);

    auto f1 = Notation::BuildPosition('f', 1);
    auto f2 = Notation::BuildPosition('f', 2);
    auto f3 = Notation::BuildPosition('f', 3);
    auto f4 = Notation::BuildPosition('f', 4);
    auto f5 = Notation::BuildPosition('f', 5);
    auto f6 = Notation::BuildPosition('f', 6);
    auto f7 = Notation::BuildPosition('f', 7);
    auto f8 = Notation::BuildPosition('f', 8);

    auto g1 = Notation::BuildPosition('g', 1);
    auto g2 = Notation::BuildPosition('g', 2);
    auto g3 = Notation::BuildPosition('g', 3);
    auto g4 = Notation::BuildPosition('g', 4);
    auto g5 = Notation::BuildPosition('g', 5);
    auto g6 = Notation::BuildPosition('g', 6);
    auto g7 = Notation::BuildPosition('g', 7);
    auto g8 = Notation::BuildPosition('g', 8);

    auto h1 = Notation::BuildPosition('h', 1);
    auto h2 = Notation::BuildPosition('h', 2);
    auto h3 = Notation::BuildPosition('h', 3);
    auto h4 = Notation::BuildPosition('h', 4);
    auto h5 = Notation::BuildPosition('h', 5);
    auto h6 = Notation::BuildPosition('h', 6);
    auto h7 = Notation::BuildPosition('h', 7);
    auto h8 = Notation::BuildPosition('h', 8);

} // namespace ElephantTest