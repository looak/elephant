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
 * Tuned by ElephantTuner, every table here together with PeSTO's pawn table, the other piece square tables, material
 * and mobility kept (--tune psqt_pawn,pawns_passed,pawns_structure,pawns_passed_extras on E12.52-1M-D12-Resolved,
 * K 1.1324, 1000 passes at rate 1, error 0.086746 -> 0.086180, held back positions 0.65% lower). First fitted against
 * PeSTO's own pawn table: the structure terms (K 1.1537, held back 1.56% lower), then the passed pawn extras with the
 * passed table (K 1.1269, 0.72% lower).    */

#pragma once
#include <system/platform.hpp>
#include <eval/tapered_score.hpp>

namespace evaluator_data
{

// a passed pawn, relative rank 2 to 7.
constexpr TaperedScore passedPawn[6] = {
    S( -12, -31), S( -12, -20), S(   1,  11), S(  33,  41), S(  67,  74), S(  49, -12),
};

// an isolated pawn, file a & h to d & e.
constexpr TaperedScore isolatedPawn[4] = {
    S(  10,  -2), S(  -6, -10), S(  -9, -12), S( -10, -16),
};

// a doubled pawn, one for each pawn with one of its own in front of it, file a & h to d & e.
constexpr TaperedScore doubledPawn[4] = {
    S( -33, -34), S( -13, -19), S(  -4,  -9), S( -10,  -7),
};

// a backward pawn, file a & h to d & e.
constexpr TaperedScore backwardPawn[4] = {
    S(   2, -14), S(  -2, -17), S(  -4, -18), S(  -4, -22),
};

// a connected pawn, beside or defended by one of its own, relative rank 2 to 7.
constexpr TaperedScore connectedPawn[6] = {
    S(   1,  -1), S(  14,   4), S(  13,   4), S(  15,  12), S(  34,  42), S( 200,  16),
};

// passed pawn extras, see passed_pawns.hpp.

// a passed pawn, by how far its own king is from it, 0 to 7 squares (0 can't happen).
constexpr TaperedScore passedOwnKing[8] = {
    S(   0,   0), S(  24,  17), S(  -3,  10), S( -11,  -4), S(  -6, -15), S(  -1, -19), S(  24, -29), S(  21, -44),
};

// a passed pawn, by how far the enemy king is from it, 0 to 7 squares (0 can't happen).
constexpr TaperedScore passedEnemyKing[8] = {
    S(   0,   0), S( -13, -48), S(  13,   4), S(   6,  25), S(  10,  34), S(  10,  42), S(  -8,  54), S( -21,  64),
};

// a passed pawn with a piece on the square in front of it, relative rank 2 to 7.
constexpr TaperedScore passedBlocked[6] = {
    S( -18,  -9), S( -20,   7), S( -20,  -7), S( -16, -13), S(  -9, -38), S(  -7, -35),
};

// a passed pawn with nothing in front of it up to the promotion square, relative rank 2 to 7.
constexpr TaperedScore passedFreePath[6] = {
    S( -10,   4), S( -16,   2), S( -18,   6), S( -19,  12), S( -26,  32), S(   0,  31),
};

// a rook behind a passed pawn on its file, nothing between them: its own side's rook, the enemy's.
constexpr TaperedScore passedRookBehind[2] = {
    S( -11,   0), S( -20, -15),
};

} // namespace evaluator_data
