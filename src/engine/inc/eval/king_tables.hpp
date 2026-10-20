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
 * at most the one before it, the fit's tail from 48 units rose back towards 0 where few positions are.    */

#pragma once
#include <system/platform.hpp>
#include <eval/tapered_score.hpp>

namespace evaluator_data
{

// the king's own pawn nearest it on a file, file a & h to d & e, then no pawn & relative rank 2 to 7.
constexpr TaperedScore kingShelter[32] = {
    S(  -5,   2), S(  28, -23), S(  30, -15), S(  25,  -8), S(  28,   4), S(  14,  22), S(  79,   2), S(   0,   0),
    S( -29,   9), S(   7,   5), S(  13,  -1), S(  -3,   5), S(  -1,  12), S(   7,  27), S(  36,  23), S(   0,   0),
    S( -12,  -3), S(   9,   0), S(   4,  -2), S(   1,  -1), S(   7,   3), S(  -6,  14), S(  35,  16), S(   0,   0),
    S( -15,   1), S(  17,  -5), S(   7,  -4), S(   2,   5), S(   5,  12), S( -22,  22), S(  -7,  21), S(   0,   0),
};

// the enemy pawn nearest the king on a file, free to advance, file a & h to d & e, then no pawn & the relative rank
// from the king's side 2 to 7.
constexpr TaperedScore kingStorm[32] = {
    S( -24,  -9), S(  35,  78), S( -12,  51), S( -17,  16), S(  -9,  -2), S(  -7,  -6), S( -11,  -4), S(   0,   0),
    S( -18, -10), S(  -5,  85), S( -22,  52), S( -11,   6), S(  -1,  -2), S(  12,  -8), S(   5,  -5), S(   0,   0),
    S(   0, -14), S( -12,  59), S( -24,  33), S(   9,  -3), S(  10,  -8), S(  14,  -9), S(  19, -12), S(   0,   0),
    S(  -2,  -9), S(-135,  66), S( -42,  26), S(  -6,  -2), S(  10,  -8), S(  21,  -9), S(  19, -11), S(   0,   0),
};

// the enemy pawn nearest the king on a file, blocked by the king's own pawn right in front of it, by the relative
// rank from the king's side, 0 & 1 can't happen.
constexpr TaperedScore kingStormBlocked[8] = {
    S(   0,   0), S(   0,   0), S(  -4, -32), S(  10, -19), S(   8, -22), S(   6, -28), S(   8, -36), S(   0,   0),
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
