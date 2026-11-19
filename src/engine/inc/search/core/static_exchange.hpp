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
 * @file static_exchange.hpp
 * @brief Static exchange evaluation (SEE), what a sequence of captures on one square wins or loses.
 */

#pragma once
#include <material/chess_piece_defines.hpp>
#include <move/move.hpp>

struct MaterialPositionMask;

namespace see {

// piece values of the exchange, the king is never captured so it's worth nothing to it.
inline constexpr i32 value[6] = { 100, 300, 300, 500, 900, 0 };

/**
 * @brief True when the exchange the move starts on its target square gains at least threshold for the side making it.
 * Both sides recapture with their least valuable attacker and may stop when recapturing loses, sliders behind
 * attackers join as the squares in front of them clear. Pins and checks are ignored.
 * @param material Position before the move.
 * @param move A legal move, castling never loses material.
 * @param threshold Minimum gain, in see::value units.   */
bool ge(const MaterialPositionMask& material, PackedMove move, i32 threshold);

} // namespace see
