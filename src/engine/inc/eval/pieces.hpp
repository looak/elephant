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
 * Files, for each rook & queen on a file without its own side's pawns: open when no pawn of either side is on it,
 * half-open when the enemy's are. A file without pawns of its own is a way in for a heavy piece. Which files hold
 * whose pawns comes from PawnInfo, cached by the pawns.
 *
 * The trace sees every term, the tuner fits the tables on them, the evaluation passes NoPieceTrace.    */

#pragma once

#include <bitboard/bitboard_constants.hpp>
#include <eval/pawn_structure.hpp>
#include <eval/piece_tables.hpp>
#include <eval/tapered_score.hpp>
#include <material/chess_piece_defines.hpp>
#include <material/material_mask.hpp>

// What pieceScore reports to a trace: a piece pair by its piece index (knight 1 to rook 3), and a rook or queen on a
// file by its piece index (rook 3, queen 4) and 0 half-open or 1 open; each with its sign, +1 white & -1 black. The
// score is the sum of sign * the term's value.
struct NoPieceTrace {
    constexpr void piecePair(u32, i32) {}
    constexpr void pieceOnFile(u32, u32, i32) {}
};

namespace piece_internals {

// one side's pieces, scored from its point of view.
template<Set us, typename Trace>
TaperedScore sideScore(const MaterialPositionMask& material, const PawnInfo& pawns, Trace& trace) {
    constexpr Set them = us == Set::WHITE ? Set::BLACK : Set::WHITE;
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

    const u8 ourFiles = pawns.filesOf(us);
    const u8 theirFiles = pawns.filesOf(them);
    auto onFile = [&](u32 pieceId, const TaperedScore (&values)[2], u32 sqr) {
        const u32 file = 1u << (sqr & 7);
        if ((ourFiles & file) != 0)
            return;
        const u32 open = (theirFiles & file) == 0 ? 1 : 0;
        score += values[open];
        trace.pieceOnFile(pieceId, open, sign);
    };
    for (auto rooks = material.rooks<us>(); !rooks.empty();)
        onFile(rookId, evaluator_data::rookOnFile, rooks.popLsb());
    for (auto queens = material.queens<us>(); !queens.empty();)
        onFile(queenId, evaluator_data::queenOnFile, queens.popLsb());
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
