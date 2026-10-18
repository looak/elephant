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
 * @file pawn_structure.hpp
 * @brief Pawn structure: passed, isolated, doubled, backward and connected pawns, scored per pawn from
 * pawn_tables.hpp. The piece square tables say where a pawn stands well; these say how it stands with the other pawns.
 *
 * Each pawn, from its own side's point of view:
 *  - doubled: one of its own pawns stands in front of it on its file. Three on a file count two.
 *  - passed: no enemy pawn in front of it on its own or the adjacent files, and none of its own in front of it either,
 *    the rear pawn of a doubled pair isn't passed.
 *  - isolated: none of its own pawns on the adjacent files.
 *  - backward: not isolated, but none of its own pawns on the adjacent files beside or behind it to support its
 *    advance, and an enemy pawn attacks the square in front of it.
 *  - connected: one of its own pawns beside it on an adjacent file, or defending it.
 * A pawn can be several of these, connected and passed, doubled and isolated. Isolated and backward exclude each other.
 *
 * The trace sees every pawn's terms, the tuner fits the tables on them, the evaluation passes NoPawnTrace.    */

#pragma once

#include <bitboard/attacks/attacks.hpp>
#include <bitboard/bitboard_constants.hpp>
#include <bitboard/intrinsics.hpp>
#include <eval/pawn_tables.hpp>
#include <eval/tapered_score.hpp>
#include <material/chess_piece_defines.hpp>

// What pawnScore reports to a trace for every pawn term: the relative rank (rank 2 = 1) for passed & connected, the
// file (a = 0, not folded) for isolated, doubled & backward, and its sign, +1 white & -1 black. The score is the sum of
// sign * the term's table at that rank or folded file.
struct NoPawnTrace {
    constexpr void passed(u32, i32) {}
    constexpr void isolated(u32, i32) {}
    constexpr void doubled(u32, i32) {}
    constexpr void backward(u32, i32) {}
    constexpr void connected(u32, i32) {}
};

namespace pawn_internals {

constexpr u64 fileMask(u32 file) { return board_constants::fileaMask << file; }
constexpr u64 rankMask(u32 rank) { return board_constants::rank0Mask << (8 * rank); }

constexpr u64 adjacentFiles(u32 file) {
    const u64 mask = fileMask(file);
    return ((mask & ~board_constants::fileaMask) >> 1) | ((mask & ~board_constants::filehMask) << 1);
}

// a & h share a value, b & g, c & f, d & e.
constexpr u32 foldedFile(u32 file) { return file < 4 ? file : 7 - file; }

// the ranks in front of rank, seen from us.
template<Set us>
constexpr u64 ranksAhead(u32 rank) {
    if constexpr (us == Set::WHITE)
        return rank >= 7 ? 0 : ~UINT64_C(0) << (8 * (rank + 1));
    else
        return rank == 0 ? 0 : ~UINT64_C(0) >> (8 * (8 - rank));
}

// one side's pawns, scored from its point of view.
template<Set us, typename Material, typename Trace>
TaperedScore sideScore(const Material& material, Trace& trace) {
    constexpr Set them = us == Set::WHITE ? Set::BLACK : Set::WHITE;
    constexpr i32 sign = us == Set::WHITE ? 1 : -1;

    const u64 ours = material.template pawns<us>().read();
    const u64 theirs = material.template pawns<them>().read();
    const u64 ourAttacks = attacks::pawnAttacks<us>(ours);
    const u64 theirAttacks = attacks::pawnAttacks<them>(theirs);

    TaperedScore score = S(0, 0);
    for (u64 pawns = ours; pawns != 0; pawns &= pawns - 1) {
        const u32 sqr = intrinsics::lsbIndex(pawns);
        const u32 file = sqr & 7;
        const u32 rank = sqr >> 3;
        const u32 relRank = us == Set::WHITE ? rank : 7 - rank;
        const u64 square = UINT64_C(1) << sqr;
        const u64 stop = us == Set::WHITE ? square << 8 : square >> 8;
        const u64 ahead = ranksAhead<us>(rank);
        const u64 beside = adjacentFiles(file);

        if ((ours & fileMask(file) & ahead) != 0) {
            score += evaluator_data::doubledPawn[foldedFile(file)];
            trace.doubled(file, sign);
        }
        else if ((theirs & (fileMask(file) | beside) & ahead) == 0) {
            score += evaluator_data::passedPawn[relRank - 1];
            trace.passed(relRank, sign);
        }

        if ((ours & beside) == 0) {
            score += evaluator_data::isolatedPawn[foldedFile(file)];
            trace.isolated(file, sign);
        }
        else if ((ours & beside & ~ahead) == 0 && (theirAttacks & stop) != 0) {
            score += evaluator_data::backwardPawn[foldedFile(file)];
            trace.backward(file, sign);
        }

        if ((ours & beside & rankMask(rank)) != 0 || (ourAttacks & square) != 0) {
            score += evaluator_data::connectedPawn[relRank - 1];
            trace.connected(relRank, sign);
        }
    }
    return score;
}

} // namespace pawn_internals

/**
 * Pawn structure of both sides, white's minus black's, midgame & endgame packed. Material is a MaterialPositionMask,
 * templated so this header doesn't need it.    */
template<typename Material, typename Trace>
[[nodiscard]] TaperedScore pawnScore(const Material& material, Trace& trace) {
    return pawn_internals::sideScore<Set::WHITE>(material, trace)
         - pawn_internals::sideScore<Set::BLACK>(material, trace);
}

template<typename Material>
[[nodiscard]] TaperedScore pawnScore(const Material& material) {
    NoPawnTrace none;
    return pawnScore(material, none);
}
