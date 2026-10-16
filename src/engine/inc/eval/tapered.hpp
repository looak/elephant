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
 * @file tapered.hpp
 * @brief The one taper every midgame/endgame pair goes through: PeSTO's sums in PestoAccumulator and every tapered
 * weight. Interpolates on PeSTO's game phase, maxGamePhase with every piece on the board (midgame) down to 0 with only
 * kings & pawns left (endgame). All constexpr, safe to include anywhere.    */

#pragma once
#include <system/platform.hpp>

#include <array>
#include <cstddef>

namespace evaluator_data
{

// game phase with all pieces on the board. Promotions can push the phase past it, taper caps it there.
constexpr i32 maxGamePhase = 24;

// midgame at maxGamePhase and above, endgame at 0, linear in between.
template<typename T>
[[nodiscard]] constexpr T taper(T midgame, T endgame, i32 gamePhase) {
    const i32 phase = gamePhase < maxGamePhase ? gamePhase : maxGamePhase;
    return static_cast<T>((midgame * phase + endgame * (maxGamePhase - phase)) / maxGamePhase);
}

} // namespace evaluator_data

// A midgame & endgame pair, value * gamePhase tapers it.
template<typename T>
struct Tapered {
    T midgame;
    T endgame;

    [[nodiscard]] constexpr T operator*(i32 gamePhase) const {
        return evaluator_data::taper(midgame, endgame, gamePhase);
    }

    bool operator==(const Tapered&) const = default;
};

using TaperedScore = Tapered<i32>;

// Pairs two tables written side by side as published, midgame & endgame, into one table of tapered pairs.
template<typename T, std::size_t N>
[[nodiscard]] constexpr std::array<Tapered<T>, N> zipTapered(const T (&midgame)[N], const T (&endgame)[N]) {
    std::array<Tapered<T>, N> result{};
    for (std::size_t i = 0; i < N; ++i)
        result[i] = { midgame[i], endgame[i] };
    return result;
}
