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
#include <stdint.h>
#include "log.h"

namespace platform
{
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
}

typedef unsigned char byte;
typedef unsigned char u8;
typedef signed short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char i8;
typedef signed short i16;
typedef signed long long i64;
typedef signed int i32;


const u64 universe = 0xffffffffffffffffULL;

#define a_file 0
#define b_file 1
#define c_file 2
#define d_file 3
#define e_file 4
#define f_file 5
#define g_file 6
#define h_file 7

#define pawnId 0
#define knightId 1
#define bishopId 2
#define rookId 3
#define queenId 4
#define kingId 5

static const std::string c_startPositionFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
