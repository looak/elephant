/******************************************************************************
 * Elephant Gambit Chess Engine - a Chess AI
 * Copyright(C) 2025  Alexander Loodin Ek
 * 
 * This program is free software : you can redistribute it and /or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.If not, see < http://www.gnu.org/licenses/>. 
 *****************************************************************************/

/**
 * @file move_executor.hpp
 * @brief Implements move execution logic for a chess game, updating game state and history, allows implicit unmakeMove which
 * can be used to revert the last move made, or multiple moves in a row.
 */

#pragma once

#include <eval/pesto_accumulator.hpp>
#include <position/position_accessors.hpp>
#include <move/move.hpp>

class GameContext;
struct GameHistory;
struct GameState;

class MoveExecutor {
public:
    // accumulator, when given, is updated as make adds & removes pieces and restored from the undo unit on unmake, see
    // PestoAccumulator. Search owns it, the position doesn't carry evaluation state.
    MoveExecutor(PositionProxy<PositionEditPolicy> position, PestoAccumulator* accumulator = nullptr)
        : m_position(position), m_accumulator(accumulator) {}

    ~MoveExecutor() = default;

    template<bool validation = false>
    void makeMove(const PackedMove move, MoveUndoUnit& undoState, u16& plyCount);
    bool unmakeMove(const MoveUndoUnit& undoState);

private:
    MoveUndoUnit internalMakeMove(const std::string& moveString);
    void internalMakeMove(ChessPiece piece, Square source, Square target, MutableMaterialProxy materialEditor);

    std::tuple<Square, ChessPiece> internalHandlePawnMove(const PackedMove move, Set set, MutableMaterialProxy& materialEditor, MoveUndoUnit& undoState);
    void internalHandleRookMove(const ChessPiece piece, const PackedMove move, Square targetRook, Square rookMove, MoveUndoUnit& undoState);
    void internalHandleRookMovedOrCaptured(Square rookSquare, MoveUndoUnit& undoState);
    void internalUpdateCastlingState(byte mask, MoveUndoUnit& undoState);    
    bool internalHandleKingMove(const PackedMove move, Set set, Square& targetRook, Square& rookMove, MoveUndoUnit& undoState);
    void internalHandleKingRookMove(const ChessPiece piece, const PackedMove move, MoveUndoUnit& undoState);
    void internalHandleCapture(const PackedMove move, const Square pieceTarget, MoveUndoUnit& undoState);
    void internalUpdateEnPassant(Square source, Square target);

    // a piece entered or left a square, forwarded to the accumulator.
    void pieceAdded(ChessPiece piece, Square square) { if (m_accumulator) m_accumulator->add(piece, square); }
    void pieceRemoved(ChessPiece piece, Square square) { if (m_accumulator) m_accumulator->remove(piece, square); }

    PositionProxy<PositionEditPolicy> m_position;
    PestoAccumulator* m_accumulator;
};