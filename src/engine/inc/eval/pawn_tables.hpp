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
 * Not tuned yet, all 0: the terms are traced and scored but don't change the evaluation.    */

#pragma once
#include <system/platform.hpp>
#include <eval/tapered_score.hpp>

namespace evaluator_data
{

// a passed pawn, relative rank 2 to 7.
constexpr TaperedScore passedPawn[6] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

// an isolated pawn, file a & h to d & e.
constexpr TaperedScore isolatedPawn[4] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

// a doubled pawn, one for each pawn with one of its own in front of it, file a & h to d & e.
constexpr TaperedScore doubledPawn[4] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

// a backward pawn, file a & h to d & e.
constexpr TaperedScore backwardPawn[4] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

// a connected pawn, beside or defended by one of its own, relative rank 2 to 7.
constexpr TaperedScore connectedPawn[6] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

} // namespace evaluator_data
