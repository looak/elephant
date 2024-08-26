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

/**
 * @file position_editor.hpp
 * @brief class for updating the position of the board. has helpers for placing pieces
 * and making moves.
 */

#pragma once
#include <position/position.hpp>
#include <chess_piece.h>

typedef ChessPiece Piece;

class PositionEditor {
public:
    PositionEditor(Position& position) : m_position(position) {}

    void clear();
    bool empty();

    template<typename... placementpairs>
    bool placePieces(placementpairs... placements);
    bool placePiece(Piece piece, Square square);
    bool removePiece(Square square);

    template<typename... moves>
    [[nodiscard]] std::vector<MoveUndoUnit> makeMoves(const moves&... _moves);
    [[nodiscard]] MoveUndoUnit makeMove(PackedMove move);

    void unmakeMove(const MoveUndoUnit& undoUnit);

    EnPassantStateInfo& editEnPassantState();
    void updateEnPassantState(EnPassantStateInfo& info);

    CastlingStateInfo& editCastlingState();
    void updateCastlingState(CastlingStateInfo& info);

private:
    template<typename piece, typename square, typename... placements>
    bool internalUnrollPlacementPairs(const piece& p, const square& sqr, const placements&... _placements);
    bool internalUnrollPlacementPairs() { return true; }

    template<typename move, typename... moves>
    std::vector<MoveUndoUnit> internalUnrollMoves(const move& m, const moves&... _moves);
    std::vector<MoveUndoUnit> internalUnrollMoves() { return {}; }

    Position& m_position;
};

template<typename piece, typename square, typename... placements>
bool PositionEditor::internalUnrollPlacementPairs(const piece& p, const square& sqr, const placements&... _placements) {
    if (placePiece(p, sqr) == false)
        return false;

    return internalUnrollPlacementPairs(_placements...);
}

template<typename... placements>
bool PositionEditor::placePieces(placements... _placement) {
    static_assert(sizeof...(_placement) % 2 == 0, "Number of arguments must be even");
    return internalUnrollPlacementPairs(_placement...);
}

template<typename... moves>
std::vector<MoveUndoUnit> PositionEditor::makeMoves(const moves&... _moves) {
    return internalUnrollMoves(_moves...);
}

template<typename move, typename... moves>
std::vector<MoveUndoUnit> Chessboard::InternalUnrollMoves(const move& m, const moves&... _moves) {
    std::vector<MoveUndoUnit> result;

    // result.push_back(InternalMakeMove(m));
    // auto temp = InternalUnrollMoves(_moves...);
    // result.insert(result.end(), temp.begin(), temp.end());
    return result;
}