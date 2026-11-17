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
 * index 0 for none. Written by phant-tuner, laid out the way it writes them. All constexpr, safe to include
 * anywhere.
 *
 * Tuned by phant-tuner with PeSTO's tables kept as they are (--tune mobility): E12.52-1M-D12-Resolved, K 1.1545,
 * 1000 passes at rate 1, error 0.089893 -> 0.088943 (held back positions 1.08% lower). Each table is centred, its
 * average weighted by how often each count occurs is 0, so mobility doesn't change what a piece is worth on average,
 * only how much a free piece is worth over a hemmed in one.
 *
 * Rebalanced in 1.4, every term fitted together but material, the king's table and kingDanger (--tune psqt_pawn,
 * psqt_knight, psqt_bishop, psqt_rook, psqt_queen, mobility, pawns_passed, pawns_structure, pawns_passed_extras,
 * king_shelter, pieces_pairs --min-usage 50000 on E12.52-1M-D12-Resolved, K 1.1889, 1000 passes at rate 1, error
 * 0.085192 -> 0.084398, held back 0.87% lower). Each tuned piece square table kept centred on its own average, so
 * the pieces keep their values and only the shapes moved; the 120 values fewer than 50k positions use kept as they
 * were.
 *
 * Then fine-tuned on Elephant's own games, the same groups, kept values and centring as the rebalance (--min-usage
 * 6000 on elephant-positions-0.1, 1.2M positions from nyayo, K 1.2633): its held back error 1.23% lower, but
 * E12.52's 1.13% higher (0.084365 -> 0.085321, K 1.2302 -> 1.1608), the two datasets disagree.
 *
 * Then refitted on Elephant's games since 1.4, the same groups, kept values and centring (--min-usage 7250 on
 * positions-3, 1.45M positions from nyayo's games since 2026-10-30, K 1.3545): its held back error 0.57% lower.
 * Unlike the first own-games fit it agrees with E12.52, whose error is 0.26% lower (0.085307 -> 0.085085), while
 * elephant-positions-0.1's is 0.74% higher, the older games those tables were fitted on.    */

#pragma once
#include <system/platform.hpp>
#include <eval/tapered_score.hpp>

namespace evaluator_data
{

// a knight, 0 to 8 safe squares.
constexpr TaperedScore knightMobility[9] = {
    S( -40,-204), S( -20, -70), S( -12, -20), S(  -6,   2), S(   0,   8), S(   4,  11), S(   8,  12), S(  14,   6),
    S(  20,  -8),
};

// a bishop, 0 to 13 safe squares.
constexpr TaperedScore bishopMobility[14] = {
    S( -23, -33), S( -18, -33), S(  -9, -23), S(  -5, -10), S(   0,   0), S(   3,  10), S(   5,  16), S(   6,  18),
    S(   8,  20), S(  11,  21), S(  13,  23), S(  30,  19), S(  52,  17), S( 159, -15),
};

// a rook, 0 to 14 safe squares.
constexpr TaperedScore rookMobility[15] = {
    S( -37, -18), S( -24, -21), S( -18, -17), S( -15, -11), S( -16,  -3), S( -10,   1), S(  -5,   7), S(   1,   7),
    S(   6,  12), S(  13,  13), S(  23,  10), S(  25,  11), S(  33,  10), S(  39,   6), S(  85, -17),
};

// a queen, 0 to 27 safe squares.
constexpr TaperedScore queenMobility[28] = {
    S( -22,  23), S(   0,-160), S(  -7, -96), S(  -7, -44), S(  -5, -42), S(  -5, -22), S(  -2, -18), S(  -1,  -7),
    S(   2,  -2), S(   0,  11), S(   3,   9), S(   3,  17), S(  -1,  29), S(   3,  27), S(   0,  32), S(  -1,  40),
    S(  -1,  39), S(   9,  33), S(   0,  40), S(  14,  32), S(  32,  20), S(  24,  35), S(  -6,  45), S(  35,  17),
    S(   0,  31), S(  -3,  36), S(  -1,  51), S(  34,  61),
};

// by piece index, knight to queen; pawns and kings have no mobility values.
constexpr const TaperedScore* mobilityTables[6] = {
    nullptr, knightMobility, bishopMobility, rookMobility, queenMobility, nullptr
};

} // namespace evaluator_data
