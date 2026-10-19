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
 * @file king_safety.hpp
 * @brief King shelter & pawn storm: the pawns on the king's file and the files beside it, scored from
 * king_tables.hpp. The king's piece square table says where it stands well; these say how well its pawns cover it.
 *
 * For each king, on three files: the king's own file and the two beside it, moved in from the edge so a king on
 * the a or h file still looks at three (a king on a looks at a, b & c). Only pawns on the king's rank or in front of
 * it count, a pawn behind the king covers nothing. On each file:
 *  - shelter: the king's own pawn nearest it, by its relative rank, or no pawn, a file open in front of the king.
 *  - storm: the enemy pawn nearest the king, by how far it has come, its relative rank seen from the king's side, or
 *    no pawn. Blocked when the king's own pawn stands right in front of it: it can't advance to open the file, it's
 *    scored from its own table, by rank alone.
 * Shelter & storm are by file folded onto a to d, an edge file matters differently from a centre one.
 *
 * King attack: how hard the enemy's pieces bear on the king zone, the king's square and the squares around it and
 * those once more a rank further towards the enemy (3 files by up to 4 ranks). Every enemy knight, bishop, rook &
 * queen reaching the zone adds its attack units for each zone square it reaches; when the enemy has a rook or queen,
 * every open file of the king's three adds more, and every file without the king's pawns a little less. The units
 * count only once two pieces attack, one alone is no attack. The danger is kingDanger at the units, the table is fitted,
 * the units are hand-set, which keeps the term linear for the tuner.
 *
 * Counted every evaluation, the king moves more often than the pawns: a few bit scans per file, the enemy pieces'
 * attacks once more. The trace sees every term, the tuner fits the tables on them, the evaluation passes NoKingTrace.  */

#pragma once

#include <algorithm>

#include <bitboard/attacks/attacks.hpp>
#include <bitboard/intrinsics.hpp>
#include <eval/king_tables.hpp>
#include <eval/pawn_structure.hpp>
#include <eval/tapered_score.hpp>
#include <material/chess_piece_defines.hpp>
#include <material/material_mask.hpp>

// What kingSafetyScore reports to a trace for every file: the folded file (0 to 3) and the relative rank index (0 no
// pawn, 1 to 6 for rank 2 to 7) of shelter and storm, or the rank alone for a blocked storm, and its sign, +1 white &
// -1 black. The score is the sum of sign * the term's table at that index. kingAttackScore reports each king's attack
// units, 1 to 63, when they're not 0.
struct NoKingTrace {
    constexpr void kingShelter(u32, u32, i32) {}
    constexpr void kingStorm(u32, u32, i32) {}
    constexpr void kingStormBlocked(u32, i32) {}
    constexpr void kingDanger(u32, i32) {}
};

