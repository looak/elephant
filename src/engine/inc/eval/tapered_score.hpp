// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2025  Alexander Loodin Ek

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
 * @file tapered_score.hpp
 * @brief TaperedScore, a midgame & endgame value packed into one 32-bit integer: midgame in the low 16 bits, endgame
 * in the high 16. Evaluation terms are written S(mg, eg), summed as one value and tapered once at the end with taper().
 *
 * Only add, subtract, negate and multiply by an integer: those work on both halves at once, the low half's carry or
 * borrow into the high half is undone when the endgame value is read. Dividing or scaling needs the halves read out
 * first. The halves have to fit an i16 when read, sums in between may wrap. Arithmetic is done on u32 so wrapping is
 * defined behaviour.    */

#pragma once
#include <system/platform.hpp>

enum class TaperedScore : i32 {};

[[nodiscard]] constexpr TaperedScore S(i32 mg, i32 eg) {
    return static_cast<TaperedScore>((static_cast<u32>(eg) << 16) + static_cast<u32>(mg));
}

[[nodiscard]] constexpr i16 mg_score(TaperedScore s) {
    return static_cast<i16>(static_cast<u16>(static_cast<u32>(s)));
}

// + 0x8000 undoes the borrow a negative midgame half took from the endgame half.
[[nodiscard]] constexpr i16 eg_score(TaperedScore s) {
    return static_cast<i16>(static_cast<u16>((static_cast<u32>(s) + 0x8000u) >> 16));
}

[[nodiscard]] constexpr TaperedScore operator+(TaperedScore a, TaperedScore b) {
    return static_cast<TaperedScore>(static_cast<u32>(a) + static_cast<u32>(b));
}
[[nodiscard]] constexpr TaperedScore operator-(TaperedScore a, TaperedScore b) {
    return static_cast<TaperedScore>(static_cast<u32>(a) - static_cast<u32>(b));
}
[[nodiscard]] constexpr TaperedScore operator-(TaperedScore s) {
    return static_cast<TaperedScore>(0u - static_cast<u32>(s));
}
[[nodiscard]] constexpr TaperedScore operator*(TaperedScore s, i32 n) {
    return static_cast<TaperedScore>(static_cast<u32>(s) * static_cast<u32>(n));
}
[[nodiscard]] constexpr TaperedScore operator*(i32 n, TaperedScore s) { return s * n; }
constexpr TaperedScore& operator+=(TaperedScore& a, TaperedScore b) { return a = a + b; }
constexpr TaperedScore& operator-=(TaperedScore& a, TaperedScore b) { return a = a - b; }

namespace evaluator_data
{

// game phase with all pieces on the board. Promotions can push the phase past it, taper caps it there.
constexpr i32 maxGamePhase = 24;

// the midgame half at maxGamePhase and above, the endgame half at 0, linear in between.
[[nodiscard]] constexpr i32 taper(TaperedScore s, i32 gamePhase) {
    const i32 phase = gamePhase < maxGamePhase ? gamePhase : maxGamePhase;
    return (mg_score(s) * phase + eg_score(s) * (maxGamePhase - phase)) / maxGamePhase;
}

} // namespace evaluator_data
