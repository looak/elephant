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
 * @file bitboard_constants.hpp
 * @brief Defines constants and utility functions for bitboard representation in chess applications 
 * static constexpr for header array constant
 * inline constexpr for header non-array constants
 * 
 */

#pragma once
#include <array>
#include <system/platform.hpp>
#include <math/math.hpp>
#include <math/cast.hpp>

namespace coordinates {
inline constexpr u8 rank_1 = 0;
inline constexpr u8 rank_2 = 1;
inline constexpr u8 rank_3 = 2;
inline constexpr u8 rank_4 = 3;
inline constexpr u8 rank_5 = 4;
inline constexpr u8 rank_6 = 5;
inline constexpr u8 rank_7 = 6;
inline constexpr u8 rank_8 = 7;

inline constexpr u8 file_a = 0;
inline constexpr u8 file_b = 1;
inline constexpr u8 file_c = 2;
inline constexpr u8 file_d = 3;
inline constexpr u8 file_e = 4;
inline constexpr u8 file_f = 5;
inline constexpr u8 file_g = 6;
inline constexpr u8 file_h = 7;
}



namespace cardinal_constants {    
inline constexpr u8 north = 0;
inline constexpr u8 east = 1;
inline constexpr u8 south = 2;
inline constexpr u8 west = 3;
inline constexpr u8 northeast = 4;
inline constexpr u8 southeast = 5;
inline constexpr u8 southwest = 6;
inline constexpr u8 northwest = 7;

} // namespace cardinal_constants

namespace shifts {
inline constexpr i8 horizontal = 1;
inline constexpr i8 vertical = 8;
/**
 * Naming comes from forward slash and backslash relative to the set */
inline constexpr i8 forward_diagonal = 9;
inline constexpr i8 backward_diagonal = 7;

static constexpr std::array<i8, 8> shifts = {
    vertical,               // lsh - north
    horizontal,             // lsh - east
    -vertical,              // rsh - south
    -horizontal,            // rsh - west
    forward_diagonal,       // lsh - northeast
    -backward_diagonal,     // rsh - southeast
    -forward_diagonal,      // rsh - southwest
    backward_diagonal       // lsh - northwest
};
}  // namespace shifts

