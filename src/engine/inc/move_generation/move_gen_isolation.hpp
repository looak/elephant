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
 * @file move_gen_isolation.hpp
 * @brief PieceIsolator class used by move generation to isolate moves from a bulk generated bitboard for a specific piece.
 *
 * @author Alexander Loodin Ek */
#pragma once
#include <bitboard.hpp>
#include <material/chess_piece_defines.hpp>
#include <move_generation/king_pin_threats.hpp>
#include <position/position_accessors.hpp>

struct MovesMask {
    Bitboard quiets;
    Bitboard captures;
};

template<Set us, u8 pieceId>
class PieceIsolator
{
public:
    PieceIsolator(PositionReader position, Bitboard moveMask, const KingPinThreats<us>& pinThreats)
        : m_position(position), m_movesMask(moveMask), m_pinThreats(pinThreats) {}
    
    inline MovesMask isolate(Square src) const;

private:
    const KingPinThreats<us>& m_pinThreats;
    PositionReader m_position;
    Bitboard m_movesMask;
};
