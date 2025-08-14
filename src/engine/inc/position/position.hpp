// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021-2025  Alexander Loodin Ek

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
 * @file position.hpp
 * @brief Backend of this chess engine. Represents the chessboard as a few 64bit
 * integers where each bit represents a square on the board. By using this
 * representation we can optimize the move generation and evaluation of the
 * board. The board is built up by 8 bitboards, one for each piece type and one 
 * for each set, combination of which gives position of each players pieces.
 *
 * @author Alexander Loodin Ek    */
#pragma once
#include <defines.hpp>
#include <material/material_mask.hpp>
#include <position/castling_state_info.hpp>
#include <position/en_passant_state_info.hpp>
#include <position/position_accessors.hpp>

/**
 * @brief Represents a chess position, all pieces and what squares they are on.
 * A chess position, represented as a set of bitboards and some bytes of additional state.
 * 64 bytes of material information, by using 2 boards for set and 6 for pieces
 * 1 byte for castling information.
 * 1 byte for enpassant information. 
 * 8 bytes for position hash. */
class Position {
    friend class PositionProxy<PositionEditPolicy>;
    friend class PositionProxy<PositionReadOnlyPolicy>;

public:
    Position();
    Position(const Position& other);
    Position& operator=(const Position& other);

    bool operator==(const Position& other) const;

    PositionEditor edit() { return PositionEditor(*this); }
    PositionReader read() const { return PositionReader(*this); }
    
private:
    MaterialPositionMask m_materialMask;
    CastlingStateInfo m_castlingState;
    EnPassantStateInfo m_enpassantState;

    // TODO: consider moving this somewhere else.
    u64 m_hash = 0;
};