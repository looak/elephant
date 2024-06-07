// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021  Alexander Loodin Ek

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
#include <cstdint>
#include "log.h"

namespace platform {
constexpr bool is_win64 =
#ifdef _WIN64
true;
#else
false;
#endif

constexpr bool is_amd64_x86 =
#ifdef __x86_64__
true;
#else
false;
#endif
}  // namespace platform

using byte = std::uint8_t;
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
// typedef __m128 i128;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i64 = std::int64_t;
using i32 = std::int32_t;

const u64 universe = 0xffffffffffffffffULL;

#define to0x88(sqr) sqr + (sqr & ~7)
#define fr0x88(sq0x88) (sq0x88 + (sq0x88 & 7)) >> 1

#define file_a 0
#define file_b 1
#define file_c 2
#define file_d 3
#define file_e 4
#define file_f 5
#define file_g 6
#define file_h 7

#define rank_1 0
#define rank_2 1
#define rank_3 2
#define rank_4 3
#define rank_5 4
#define rank_6 5
#define rank_7 6
#define rank_8 7

#define pawnId 0
#define knightId 1
#define bishopId 2
#define rookId 3
#define queenId 4
#define kingId 5

#define north 0
#define east 1
#define south 2
#define west 3

#define northeast 4
#define southeast 5
#define southwest 6
#define northwest 7

// faster divide by eight
// only works on positive integers
// acording to own tests ~3x faster than regular mod by eight.
#define mod_by_eight(v) (v & 7)

static const std::string c_startPositionFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

enum class Square : uint8_t {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    NullSQ
};

constexpr byte operator*(Square sqr)
{
    return static_cast<byte>(sqr);
}

template<typename T>
constexpr T&
increment(T& value)
{
    static_assert(std::is_integral<std::underlying_type_t<T>>::value, "Can't increment value");
    ((std::underlying_type_t<T>&)value)++;
    return value;
}

#define EG_TESTING
// #define EG_DEBUGGING