namespace board_constants {
inline constexpr u64 rank0Mask = UINT64_C(0x00000000000000FF);
inline constexpr u64 rank1Mask = UINT64_C(0x000000000000FF00);
inline constexpr u64 rank2Mask = UINT64_C(0x0000000000FF0000);
inline constexpr u64 rank3Mask = UINT64_C(0x00000000FF000000);
inline constexpr u64 rank4Mask = UINT64_C(0x000000FF00000000);
inline constexpr u64 rank5Mask = UINT64_C(0x0000FF0000000000);
inline constexpr u64 rank6Mask = UINT64_C(0x00FF000000000000);
inline constexpr u64 rank7Mask = UINT64_C(0xFF00000000000000);

inline constexpr u64 fileaMask = UINT64_C(0x0101010101010101);
inline constexpr u64 filebMask = UINT64_C(0x0202020202020202);
inline constexpr u64 filecMask = UINT64_C(0x0404040404040404);
inline constexpr u64 filedMask = UINT64_C(0x0808080808080808);
inline constexpr u64 fileeMask = UINT64_C(0x1010101010101010);
inline constexpr u64 filefMask = UINT64_C(0x2020202020202020);
inline constexpr u64 filegMask = UINT64_C(0x4040404040404040);
inline constexpr u64 filehMask = UINT64_C(0x8080808080808080);

static constexpr u64 forwardDiagonalMasks[15] = {0x0100000000000000, 0x0201000000000000, 0x0402010000000000, 0x0804020100000000,
                                                 0x1008040201000000, 0x2010080402010000, 0x4020100804020100, 0x8040201008040201,
                                                 0x0080402010080402, 0x0000804020100804, 0x0000008040201008, 0x0000000080402010,
                                                 0x0000000000804020, 0x0000000000008040, 0x0000000000000080};

static constexpr u64 backwardDiagonalMasks[15] = {
    0x0000000000000001, 0x0000000000000102, 0x0000000000010204, 0x0000000001020408, 0x0000000102040810,
    0x0000010204081020, 0x0001020408102040, 0x0102040810204080, 0x0204081020408000, 0x0408102040800000,
    0x0810204080000000, 0x1020408000000000, 0x2040800000000000, 0x4080000000000000, 0x8000000000000000,
};

inline constexpr u64 darkSquares = UINT64_C(0xAA55AA55AA55AA55);
inline constexpr u64 lightSquares = UINT64_C(0x55AA55AA55AA55AA);

static constexpr u64 rankMasks[8] = {rank0Mask, rank1Mask, rank2Mask, rank3Mask, rank4Mask, rank5Mask, rank6Mask, rank7Mask};

static constexpr u64 fileMasks[8] = {fileaMask, filebMask, filecMask, filedMask, fileeMask, filefMask, filegMask, filehMask};

static constexpr u64 boundsRelativeMasks[2][4] = {{rank7Mask, filehMask, rank0Mask, fileaMask},
                                                  {rank0Mask, fileaMask, rank7Mask, filehMask}};

static constexpr u64 enPassantRankRelative[2] = {rank3Mask, rank4Mask};
static constexpr u64 baseRankRelative[2] = {rank0Mask, rank7Mask};

consteval std::array<std::array<i16, 64>, 64>
generateManhattanDistances() {
    std::array<std::array<i16, 64>, 64> result{};
    for (i64 i = 0; i < 64; ++i)
    {
        for (i64 j = 0; j < 64; ++j)
        {
            i64 rankDiff = (i / 8) - (j / 8);
            i64 fileDiff = (i % 8) - (j % 8);

            size_t a = checked_cast<size_t>(i);
            size_t b = checked_cast<size_t>(j);
            result[a][b] = checked_cast<i16>(constexprAbs(rankDiff) + constexprAbs(fileDiff));
        }
    }

    return result;
}

consteval std::array<i16, 64> generateManhattanDistanceFromCenter() {
    std::array<i16, 64> result{};
    for (i64 i = 0; i < 64; ++i)
    {
        i64 rankDiff = (i / 8) - 3;
        i64 fileDiff = (i % 8) - 3;
        size_t a = checked_cast<size_t>(i);
        result[a] = checked_cast<i16>(constexprAbs(rankDiff) + constexprAbs(fileDiff));
    }

    return result;
}

static constexpr auto manhattanDistances = generateManhattanDistances();
static constexpr auto manhattanDistanceFromCenter = generateManhattanDistanceFromCenter();

}  // namespace board_constants

namespace pawn_constants {
static constexpr u64 baseRank[2] = {board_constants::rank2Mask, board_constants::rank5Mask};
static constexpr u64 promotionRank[2] = {board_constants::rank7Mask, board_constants::rank0Mask};
static constexpr u64 moveMask[2] = {UINT64_C(0x10100), UINT64_C(0x101)};
}  // namespace pawn_constants

namespace king_constants {
inline constexpr u64 moveMask = UINT64_C(0x70507);
inline constexpr i32 maskOffset = 9;
inline constexpr u64 queenSideCastleMask = board_constants::filecMask;
inline constexpr u64 kingSideCastleMask = board_constants::filegMask;
}  // namespace king_constants

constexpr std::array<u64, 64>
generateSquareLookupTable()
{
    std::array<u64, 64> result{};
    for (size_t i = 0; i < 64; ++i)
        result[i] = UINT64_C(1) << i;

    return result;
}
/**
 * Lookup table for per-square 64bit masks. Will match the Square class enum     */
static constexpr auto squareMaskTable = generateSquareLookupTable();