namespace king_internals {

// the relative rank, from us, of the pawn in pawns nearest our back rank, 0 when there's none. pawns are on one file.
template<Set us>
constexpr u32 nearestRelRank(u64 pawns) {
    if (pawns == 0)
        return 0;
    const u32 sqr = us == Set::WHITE ? intrinsics::lsbIndex(pawns) : 63 - intrinsics::msbIndex(pawns);
    return sqr >> 3;
}

// one side's king, scored from its point of view.
template<Set us, typename Trace>
TaperedScore sideScore(const MaterialPositionMask& material, Trace& trace) {
    constexpr Set them = us == Set::WHITE ? Set::BLACK : Set::WHITE;
    constexpr i32 sign = us == Set::WHITE ? 1 : -1;

    const u64 king = material.king<us>().read();
    if (king == 0)
        return S(0, 0);
    const u32 kingSqr = intrinsics::lsbIndex(king);
    const u32 kingRank = kingSqr >> 3;
    // the king's rank and every rank in front of it.
    const u64 front = pawn_internals::ranksAhead<us>(kingRank) | pawn_internals::rankMask(kingRank);
    const u64 ours = material.pawns<us>().read() & front;
    const u64 theirs = material.pawns<them>().read() & front;

    TaperedScore score = S(0, 0);
    const u32 centre = std::clamp<u32>(kingSqr & 7, 1, 6);
    for (u32 file = centre - 1; file <= centre + 1; ++file) {
        const u64 mask = pawn_internals::fileMask(file);
        const u32 folded = pawn_internals::foldedFile(file);
        const u32 ourRank = nearestRelRank<us>(ours & mask);
        const u32 theirRank = nearestRelRank<us>(theirs & mask);

        score += evaluator_data::kingShelter[folded * 8 + ourRank];
        trace.kingShelter(folded, ourRank, sign);

        if (ourRank != 0 && theirRank == ourRank + 1) {
            score += evaluator_data::kingStormBlocked[theirRank];
            trace.kingStormBlocked(theirRank, sign);
        }
        else {
            score += evaluator_data::kingStorm[folded * 8 + theirRank];
            trace.kingStorm(folded, theirRank, sign);
        }
    }
    return score;
}

// the king zone of a king on kingSqr: its square & the squares around it, and those a rank further towards the enemy.
template<Set us>
constexpr u64 kingZone(u32 kingSqr) {
    const u64 around = attacks::getKingAttacks(kingSqr) | (UINT64_C(1) << kingSqr);
    return around | (us == Set::WHITE ? around << 8 : around >> 8);
}

// the enemy's attack units against our king, 0 to 63, see the top of this file.
template<Set us>
u32 attackUnits(const MaterialPositionMask& material, u32 kingSqr) {
    constexpr Set them = us == Set::WHITE ? Set::BLACK : Set::WHITE;

    const u64 zone = kingZone<us>(kingSqr);
    const u64 occupied = material.combine().read();
    u32 attackers = 0;
    u32 units = 0;
    auto add = [&](u32 pieceId, u64 reach) {
        const u64 hits = reach & zone;
        if (hits == 0)
            return;
        ++attackers;
        units += evaluator_data::kingAttackUnits[pieceId] * static_cast<u32>(intrinsics::popcnt(hits));
    };

    for (auto pieces = material.knights<them>(); !pieces.empty();)
        add(knightId, attacks::getKnightAttacks(pieces.popLsb()));
    for (auto pieces = material.bishops<them>(); !pieces.empty();)
        add(bishopId, attacks::getBishopAttacks(pieces.popLsb(), occupied));
    for (auto pieces = material.rooks<them>(); !pieces.empty();)
        add(rookId, attacks::getRookAttacks(pieces.popLsb(), occupied));
    for (auto pieces = material.queens<them>(); !pieces.empty();) {
        const u32 sqr = pieces.popLsb();
        add(queenId, attacks::getRookAttacks(sqr, occupied) | attacks::getBishopAttacks(sqr, occupied));
    }
    if (attackers < 2)
        return 0;

    // the king's three files, open to the enemy's rooks & queens.
    if ((material.rooks<them>().read() | material.queens<them>().read()) != 0) {
        const u64 ours = material.pawns<us>().read();
        const u64 theirs = material.pawns<them>().read();
        const u32 centre = std::clamp<u32>(kingSqr & 7, 1, 6);
        for (u32 file = centre - 1; file <= centre + 1; ++file) {
            const u64 mask = pawn_internals::fileMask(file);
            if ((ours & mask) != 0)
                continue;
            units += (theirs & mask) == 0 ? evaluator_data::kingOpenFileUnits : evaluator_data::kingHalfOpenFileUnits;
        }
    }
    return std::min<u32>(units, 63);
}

// one side's king attacked, scored from its point of view.
template<Set us, typename Trace>
TaperedScore attackScore(const MaterialPositionMask& material, Trace& trace) {
    constexpr i32 sign = us == Set::WHITE ? 1 : -1;

    const u64 king = material.king<us>().read();
    if (king == 0)
        return S(0, 0);
    const u32 units = attackUnits<us>(material, intrinsics::lsbIndex(king));
    if (units == 0)
        return S(0, 0);
    trace.kingDanger(units, sign);
    return evaluator_data::kingDanger[units];
}

} // namespace king_internals

/**
 * King shelter & pawn storm of both sides, white's minus black's, midgame & endgame packed.    */
template<typename Trace>
[[nodiscard]] TaperedScore kingSafetyScore(const MaterialPositionMask& material, Trace& trace) {
    return king_internals::sideScore<Set::WHITE>(material, trace)
         - king_internals::sideScore<Set::BLACK>(material, trace);
}

[[nodiscard]] inline TaperedScore kingSafetyScore(const MaterialPositionMask& material) {
    NoKingTrace none;
    return kingSafetyScore(material, none);
}

/**
 * King attack of both sides, white's minus black's, midgame & endgame packed.    */
template<typename Trace>
[[nodiscard]] TaperedScore kingAttackScore(const MaterialPositionMask& material, Trace& trace) {
    return king_internals::attackScore<Set::WHITE>(material, trace)
         - king_internals::attackScore<Set::BLACK>(material, trace);
}

[[nodiscard]] inline TaperedScore kingAttackScore(const MaterialPositionMask& material) {
    NoKingTrace none;
    return kingAttackScore(material, none);
}
