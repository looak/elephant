// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021-2023  Alexander Loodin Ek

// This program is free software : you can redistribute it and /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.If not, see < http://www.gnu.org/licenses/>.

#pragma once
#include <array>
#include <queue>
#include <string>
#include <utility>
#include <vector>
#include "chess_piece.h"
#include "move.h"
#include "notation.h"
#include <position/position.hpp>

/**
 * The Chessboard class represents a chess board and its current state.
 * It provides functions for moving and placing chess pieces, and updates
 * all underlying state accordingly.
 *
 * @author Alexander Loodin Ek  */
class Chessboard {
public:
    Chessboard();
    ~Chessboard() = default;
    Chessboard(const Chessboard& other);

    void Clear();
    bool PlacePiece(ChessPiece piece, Notation target, bool overwrite = false);

    template<bool validation>
    MoveUndoUnit MakeMove(const PackedMove move);

    bool UnmakeMove(const MoveUndoUnit& undoState);

    template<typename... moves>
    std::vector<MoveUndoUnit> MakeMoves(const moves&... _moves);

    
    /**
     * @brief Sets the en passant square.
     * Sets the en passant square, updates hash and calculates the en passant target square.
     * @param notation The notation of the en passant square.
     * @return true if the en passant square was set */
    bool setEnPassant(Notation notation);
    bool setCastlingState(u8 castlingState);
    
    PositionReader readPosition() const { return m_position.read(); }
    PositionEditor editPosition() { return m_position.edit(); }

    
    short readMoveCount() const { return m_moveCount; }
    short readPlyCount() const { return m_plyCount; }
    void setPlyAndMoveCount(short ply, short moveCount)
    {
        m_plyCount = ply;
        m_moveCount = moveCount;
    }
    Set readToPlay() const { return m_isWhiteTurn ? Set::WHITE : Set::BLACK; }
    void setToPlay(Set set);
    short readAge() const { return m_age; }

    std::string toString() const;

private:
    template<typename move, typename... moves>
    std::vector<MoveUndoUnit> InternalUnrollMoves(const move& m, const moves&... _moves);
    std::vector<MoveUndoUnit> InternalUnrollMoves() { return {}; }

    MoveUndoUnit InternalMakeMove(const std::string& moveString);

    std::tuple<Square, ChessPiece> InternalHandlePawnMove(const PackedMove move, Set set, MutableMaterialProxy& materialEditor, MoveUndoUnit& undoState);
    void InternalHandleRookMove(const ChessPiece piece, const PackedMove move, Square targetRook, Square rookMove, MoveUndoUnit& undoState);
    void InternalHandleRookMovedOrCaptured(Notation rookSquare, MoveUndoUnit& undoState);
    void InternalUpdateCastlingState(byte mask, MoveUndoUnit& undoState);

    /**
     * Internal helper function for handling the movement of a king chess piece.
     *
     * @param move The move being made.
     * @param set The set of the king being moved.
     * @param targetRook The position of the rook that will be involved in the castle move (if any).
     * @param rookMove The position that the rook will move to during the castle move (if any).
     * @return True if the move is a castle move, false otherwise. */
    bool InternalHandleKingMove(const PackedMove move, Set set, Square& targetRook, Square& rookMove, MoveUndoUnit& undoState);
    void InternalHandleKingRookMove(const ChessPiece piece, const PackedMove move, MoveUndoUnit& undoState);
    void InternalHandleCapture(const PackedMove move, const Square pieceTarget, MoveUndoUnit& undoState);

    bool InternalUpdateEnPassant(Notation source, Notation target);
    void InternalMakeMove(ChessPiece piece, Square source, Square target, MutableMaterialProxy materialEditor);
    
    Position m_position;

    bool m_isWhiteTurn;
    short m_moveCount;
    short m_plyCount;
    short m_age;

    // caching kings and their locations
    std::pair<ChessPiece, Notation> m_kings[2];
};

template<typename... moves>
std::vector<MoveUndoUnit>
Chessboard::MakeMoves(const moves&... _moves)
{
    return InternalUnrollMoves(_moves...);
}

template<typename move, typename... moves>
std::vector<MoveUndoUnit>
Chessboard::InternalUnrollMoves(const move& m, const moves&... _moves)
{
    std::vector<MoveUndoUnit> result;
    result.push_back(InternalMakeMove(m));
    auto temp = InternalUnrollMoves(_moves...);
    result.insert(result.end(), temp.begin(), temp.end());
    return result;
}