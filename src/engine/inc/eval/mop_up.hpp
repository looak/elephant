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
 * @file mop_up.hpp
 * @brief Mop-up: in an endgame, the side more than two pawns ahead brings its king to the enemy's and drives the
 * enemy king to the edge, towards a mate search can't see yet.
 *
 * An endgame here is 12 pieces or fewer, kings included, or less than half of the starting material on the board,
 * the starting material counting six rooks instead of four to lean towards the endgame sooner. Material is
 * piece_constants::value, kept by the PestoAccumulator. Inside an endgame mop-up counts in full, outside it not at all.
 *
 * The winning side scores mopUpKingDistance for every square the kings are closer than 14 apart (Manhattan distance)
 * and mopUpLosingKing at the enemy king's square, from evaluator_data. Hand-set and not tapered: Evaluate adds it to the tapered
 * terms, the tuner carries it as a fixed offset.    */

#pragma once

#include <bitboard/bitboard_constants.hpp>
#include <eval/evaluator_data.hpp>
#include <eval/pesto_accumulator.hpp>
#include <material/chess_piece_defines.hpp>
#include <material/material_mask.hpp>

namespace mop_up_internals {

// the non-king material of the starting position, rooks counted six times instead of four.
constexpr i32 c_startingMaterial = piece_constants::value[pawnId] * 16 + piece_constants::value[knightId] * 4
                                 + piece_constants::value[bishopId] * 4 + piece_constants::value[rookId] * 6
                                 + piece_constants::value[queenId] * 2;

// whether mop-up applies at all, see the top of this file.
constexpr bool isEndgame(const PestoAccumulator& accumulator) {
    return accumulator.pieceCount <= 12 || 2 * accumulator.nonKingMaterial < c_startingMaterial;
}

// us's mop-up, lead is us's material minus the enemy's. 0 unless us is more than two pawns ahead.
template<Set us>
i32 sideScore(const MaterialPositionMask& material, i32 lead) {
    if (lead <= 2 * static_cast<i32>(piece_constants::value[pawnId]))
        return 0;
    const u32 usKingSqr = material.king<us>().lsbIndex();
    const u32 opKingSqr = material.king<opposing_set<us>()>().lsbIndex();
    return (14 - board_constants::manhattanDistances[usKingSqr][opKingSqr]) * evaluator_data::mopUpKingDistance
         + evaluator_data::mopUpLosingKing[opKingSqr];
}

} // namespace mop_up_internals

/**
 * Mop-up of both sides, white's minus black's, in centipawns, see the top of this file. accumulator must match
 * material.    */
[[nodiscard]] inline i32 mopUpScore(const MaterialPositionMask& material, const PestoAccumulator& accumulator) {
    if (!mop_up_internals::isEndgame(accumulator))
        return 0;
    return mop_up_internals::sideScore<Set::WHITE>(material, accumulator.material)
         - mop_up_internals::sideScore<Set::BLACK>(material, -accumulator.material);
}
