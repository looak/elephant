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
 * @file pieces.hpp
 * @brief Piece terms, scored from piece_tables.hpp. PeSTO values each piece on its own; these say what the pieces are
 * worth together.
 *
 * Piece pairs, for each side:
 *  - bishop pair: bishops on both colours, together they reach every square. Two on one colour, after a promotion,
 *    aren't a pair.
 *  - knight pair & rook pair: two or more. Kaufman has them worth a little less than alone, they do the same work;
 *    fitted on top of PeSTO they came out a bonus in the endgame instead, smaller than the bishop pair's.
 *
 * Outposts, for each knight & bishop on one: a square on its side's 4th to 6th rank that no enemy pawn attacks or can
 * attack after advancing, worth more when one of its own pawns defends it. Only pieces can drive it off, and the
 * piece square tables can't tell those squares from others. Which squares are outposts comes from PawnInfo, cached by
 * the pawns.
 *
 * The trace sees every term, the tuner fits the tables on them, the evaluation passes NoPieceTrace.    */

#pragma once

#include <bitboard/attacks/attacks.hpp>
#include <bitboard/bitboard_constants.hpp>
#include <eval/pawn_structure.hpp>
#include <eval/piece_tables.hpp>
#include <eval/tapered_score.hpp>
#include <material/chess_piece_defines.hpp>
#include <material/material_mask.hpp>

// What pieceScore reports to a trace: a piece pair by its piece index (knight 1 to rook 3), and a piece on an outpost
// by its piece index (knight 1, bishop 2) and 0 undefended or 1 defended by a pawn; each with its sign, +1 white & -1
// black. The score is the sum of sign * the term's value.
struct NoPieceTrace {
    constexpr void piecePair(u32, i32) {}
    constexpr void outpost(u32, u32, i32) {}
};

namespace piece_internals {

// one side's pieces, scored from its point of view.
template<Set us, typename Trace>
TaperedScore sideScore(const MaterialPositionMask& material, const PawnInfo& pawns, Trace& trace) {
    constexpr i32 sign = us == Set::WHITE ? 1 : -1;

    TaperedScore score = S(0, 0);
    auto pair = [&](u32 pieceId) {
        score += evaluator_data::piecePair[pieceId - knightId];
        trace.piecePair(pieceId, sign);
    };

    if (material.knights<us>().count() >= 2)
        pair(knightId);
    const u64 bishops = material.bishops<us>().read();
    if ((bishops & board_constants::lightSquares) != 0 && (bishops & board_constants::darkSquares) != 0)
        pair(bishopId);
    if (material.rooks<us>().count() >= 2)
        pair(rookId);

    const u64 outposts = pawns.outpostsOf(us);
    const u64 defended = attacks::pawnAttacks<us>(material.pawns<us>().read());
    auto onOutposts = [&](u32 pieceId, const TaperedScore (&values)[2], u64 pieces) {
        for (pieces &= outposts; pieces != 0; pieces &= pieces - 1) {
            const u32 isDefended = (defended & pieces & (~pieces + 1)) != 0 ? 1 : 0;
            score += values[isDefended];
            trace.outpost(pieceId, isDefended, sign);
        }
    };
    onOutposts(knightId, evaluator_data::knightOutpost, material.knights<us>().read());
    onOutposts(bishopId, evaluator_data::bishopOutpost, bishops);
    return score;
}

} // namespace piece_internals

/**
 * Piece terms of both sides, white's minus black's, midgame & endgame packed. pawns is material's PawnInfo.    */
template<typename Trace>
[[nodiscard]] TaperedScore pieceScore(const MaterialPositionMask& material, const PawnInfo& pawns, Trace& trace) {
    return piece_internals::sideScore<Set::WHITE>(material, pawns, trace)
         - piece_internals::sideScore<Set::BLACK>(material, pawns, trace);
}

[[nodiscard]] inline TaperedScore pieceScore(const MaterialPositionMask& material, const PawnInfo& pawns) {
    NoPieceTrace none;
    return pieceScore(material, pawns, none);
}
