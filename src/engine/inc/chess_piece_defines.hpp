// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2023  Alexander Loodin Ek

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
#ifndef CHESS_PIECE_DEFINES_HEADER
#define CHESS_PIECE_DEFINES_HEADER

#include "defines.h"

#define pieceIndexMax 6

enum class PieceType : byte {
    NON = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK = 4,
    QUEEN = 5,
    KING = 6,
    NR_OF_PIECES = 7,
};

enum class Set : byte { WHITE = 0, BLACK = 1, NR_OF_SETS = 2 };

template<Set s>
constexpr Set
opposing_set()
{
    if constexpr (s == Set::WHITE)
        return Set::BLACK;

    return Set::WHITE;
}

constexpr signed short pieceValues[6] = {100, 350, 350, 525, 1000, 10000};
constexpr byte moveCount[6] = {2, 8, 4, 4, 8, 8};

constexpr bool slides[6] = {
    false, false, true, true, true, false,
};

constexpr signed short moves0x88[6][8] = {{-16, -32, 0, 0, 0, 0, 0, 0},       {-33, -31, -18, -14, 14, 18, 31, 33},
                                          {-17, -15, 15, 17, 0, 0, 0, 0},     {-16, -1, 1, 16, 0, 0, 0, 0},
                                          {-17, -16, -15, -1, 1, 15, 16, 17}, {-17, -16, -15, -1, 1, 15, 16, 17}};

constexpr signed short attacks0x88[6][8] = {{-15, -17, 0, 0, 0, 0, 0, 0},       {-33, -31, -18, -14, 14, 18, 31, 33},
                                            {-17, -15, 15, 17, 0, 0, 0, 0},     {-16, -1, 1, 16, 0, 0, 0, 0},
                                            {-17, -16, -15, -1, 1, 15, 16, 17}, {-17, -16, -15, -1, 1, 15, 16, 17}};

constexpr PieceType slidingPieceTypes[3] = {PieceType::BISHOP, PieceType::ROOK, PieceType::QUEEN};

class ChessPieceDef {
public:
    static inline constexpr byte MoveCount(byte pIndex) { return moveCount[pIndex]; };
    static inline constexpr bool Slides(byte pIndex) { return slides[pIndex]; }
    static inline constexpr signed short Moves0x88(byte pIndex, byte mIndex) { return moves0x88[pIndex][mIndex]; }
    static inline constexpr signed short Attacks0x88(byte pIndex, byte mIndex) { return attacks0x88[pIndex][mIndex]; }
    static inline constexpr signed short Value(byte pIndex) { return pieceValues[pIndex]; }
    static inline constexpr bool IsDiagonalMove(signed short mvValue)
    {
        return (mvValue == -17 || mvValue == -15 || mvValue == 15 || mvValue == 17);
    }
};

#endif  // CHESS_PIECE_DEFINES_HEADER