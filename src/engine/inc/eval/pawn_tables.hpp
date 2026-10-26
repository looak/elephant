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
 * passed table (K 1.1269, 0.72% lower).
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

// a passed pawn, relative rank 2 to 7.
constexpr TaperedScore passedPawn[6] = {
    S( -23, -27), S( -17, -18), S(   2,  10), S(  35,  40), S(  66,  92), S(  59, -11),
};

// an isolated pawn, file a & h to d & e.
constexpr TaperedScore isolatedPawn[4] = {
    S(  13,  -6), S(  -8,  -8), S(  -7, -13), S(  -9, -16),
};

// a doubled pawn, one for each pawn with one of its own in front of it, file a & h to d & e.
constexpr TaperedScore doubledPawn[4] = {
    S( -31, -36), S( -16, -23), S(  -9,  -8), S( -13,  -3),
};

// a backward pawn, file a & h to d & e.
constexpr TaperedScore backwardPawn[4] = {
    S(   3, -14), S(  -2, -16), S(  -5, -18), S(  -5, -21),
};

// a connected pawn, beside or defended by one of its own, relative rank 2 to 7.
constexpr TaperedScore connectedPawn[6] = {
    S(   0,  -2), S(  13,   5), S(  12,   6), S(  15,  15), S(  35,  51), S( 200,  16),
};

// passed pawn extras, see passed_pawns.hpp.

// a passed pawn, by how far its own king is from it, 0 to 7 squares (0 can't happen).
constexpr TaperedScore passedOwnKing[8] = {
    S(   0,   0), S(  23,  23), S(   2,  13), S(  -8,  -1), S(  -6, -13), S(  -3, -20), S(  11, -26), S(   4, -44),
};

// a passed pawn, by how far the enemy king is from it, 0 to 7 squares (0 can't happen).
constexpr TaperedScore passedEnemyKing[8] = {
    S(   0,   0), S( -46, -28), S( -10,   8), S(   0,  22), S(   6,  30), S(  10,  37), S(   5,  46), S(  -3,  56),
};

// a passed pawn with a piece on the square in front of it, relative rank 2 to 7.
constexpr TaperedScore passedBlocked[6] = {
    S( -13,  -3), S( -12,   6), S( -19,  -8), S( -15, -18), S( -16, -38), S(  -9, -35),
};

// a passed pawn with nothing in front of it up to the promotion square, relative rank 2 to 7.
constexpr TaperedScore passedFreePath[6] = {
    S(  -8,   7), S(  -9,   4), S( -11,   7), S( -11,  13), S( -12,  24), S(  14,  31),
};

// a rook behind a passed pawn on its file, nothing between them: its own side's rook, the enemy's.
constexpr TaperedScore passedRookBehind[2] = {
    S( -10,   6), S(  -3, -15),
};

} // namespace evaluator_data
