// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2023-2025  Alexander Loodin Ek

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
 * @file chess_piece_defines.hpp
 * @brief Defines for chess pieces used in the engine.
 * Contains definitions for piece types, sets, and utility functions.
 * This file is used to define the chess pieces and their properties.
 *
 * @author Alexander Loodin Ek    */
#pragma once

#include "defines.hpp"
#include <map>

namespace piece_constants {
namespace index {
    constexpr byte pawn = 0;
    constexpr byte knight = 1;
    constexpr byte bishop = 2;
    constexpr byte rook = 3;
    constexpr byte queen = 4;
    constexpr byte king = 5;
    constexpr byte max = 6;
}; // namespace index
}; // namespace piece_constants

#define pawnId piece_constants::index::pawn
#define knightId piece_constants::index::knight
#define bishopId piece_constants::index::bishop
#define rookId piece_constants::index::rook
#define queenId piece_constants::index::queen
#define kingId piece_constants::index::king
#define pieceIndexMax piece_constants::index::max

enum class PieceType : byte {
    NONE = 0,
    PAWN = pawnId + 1,
    KNIGHT = knightId + 1,
    BISHOP = bishopId + 1,
    ROOK = rookId + 1,
    QUEEN = queenId + 1,
    KING = kingId + 1,
    NR_OF_PIECES = KING + 1,
};

constexpr u8
toPieceIndex(PieceType type) {
    return static_cast<u8>(type) - 1;
}

enum class Set : byte { WHITE = 0, BLACK = 1, NR_OF_SETS = 2 };

constexpr u8
toSetId(Set set)
{
    return static_cast<u8>(set);
}

template<Set s>
constexpr Set
opposing_set()
{
    if constexpr (s == Set::WHITE)
        return Set::BLACK;

    return Set::WHITE;
}

constexpr u8
opposing_set(u8 set) {
    return set ^ 1;
}

template<Set s>
constexpr i8 pawn_modifier()
{
    if constexpr (s == Set::WHITE)
        return 1;

    return -1;
}

namespace piece_constants {
namespace notation {
inline PieceType fromChar(char c) {        
    switch (c) {
        case 'K': case 'k': return PieceType::KING;
        case 'Q': case 'q': return PieceType::QUEEN;
        case 'R': case 'r': return PieceType::ROOK;
        case 'B': case 'b': return PieceType::BISHOP;
        case 'N': case 'n': return PieceType::KNIGHT;
        // case 'P': case 'p': return PieceType::PAWN;
        // pawns have no letter in SAN; return sentinel if needed:
        default:  return PieceType::NR_OF_PIECES;
    }
}
} // namespace notation

constexpr u16 value[6] = { 100, 350, 350, 525, 975, 10000 };
constexpr byte move_count[6] = { 2, 8, 4, 4, 8, 8 };

constexpr bool slides[6] = {
    false, false, true, true, true, false,
};

constexpr i8 moves0x88[6][8] = { 
    {-16, -32, 0, 0, 0, 0, 0, 0},       {-33, -31, -18, -14, 14, 18, 31, 33},
    {-17, -15, 15, 17, 0, 0, 0, 0},     {-16, -1, 1, 16, 0, 0, 0, 0},
    {-17, -16, -15, -1, 1, 15, 16, 17}, {-17, -16, -15, -1, 1, 15, 16, 17} 
};

constexpr i8 attacks0x88[6][8] = {
    {-15, -17, 0, 0, 0, 0, 0, 0},       {-33, -31, -18, -14, 14, 18, 31, 33},
    {-17, -15, 15, 17, 0, 0, 0, 0},     {-16, -1, 1, 16, 0, 0, 0, 0},
    {-17, -16, -15, -1, 1, 15, 16, 17}, {-17, -16, -15, -1, 1, 15, 16, 17} 
};


} // namespace piece_constants