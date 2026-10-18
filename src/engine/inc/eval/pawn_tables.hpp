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
 * @file pawn_tables.hpp
 * @brief Pawn structure values, see pawn_structure.hpp. Passed and connected pawns by relative rank 2 to 7, index 0
 * for rank 2; isolated, doubled and backward pawns by file folded onto a to d, index 0 for the a & h files. Written by
 * ElephantTuner, laid out the way it writes them. All constexpr, safe to include anywhere.
 *
 * Tuned by ElephantTuner with PeSTO's and mobility's tables kept as they are (--tune pawns_passed,pawns_structure):
 * E12.52-1M-D12-Resolved, K 1.1537, 1000 passes at rate 1, error 0.088897 -> 0.087448 (held back positions 1.56%
 * lower). They're what's left over after the pawn piece square tables, which already pay for an average pawn on each
 * square: a passed pawn far back scores less than the table gives it, an advanced one more.    */

#pragma once
#include <system/platform.hpp>
#include <eval/tapered_score.hpp>

namespace evaluator_data
{

// a passed pawn, relative rank 2 to 7.
constexpr TaperedScore passedPawn[6] = {
    S( -11, -21), S( -20, -10), S(   2,  18), S(  30,  40), S(  57,  35), S(  55, -10),
};

// an isolated pawn, file a & h to d & e.
constexpr TaperedScore isolatedPawn[4] = {
    S(  16,  -6), S( -10, -10), S( -11,  -8), S( -14, -18),
};

// a doubled pawn, one for each pawn with one of its own in front of it, file a & h to d & e.
constexpr TaperedScore doubledPawn[4] = {
    S( -29, -35), S( -25, -18), S( -10,  -4), S( -16,  -9),
};

// a backward pawn, file a & h to d & e.
constexpr TaperedScore backwardPawn[4] = {
    S(   2, -12), S(  -3, -16), S(  -8, -13), S(  -5, -22),
};

// a connected pawn, beside or defended by one of its own, relative rank 2 to 7.
constexpr TaperedScore connectedPawn[6] = {
    S(   3,   2), S(   5,   8), S(   5,   6), S(   6,   3), S(  25,  11), S( 159,  25),
};

// passed pawn extras, see passed_pawns.hpp. Tuned together with passedPawn above, the rest kept (--tune
// pawns_passed,pawns_passed_extras): E12.52-1M-D12-Resolved, K 1.1269, 1000 passes at rate 1, error 0.087401 ->
// 0.086756 (held back positions 0.72% lower).

// a passed pawn, by how far its own king is from it, 0 to 7 squares (0 can't happen).
constexpr TaperedScore passedOwnKing[8] = {
    S(   0,   0), S(  20,  15), S(  -7,   9), S( -19,  -3), S( -10, -16), S(  -2, -19), S(  22, -27), S(  23, -46),
};

// a passed pawn, by how far the enemy king is from it, 0 to 7 squares (0 can't happen).
constexpr TaperedScore passedEnemyKing[8] = {
    S(   0,   0), S(   0, -48), S(  14,   4), S(   1,  25), S(   1,  34), S(   8,  40), S( -11,  52), S( -30,  65),
};

// a passed pawn with a piece on the square in front of it, relative rank 2 to 7.
constexpr TaperedScore passedBlocked[6] = {
    S( -17,  -8), S( -20,   7), S( -22,  -5), S( -16, -14), S( -10, -40), S(  -2, -34),
};

// a passed pawn with nothing in front of it up to the promotion square, relative rank 2 to 7.
constexpr TaperedScore passedFreePath[6] = {
    S( -10,   4), S( -15,   3), S( -19,   7), S( -20,  12), S( -28,  30), S(   2,  32),
};

// a rook behind a passed pawn on its file, nothing between them: its own side's rook, the enemy's.
constexpr TaperedScore passedRookBehind[2] = {
    S( -10,  -2), S( -19, -16),
};

} // namespace evaluator_data
