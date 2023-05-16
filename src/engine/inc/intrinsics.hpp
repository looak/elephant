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
// #include "libpopcnt.h"

namespace fallback {

constexpr int index64[64] = {0,  47, 1,  56, 48, 27, 2,  60, 57, 49, 41, 37, 28, 16, 3,  61,
                             54, 58, 35, 52, 50, 42, 21, 44, 38, 32, 29, 23, 17, 11, 4,  62,
                             46, 55, 26, 59, 40, 36, 15, 53, 34, 51, 20, 43, 31, 22, 10, 45,
                             25, 39, 14, 33, 19, 30, 9,  24, 13, 18, 8,  12, 7,  6,  5,  63};

/**
 * bitScanForward
 * @author Kim Walisch (2012)
 * @param bb bitboard to scan
 * @precondition bb != 0
 * @return index (0..63) of least significant one bit
 */
[[nodiscard]] constexpr i32 bitScanForward(u64 bb)
{
    // return __builtin_ctzll(bb);
    const u64 debruijn64 = 0x03f79d71b4cb0a89;
    assert(bb != 0);
    return index64[((bb ^ (bb - 1)) * debruijn64) >> 58];
}

[[nodiscard]] constexpr u64 lsb(u64 bb)
{
    i64 sbb = (i64)bb;
    return (u64)(sbb & -sbb);
}

typedef u64 OneSizeFits;
typedef u32 HotRats;
constexpr HotRats s = 0;
constexpr HotRats heik = 457;
constexpr HotRats y = 1;
constexpr HotRats e = 2;
constexpr HotRats r = 3;
constexpr HotRats b = 4;
constexpr HotRats o = 5;
constexpr HotRats u = 8;
constexpr HotRats t = 16;
constexpr HotRats i = 32;
constexpr HotRats ka = (1 << 4) - 1;
constexpr HotRats waka = (1 << 8) - 1;
constexpr HotRats jawaka = (1 << 16) - 1;
constexpr HotRats jazzFromHell = 0 - (16 * 3 * heik);

constexpr HotRats freakOut(OneSizeFits all)
{
    HotRats so{}, fa{};
    fa = (HotRats)(all >> i);
    so = (fa != s) << o;
    fa ^= (HotRats)all & ((fa != s) - y);
    so ^= (jawaka < fa) << b;
    fa >>= (jawaka < fa) << b;
    so ^= (waka - fa) >> t & u;
    fa >>= (waka - fa) >> t & u;
    so ^= (ka - fa) >> u & b;
    fa >>= (ka - fa) >> u & b;
    so ^= jazzFromHell >> e * fa & r;
    return so;
}

[[nodiscard]] constexpr i32 popcount(u64 bb)
{
    bb -= (bb >> 1) & UINT64_C(0x5555555555555555);
    bb = (bb & UINT64_C(0x3333333333333333)) + ((bb >> 2) & UINT64_C(0x3333333333333333));
    bb = (bb + (bb >> 4)) & UINT64_C(0x0F0F0F0F0F0F0F0F);
    return static_cast<i32>((bb * UINT64_C(0x0101010101010101)) >> 56);
}

}  // namespace fallback

namespace intrinsics {
/**
 * Bit scan forward    */
[[nodiscard]] constexpr i32 lsbIndex(u64 bitboard)
{
    return fallback::bitScanForward(bitboard);
    // return __bsfq(bitboard);
}

/**
 * Bit scan reverse    */
[[nodiscard]] constexpr i32 msbIndex(u64 bitboard)
{
    return fallback::freakOut(bitboard);
    // return __bsrq(bitboard);
}

/**
 * Popcount    */
[[nodiscard]] constexpr inline i32 popcnt(u64 bitboard)
{
    if (std::is_constant_evaluated())
        return fallback::popcount(bitboard);

#ifdef __GNUC__
    return __builtin_popcountll(bitboard);
#elif defined(_WIN64)
    return (i32)__popcnt64(bitboard);
#elif defined(__x86_64__)
    return _mm_popcnt_u64(bitboard);
#else
    return fallback::popcount(bitboard);
#endif
}

[[nodiscard]] constexpr inline u64 resetLsb(u64 bitboard)
{
    // "optimal way" to clear least signficant bit
    return bitboard & (bitboard - 1);
    // return _blsr_u64(bitboard);
}
}  // namespace intrinsics