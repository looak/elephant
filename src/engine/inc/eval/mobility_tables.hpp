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
 * only how much a free piece is worth over a hemmed in one.
 *
 * Rebalanced in 1.4, every term fitted together but material, the king's table and kingDanger (--tune psqt_pawn,
 * psqt_knight, psqt_bishop, psqt_rook, psqt_queen, mobility, pawns_passed, pawns_structure, pawns_passed_extras,
 * king_shelter, pieces_pairs --min-usage 50000 on E12.52-1M-D12-Resolved, K 1.1889, 1000 passes at rate 1, error
 * 0.085192 -> 0.084398, held back 0.87% lower). Each tuned piece square table kept centred on its own average, so
 * the pieces keep their values and only the shapes moved; the 120 values fewer than 50k positions use kept as they
 * were.    */

#pragma once
#include <system/platform.hpp>
#include <eval/tapered_score.hpp>

namespace evaluator_data
{

// a knight, 0 to 8 safe squares.
constexpr TaperedScore knightMobility[9] = {
    S( -13,-169), S(  -6, -54), S(  -4, -19), S(  -3,   2), S(  -1,   8), S(   1,  11), S(   4,  12), S(   8,   8),
    S(  16,  -9),
};

// a bishop, 0 to 13 safe squares.
constexpr TaperedScore bishopMobility[14] = {
    S( -11, -28), S( -12, -25), S(  -8, -22), S(  -4, -12), S(   0,  -1), S(   4,  10), S(   5,  19), S(   3,  22),
    S(   4,  26), S(   9,  22), S(  19,  20), S(  39,  11), S(  52,  17), S( 114, -12),
};

// a rook, 0 to 14 safe squares.
constexpr TaperedScore rookMobility[15] = {
    S( -21, -67), S( -18, -41), S( -14, -34), S( -12, -18), S( -13,  -5), S(  -8,   3), S(  -5,  10), S(   1,  14),
    S(   6,  18), S(  12,  21), S(  17,  24), S(  21,  26), S(  25,  29), S(  41,  18), S(  80,  -2),
};

// a queen, 0 to 27 safe squares.
constexpr TaperedScore queenMobility[28] = {
    S( -22,  23), S(   0,-160), S(  -6,-137), S(  -6, -87), S(  -6, -56), S(  -4, -35), S(  -2, -21), S(  -1,  -9),
    S(   1,   2), S(   2,  12), S(   3,  19), S(   4,  24), S(   5,  30), S(   5,  33), S(   2,  43), S(  -1,  49),
    S(  -5,  54), S(  -1,  51), S(  -4,  54), S(   1,  51), S(   2,  51), S(   8,  39), S(  -6,  45), S(  35,  17),
    S(   0,  31), S(  -3,  36), S(  -1,  51), S(  34,  61),
};

// by piece index, knight to queen; pawns and kings have no mobility values.
constexpr const TaperedScore* mobilityTables[6] = {
    nullptr, knightMobility, bishopMobility, rookMobility, queenMobility, nullptr
};

} // namespace evaluator_data
