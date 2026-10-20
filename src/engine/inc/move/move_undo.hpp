// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2026  Alexander Loodin Ek

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
 * @file move_undo.hpp
 * @brief MoveUndoUnit, what unmake needs to restore a position: the move, the pieces it moved & captured, castling, en
 * passant, the hash and search's PeSTO accumulator. Apart from move.hpp so the move types stay free of the evaluation:
 * only the executor, the game history and what makes & unmakes moves include this.    */

#pragma once

#include <system/platform.hpp>
#include <eval/pesto_accumulator.hpp>
#include <material/chess_piece.hpp>
#include <move/move.hpp>
#include <position/castling_state_info.hpp>
#include <position/en_passant_state_info.hpp>

struct MoveUndoUnit {
    MoveUndoUnit() :
        move(),
        movedPiece(),
        capturedPiece(),
        castlingState(),
        enPassantState(),
        hash(0),
        plyCount(0)
    {
    }
    PackedMove move;
    ChessPiece movedPiece;
    ChessPiece capturedPiece;
    CastlingStateInfo castlingState;
    EnPassantStateInfo enPassantState;
    u64 hash;
    PestoAccumulator pesto;  // search's accumulator before the move, restored on unmake like the hash
    u16 plyCount;
};
