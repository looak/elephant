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
 * @file mobility_tables.hpp
 * @brief Mobility values, see mobility.hpp: for each piece, a TaperedScore by how many safe squares it can move to,
 * index 0 for none. Written by ElephantTuner, laid out the way it writes them. All constexpr, safe to include
 * anywhere.
 *
 * Untuned, every value is S(0, 0): mobility is counted but changes no evaluation until these are tuned.    */

#pragma once
#include <system/platform.hpp>
#include <eval/tapered_score.hpp>

namespace evaluator_data
{

// a knight, 0 to 8 safe squares.
constexpr TaperedScore knightMobility[9] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S(   0,   0),
};

// a bishop, 0 to 13 safe squares.
constexpr TaperedScore bishopMobility[14] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

// a rook, 0 to 14 safe squares.
constexpr TaperedScore rookMobility[15] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

// a queen, 0 to 27 safe squares.
constexpr TaperedScore queenMobility[28] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

// by piece index, knight to queen; pawns and kings have no mobility values.
constexpr const TaperedScore* mobilityTables[6] = {
    nullptr, knightMobility, bishopMobility, rookMobility, queenMobility, nullptr
};

} // namespace evaluator_data
