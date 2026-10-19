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
#include <eval/tapered_score.hpp>

// Midgame & endgame terms are packed TaperedScores, S(mg, eg), tapered with evaluator_data::taper, see
// eval/tapered_score.hpp.

namespace evaluator_data
{

/**
 * Tempo, the side to move's bonus for the move it's about to make: the evaluation scores a position as if neither
 * side were to move. Added by search, from the side to move's view, see SearchThreadContext::evaluate. Hand-set, a
 * quiet position fit doesn't see it.  */
constexpr i16 tempoBonus = 15;

// mop-up's centre bonus by square, 2 on the edge to 16 in the middle, see Evaluator::MopUpValue.
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
