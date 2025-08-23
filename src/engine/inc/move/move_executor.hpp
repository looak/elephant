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

#include <position/position_accessors.hpp>
#include <move/move.hpp>

class GameContext;
struct GameHistory;
struct GameState;

class MoveExecutor {
public:
    MoveExecutor(GameContext& context);

    template<bool validation = false>
    void makeMove(const PackedMove move);
    bool unmakeMove();

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

    PositionProxy<PositionEditPolicy> m_position;

    GameState& m_gameStateRef;
    GameHistory& m_gameHistoryRef;
};