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
 * @file passed_pawns.hpp
 * @brief Passed pawn extras: what a passed pawn is worth beyond its rank (pawn_structure.hpp), from the pieces around
 * it, scored per passed pawn from pawn_tables.hpp. Which pawns are passed comes from PawnInfo, cached by the pawns;
 * these depend on the pieces and are counted every evaluation.
 *
 * Each passed pawn, from its own side's point of view:
 *  - own king & enemy king: how far each king is from the pawn, in king moves. An escort near, the enemy far.
 *  - blocked: a piece, either side's, stands on the square in front of it.
 *  - free path: nothing stands in front of it, all the way to its promotion square.
 *  - rook behind: a rook on its file behind it with nothing between, its own side's (pushes it) or the enemy's (holds
 *    it back from behind).
 *
 * The trace sees every passed pawn's extras, the tuner fits the tables on them, the evaluation passes NoPasserTrace.  */

#pragma once

#include <algorithm>

#include <bitboard/attacks/attacks.hpp>
#include <bitboard/bitboard_constants.hpp>
#include <bitboard/intrinsics.hpp>
#include <eval/pawn_structure.hpp>
#include <eval/pawn_tables.hpp>
#include <eval/tapered_score.hpp>
#include <material/chess_piece_defines.hpp>
#include <material/material_mask.hpp>

// What passerScore reports to a trace for every passed pawn: the kings' distances to it (0 to 7), its relative rank
// (rank 2 = 1) when blocked or with a free path, which rook is behind it (0 its own, 1 the enemy's), and its sign, +1
// white & -1 black. The score is the sum of sign * the term's table at that index.
struct NoPasserTrace {
    constexpr void passedOwnKing(u32, i32) {}
    constexpr void passedEnemyKing(u32, i32) {}
    constexpr void passedBlocked(u32, i32) {}
    constexpr void passedFreePath(u32, i32) {}
    constexpr void passedRookBehind(u32, i32) {}
};

namespace passer_internals {

// king moves between two squares.
constexpr u32 distance(u32 a, u32 b) {
    const i32 files = static_cast<i32>(a & 7) - static_cast<i32>(b & 7);
    const i32 ranks = static_cast<i32>(a >> 3) - static_cast<i32>(b >> 3);
    return static_cast<u32>(std::max(files < 0 ? -files : files, ranks < 0 ? -ranks : ranks));
}

// one side's passed pawns, scored from its point of view.
template<Set us, typename Trace>
TaperedScore sideScore(const MaterialPositionMask& material, u64 passed, Trace& trace) {
    constexpr Set them = us == Set::WHITE ? Set::BLACK : Set::WHITE;
    constexpr i32 sign = us == Set::WHITE ? 1 : -1;

    const u64 occupied = material.combine().read();
    const u64 ownKing = material.king<us>().read();
    const u64 enemyKing = material.king<them>().read();
    const u64 ownRooks = material.rooks<us>().read();
    const u64 enemyRooks = material.rooks<them>().read();

    TaperedScore score = S(0, 0);
    for (; passed != 0; passed &= passed - 1) {
        const u32 sqr = intrinsics::lsbIndex(passed);
        const u32 file = sqr & 7;
        const u32 rank = sqr >> 3;
        const u32 relRank = us == Set::WHITE ? rank : 7 - rank;
        const u64 square = UINT64_C(1) << sqr;
        const u64 stop = us == Set::WHITE ? square << 8 : square >> 8;
        const u64 path = board_constants::fileMasks[file] & board_constants::ranksAhead<us>(rank);
        const u64 behind = board_constants::fileMasks[file] & ~path & ~square;

        if (ownKing != 0) {
            const u32 d = distance(sqr, intrinsics::lsbIndex(ownKing));
            score += evaluator_data::passedOwnKing[d];
            trace.passedOwnKing(d, sign);
        }
        if (enemyKing != 0) {
            const u32 d = distance(sqr, intrinsics::lsbIndex(enemyKing));
            score += evaluator_data::passedEnemyKing[d];
            trace.passedEnemyKing(d, sign);
        }

        if ((occupied & stop) != 0) {
            score += evaluator_data::passedBlocked[relRank - 1];
            trace.passedBlocked(relRank, sign);
        }
        else if ((occupied & path) == 0) {
            score += evaluator_data::passedFreePath[relRank - 1];
            trace.passedFreePath(relRank, sign);
        }

        // the first piece behind the pawn on its file, a rook of either side or neither.
        const u64 firstBehind = attacks::getRookAttacks(sqr, occupied) & behind;
        if ((firstBehind & ownRooks) != 0) {
            score += evaluator_data::passedRookBehind[0];
            trace.passedRookBehind(0, sign);
        }
        else if ((firstBehind & enemyRooks) != 0) {
            score += evaluator_data::passedRookBehind[1];
            trace.passedRookBehind(1, sign);
        }
    }
    return score;
}

} // namespace passer_internals

/**
 * Passed pawn extras of both sides, white's minus black's, midgame & endgame packed. pawns is the position's
 * pawnInfo, for its passed pawns.    */
template<typename Trace>
[[nodiscard]] TaperedScore passerScore(const MaterialPositionMask& material, const PawnInfo& pawns, Trace& trace) {
    return passer_internals::sideScore<Set::WHITE>(material, pawns.passedOf(Set::WHITE), trace)
         - passer_internals::sideScore<Set::BLACK>(material, pawns.passedOf(Set::BLACK), trace);
}

[[nodiscard]] inline TaperedScore passerScore(const MaterialPositionMask& material, const PawnInfo& pawns) {
    NoPasserTrace none;
    return passerScore(material, pawns, none);
}
