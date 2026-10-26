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
 * @file king_tables.hpp
 * @brief King shelter & pawn storm values, see king_safety.hpp. Shelter & storm by file folded onto a to d, eight
 * values each, index 0 for no pawn and 1 to 6 for the pawn's relative rank 2 to 7 (7 can't happen); blocked storm by
 * relative rank alone. Written by ElephantTuner, laid out the way it writes them. All constexpr, safe to include
 * anywhere.
 *
 * Tuned by ElephantTuner, every other value kept, on E12.52-1M-D12-Resolved (9.5M positions, 1000 passes at rate 1).
 * Shelter & storm: --tune king_shelter, K 1.1798, error 0.086174 -> 0.085840, held back 0.38% lower. kingDanger: then
 * --tune king_attack, K 1.1959, error 0.085820 -> 0.085285, held back 0.61% lower; made monotonic afterwards, each value
 * at most the one before it, the fit's tail from 48 units rose back towards 0 where few positions are.
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

// the king's own pawn nearest it on a file, file a & h to d & e, then no pawn & relative rank 2 to 7.
constexpr TaperedScore kingShelter[32] = {
    S( -11,   7), S(  32, -36), S(  31, -20), S(  20,  -9), S(  18,   5), S(   9,  21), S(  79,   2), S(   0,   0),
    S( -22,  15), S(  19,   4), S(  12,  -1), S(  -4,   5), S(  -2,  13), S(  20,  21), S(  36,  23), S(   0,   0),
    S( -14,   0), S(  16,   4), S(  -2,   0), S(  -6,  -1), S(   7,   1), S(   4,   7), S(  35,  16), S(   0,   0),
    S( -17,   0), S(   5,  -4), S(  -4,  -2), S( -11,  10), S( -11,  18), S( -37,  24), S(  -7,  21), S(   0,   0),
};

// the enemy pawn nearest the king on a file, free to advance, file a & h to d & e, then no pawn & the relative rank
// from the king's side 2 to 7.
constexpr TaperedScore kingStorm[32] = {
    S( -18, -34), S(  35,  78), S( -49,  62), S( -26,   6), S( -12, -18), S(  -6, -22), S(  -6, -16), S(   0,   0),
    S(  -9, -35), S(  -5,  85), S( -63,  59), S( -13,  -8), S(   6, -17), S(  18, -27), S(   6, -28), S(   0,   0),
    S(  17, -34), S( -12,  59), S( -52,  34), S(  -2, -14), S(  20, -25), S(  24, -30), S(  29, -39), S(   0,   0),
    S(   7, -23), S(-135,  66), S( -49,  25), S( -10, -15), S(   8, -24), S(  23, -24), S(  21, -24), S(   0,   0),
};

// the enemy pawn nearest the king on a file, blocked by the king's own pawn right in front of it, by the relative
// rank from the king's side, 0 & 1 can't happen.
constexpr TaperedScore kingStormBlocked[8] = {
    S(   0,   0), S(   0,   0), S( -20, -49), S(  10, -34), S(  13, -39), S(  14, -47), S(  15, -49), S(   0,   0),
};

// king attack, see king_safety.hpp: what each enemy piece adds to the attack units per square of the king zone it
// reaches, by piece index (pawn & king add none), and what a file beside the king adds when the enemy has a rook or
// queen: open, or with none of the king's pawns. Hand-set, the fit tunes kingDanger on them.
constexpr u32 kingAttackUnits[6] = { 0, 2, 2, 3, 5, 0 };
constexpr u32 kingOpenFileUnits = 3;
constexpr u32 kingHalfOpenFileUnits = 2;

// the king's danger by its attack units, 0 to 63 (more count as 63), 0 when fewer than two pieces attack.
constexpr TaperedScore kingDanger[64] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,  -9), S( -14,  -9), S( -14,  -9), S( -14,  -9),
    S( -14,  -9), S( -14, -13), S( -18, -13), S( -18, -13), S( -18, -14), S( -27, -14), S( -27, -14), S( -33, -14),
    S( -33, -14), S( -33, -14), S( -49, -14), S( -49, -14), S( -65, -14), S( -65, -14), S( -65, -14), S( -78, -14),
    S( -78, -24), S( -96, -24), S( -96, -33), S( -96, -33), S(-111, -33), S(-111, -48), S(-119, -48), S(-119, -70),
    S(-120, -70), S(-144, -70), S(-161, -70), S(-174, -70), S(-174, -70), S(-179, -70), S(-193, -70), S(-194, -70),
    S(-213, -70), S(-213, -70), S(-221, -70), S(-240, -70), S(-240,-121), S(-240,-121), S(-264,-135), S(-264,-160),
    S(-264,-160), S(-264,-173), S(-264,-173), S(-264,-173), S(-264,-173), S(-264,-173), S(-264,-173), S(-264,-173),
    S(-264,-173), S(-264,-173), S(-264,-173), S(-264,-173), S(-264,-173), S(-264,-173), S(-264,-173), S(-264,-173),
};

} // namespace evaluator_data
