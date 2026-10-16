/******************************************************************************
 * Elephant Gambit Chess Engine - a Chess AI
 * Copyright(C) 2025  Alexander Loodin Ek
 *
 * This program is free software : you can redistribute it and /or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.If not, see < http://www.gnu.org/licenses/>.
 *****************************************************************************/

/**
 * @file evaluator_data.hpp
 * @brief Defines data structures and constants for evaluating chess piece positions and scores
 *
 */

#pragma once
#include <system/platform.hpp>
#include <eval/pesto_tables.hpp>
#include <eval/tapered.hpp>
#include <io/weight_store.hpp>

// Tapered weights are tapered with weight * gamePhase, see eval/tapered.hpp.

namespace evaluator_data
{

/**
 * Idea here is that doubling pawns early or midgame will hurt your structure,
 * later in the game it's not as important where there are less pawns on the board.  */
TAPERED_WEIGHT(doubledPawnScore, i16, -50, -25)

/**
 * Idea here is that isolated pawns are bad but will be worse in the endgame.  */
TAPERED_WEIGHT(isolatedPawnScore, i32, -25, -50)

/**
 * Passed pawns are a strong factor in the endgame and something to strive for. */
TAPERED_WEIGHT(passedPawnScore, i32, 25, 100)

/*
* if the passed pawn is guarded, this will be multiplied with the passedPawnScore. */
MULTIPLIER(guardedPassedPawnBonus, 2)

/*
* Idea here is that pawns that ar guarded by other pawns are stronger and more valuable.*/
WEIGHT(guardedPawnScore, i16, 8)

constexpr i32 center_bias[64] = {
      2,   2,   2,   2,   2,   2,   2,  2,
      2,   4,   4,   4,   4,   4,   4,  2,
      2,   4,   8,   8,   8,   8,   4,  2,
      2,   4,   8,  16,  16,   8,   4,  2,
      2,   4,   8,  16,  16,   8,   4,  2,
      2,   4,   8,   8,   8,   8,   4,  2,
      2,   4,   4,   4,   4,   4,   4,  2,
      2,   2,   2,   2,   2,   2,   2,  2,
};

} // namespace evaluator_data
