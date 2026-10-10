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
#include <io/weight_store.hpp>

struct TaperedScore
{
    i32 midgame;
    i32 endgame;
};

inline i32 operator*(const TaperedScore& lhs, const float& rhs)
{
    // rhs is the endgame coefficient, 0 is midgame and 1 is endgame.
    return lhs.midgame + static_cast<i32>((float)(lhs.endgame - lhs.midgame) * rhs);
}

namespace evaluator_data
{

/**
 * Idea here is that doubling pawns early or midgame will hurt your structure,
 * later in the game it's not as important where there are less pawns on the board.  */
 //static constexpr TaperedScore doubledPawnScore{-50, -25};
TAPERED_WEIGHT(doubledPawnScore, i16, -50, -25);

/**
 * Idea here is that isolated pawns are bad but will be worse in the endgame.  */
static constexpr TaperedScore isolatedPawnScore{-25, -50};

/**
 * Passed pawns are a strong factor in the endgame and something to strive for. */
static constexpr TaperedScore passedPawnScore{ 25, 100 };

/*
* if the passed pawn is guarded, this will be multiplied with the passedPawnScore. */
MULTIPLIER(guardedPassedPawnBonus, 2);

/*
* Idea here is that pawns that ar guarded by other pawns are stronger and more valuable.*/
WEIGHT(guardedPawnScore, i16, 8);

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
