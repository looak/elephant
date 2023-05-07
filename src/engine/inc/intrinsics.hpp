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

#include "defines.h"
#include "libpopcnt.h"


/**
 * Bit scan forward    */
[[nodiscard]] constexpr i32 lsbIndex(u64 bitboard)
{
    return __bsfq(bitboard);
}

/**
 * Bit scan reverse    */
[[nodiscard]] constexpr i32 msbIndex(u64 bitboard)
{
    return __bsrq(bitboard);
}

/**
 * Popcount    */
[[nodiscard]] constexpr i32 popcnt(u64 bitboard)
{
    return popcnt64(bitboard);
}

[[nodiscard]] constexpr u64 resetLsb(u64 bitboard)
{
    //return _blsr_u64(bitboard);
    return bitboard & (bitboard - 1);
}