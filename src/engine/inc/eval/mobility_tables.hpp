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
 * Tuned by ElephantTuner with PeSTO's tables kept as they are (--tune mobility): E12.52-1M-D12-Resolved, K 1.1545,
 * 1000 passes at rate 1, error 0.089893 -> 0.088943 (held back positions 1.08% lower). Each table is centred, its
 * average weighted by how often each count occurs is 0, so mobility doesn't change what a piece is worth on average,
 * only how much a free piece is worth over a hemmed in one.    */

#pragma once
#include <system/platform.hpp>
#include <eval/tapered_score.hpp>

namespace evaluator_data
{

// a knight, 0 to 8 safe squares.
constexpr TaperedScore knightMobility[9] = {
    S(  -7,-146), S(   0, -53), S(   1, -20), S(  -2,   1), S(  -2,   7), S(  -1,  11), S(   1,  12), S(   5,  10),
    S(  15,  -7),
};

// a bishop, 0 to 13 safe squares.
constexpr TaperedScore bishopMobility[14] = {
    S(  -9, -32), S( -11, -24), S(  -9, -20), S(  -5, -12), S(   1,  -2), S(   5,  10), S(   6,  18), S(   3,  22),
    S(   3,  25), S(   7,  22), S(  16,  19), S(  32,  10), S(  52,  17), S( 114, -12),
};

// a rook, 0 to 14 safe squares.
constexpr TaperedScore rookMobility[15] = {
    S( -16, -61), S( -16, -26), S( -11, -32), S( -11, -19), S( -13,  -4), S(  -9,   2), S(  -5,   9), S(   1,  11),
    S(   7,  15), S(  13,  17), S(  17,  20), S(  18,  22), S(  23,  25), S(  37,  16), S(  61,   7),
};

// a queen, 0 to 27 safe squares.
constexpr TaperedScore queenMobility[28] = {
    S( -22,  23), S(   0,-160), S(  -1,-103), S(  -1, -82), S(  -3, -54), S(  -4, -33), S(  -2, -20), S(  -1, -10),
    S(   1,   0), S(   1,  10), S(   2,  16), S(   3,  23), S(   4,  29), S(   4,  33), S(   1,  41), S(   0,  45),
    S(  -6,  52), S(  -4,  50), S(  -8,  52), S(  -2,  49), S(   4,  47), S(   8,  39), S(  -6,  45), S(  35,  17),
    S(   0,  31), S(  -3,  36), S(  -1,  51), S(  34,  61),
};

// by piece index, knight to queen; pawns and kings have no mobility values.
constexpr const TaperedScore* mobilityTables[6] = {
    nullptr, knightMobility, bishopMobility, rookMobility, queenMobility, nullptr
};

} // namespace evaluator_data
