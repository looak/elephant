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
 * @file mobility.hpp
 * @brief Mobility: how many squares a knight, bishop, rook or queen can move to, scored per piece from
 * mobility_tables.hpp. The piece square tables say where a piece stands well; mobility says how free it is there,
 * blocked by its own pawns or not, on an open file or not.
 *
 * Safe squares only: a square the piece attacks that holds none of its own pieces and isn't attacked by an enemy pawn,
 * a pawn would chase it off or win it there. Sliders see through nothing, the board's pieces block them.
 *
 * The same pass counts what the pieces do to the enemy king zone for king attack (king_safety.hpp), so each piece's
 * attacks are looked up once: KingZoneAttacks, the pieces reaching the zone and their attack units.
 *
 * The trace sees every piece's count, the tuner fits the tables on them, the evaluation passes NoMobilityTrace.    */

#pragma once

#include <bitboard/attacks/attacks.hpp>
#include <bitboard/intrinsics.hpp>
#include <eval/king_tables.hpp>
#include <eval/mobility_tables.hpp>
#include <eval/tapered_score.hpp>
#include <material/chess_piece_defines.hpp>
#include <material/material_mask.hpp>

// What mobilityScore reports to a trace for every knight, bishop, rook & queen: its piece index (knight 1 to queen 4),
// its safe squares, and its sign, +1 white & -1 black. The score is the sum of sign * mobilityTables[pieceId][count].
struct NoMobilityTrace {
    constexpr void mobility(u32, u32, i32) {}
};

// What one side's knights, bishops, rooks & queens do to the other side's king zone: how many of them reach it and
// their attack units, evaluator_data::kingAttackUnits per zone square reached. King attack scores them.
struct KingZoneAttacks {
    u32 attackers = 0;
    u32 units = 0;
};

// the king zone of a king on kingSqr: its square & the squares around it, and those a rank further towards the enemy.
template<Set us>
constexpr u64 kingZone(u32 kingSqr) {
    const u64 around = attacks::getKingAttacks(kingSqr) | (UINT64_C(1) << kingSqr);
    return around | (us == Set::WHITE ? around << 8 : around >> 8);
}

namespace mobility_internals {

// one side's pieces, scored from its point of view, and what they do to the enemy king zone left in onEnemyKing.
template<Set us, typename Trace>
TaperedScore sideScore(const MaterialPositionMask& material, Trace& trace, KingZoneAttacks& onEnemyKing) {
    constexpr Set them = us == Set::WHITE ? Set::BLACK : Set::WHITE;
    constexpr i32 sign = us == Set::WHITE ? 1 : -1;

    const u64 occupied = material.combine().read();
    const u64 enemyPawnAttacks = attacks::pawnAttacks<them>(material.pawns<them>().read());
    const u64 safe = ~material.combine<us>().read() & ~enemyPawnAttacks;
    const u64 enemyKing = material.king<them>().read();
    const u64 zone = enemyKing != 0 ? kingZone<them>(intrinsics::lsbIndex(enemyKing)) : 0;

    TaperedScore score = S(0, 0);
    onEnemyKing = {};
    auto add = [&](u32 pieceId, u64 reach) {
        const u32 count = static_cast<u32>(intrinsics::popcnt(reach & safe));
        score += evaluator_data::mobilityTables[pieceId][count];
        trace.mobility(pieceId, count, sign);

        const u64 hits = reach & zone;
        if (hits != 0) {
            ++onEnemyKing.attackers;
            onEnemyKing.units += evaluator_data::kingAttackUnits[pieceId] * static_cast<u32>(intrinsics::popcnt(hits));
        }
    };

    for (auto pieces = material.knights<us>(); !pieces.empty();)
        add(knightId, attacks::getKnightAttacks(pieces.popLsb()));
    for (auto pieces = material.bishops<us>(); !pieces.empty();)
        add(bishopId, attacks::getBishopAttacks(pieces.popLsb(), occupied));
    for (auto pieces = material.rooks<us>(); !pieces.empty();)
        add(rookId, attacks::getRookAttacks(pieces.popLsb(), occupied));
    for (auto pieces = material.queens<us>(); !pieces.empty();) {
        const u32 sqr = pieces.popLsb();
        add(queenId, attacks::getRookAttacks(sqr, occupied) | attacks::getBishopAttacks(sqr, occupied));
    }
    return score;
}

} // namespace mobility_internals

/**
 * Mobility of both sides, white's minus black's, midgame & endgame packed. onKing gets what each side's king zone
 * takes, by Set: onKing[0] white's king attacked by black's pieces, onKing[1] black's.    */
template<typename Trace>
[[nodiscard]] TaperedScore mobilityScore(const MaterialPositionMask& material, Trace& trace,
                                         KingZoneAttacks (&onKing)[2]) {
    return mobility_internals::sideScore<Set::WHITE>(material, trace, onKing[1])
         - mobility_internals::sideScore<Set::BLACK>(material, trace, onKing[0]);
}

template<typename Trace>
[[nodiscard]] TaperedScore mobilityScore(const MaterialPositionMask& material, Trace& trace) {
    KingZoneAttacks onKing[2];
    return mobilityScore(material, trace, onKing);
}

[[nodiscard]] inline TaperedScore mobilityScore(const MaterialPositionMask& material, KingZoneAttacks (&onKing)[2]) {
    NoMobilityTrace none;
    return mobilityScore(material, none, onKing);
}

[[nodiscard]] inline TaperedScore mobilityScore(const MaterialPositionMask& material) {
    NoMobilityTrace none;
    return mobilityScore(material, none);
}
