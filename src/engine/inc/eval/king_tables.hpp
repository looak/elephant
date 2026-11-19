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
 * relative rank alone. Written by phant-tuner, laid out the way it writes them. All constexpr, safe to include
 * anywhere.
 *
 * Tuned by phant-tuner, every other value kept, on E12.52-1M-D12-Resolved (9.5M positions, 1000 passes at rate 1).
 * Shelter & storm: --tune king_shelter, K 1.1798, error 0.086174 -> 0.085840, held back 0.38% lower. kingDanger: then
 * --tune king_attack, K 1.1959, error 0.085820 -> 0.085285, held back 0.61% lower; made monotonic afterwards, each value
 * at most the one before it, the fit's tail from 48 units rose back towards 0 where few positions are.
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

// the king's own pawn nearest it on a file, file a & h to d & e, then no pawn & relative rank 2 to 7.
constexpr TaperedScore kingShelter[32] = {
    S( -17,  19), S(  22, -16), S(  23, -12), S(  19,  -3), S(  -9,  18), S(  15,  29), S(  79,   2), S(   0,   0),
    S( -22,  26), S(  19,   4), S(   6,   6), S(  -9,  14), S( -14,  21), S(  84,   8), S(  36,  23), S(   0,   0),
    S(  11,  10), S(  45,   1), S(  31,   1), S(  21,   2), S(  13,  13), S(  34,  13), S(  35,  16), S(   0,   0),
    S( -16,  11), S(  -2,  14), S(  -5,   4), S( -16,  17), S( -14,  29), S( -69,  39), S(  -7,  21), S(   0,   0),
};

// the enemy pawn nearest the king on a file, free to advance, file a & h to d & e, then no pawn & the relative rank
// from the king's side 2 to 7.
constexpr TaperedScore kingStorm[32] = {
    S( -25, -55), S(  35,  78), S( -33,  43), S( -47,  -6), S(  -5, -41), S(  -5, -53), S( -10, -49), S(   0,   0),
    S(   0, -65), S(  -5,  85), S( -27,  54), S( -16, -32), S(   1, -48), S(   4, -57), S(  -5, -56), S(   0,   0),
    S(  21, -63), S( -12,  59), S( -89,  36), S( -12, -26), S(  18, -52), S(  27, -59), S(  25, -66), S(   0,   0),
    S(   7, -53), S(-135,  66), S( -54,  22), S( -12, -39), S(  14, -55), S(  19, -48), S(  18, -45), S(   0,   0),
};

// the enemy pawn nearest the king on a file, blocked by the king's own pawn right in front of it, by the relative
// rank from the king's side, 0 & 1 can't happen.
constexpr TaperedScore kingStormBlocked[8] = {
    S(   0,   0), S(   0,   0), S( -12, -64), S(  15, -61), S(  17, -68), S(  14, -75), S(  -5, -76), S(   0,   0),
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
