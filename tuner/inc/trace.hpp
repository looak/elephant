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
 * @file trace.hpp
 * @brief The values the tuner fits, each a TaperedScore at a fixed index, and the trace that records how often a
 * position uses each of them: white's uses minus black's. The evaluation of a position is then the sum of
 * coefficient * value over its coefficients, tapered on its phase.
 *
 * Fitted: PeSTO's material and piece square tables, as the engine's PestoAccumulator adds them up, and mobility, as
 * mobilityScore counts it. Each is a group the fit can tune or leave as it is.    */

#pragma once

#include <array>
#include <stdexcept>
#include <vector>

#include <eval/mobility_tables.hpp>
#include <eval/pawn_tables.hpp>
#include <eval/pesto_tables.hpp>
#include <eval/tapered_score.hpp>
#include <system/platform.hpp>

namespace tuner {
namespace params {

// the groups of values, in the order they're laid out.
enum class Group : u8 { Material, Psqt, Mobility, Pawns_Passed, Pawns_Structure, Pawns_PassedExtras, Count };
struct GroupInfo { Group group; std::string_view name; };
inline constexpr std::array<GroupInfo, static_cast<size_t>(Group::Count)> groupInfos = {{
    { Group::Material, "material" },
    { Group::Psqt, "psqt" },
    { Group::Mobility, "mobility" },
    { Group::Pawns_Passed, "pawns_passed" },
    { Group::Pawns_Structure, "pawns_structure" },
    { Group::Pawns_PassedExtras, "pawns_passed_extras" }
}};

constexpr u32 bit(Group group) { return 1u << static_cast<u32>(group); }
static_assert(static_cast<u32>(Group::Count) <= 32, "FitSettings::tuned is a u32");

constexpr u32 materialOffset = 0;  // 6, by piece index pawn to king
constexpr u32 psqtOffset = 6;      // 6 * 64, by piece index then table square (A8 = 0, see pesto_tables.hpp)
constexpr u32 mobilityOffset = psqtOffset + 6 * 64;  // knight 0-8, bishop 0-13, rook 0-14, queen 0-27 safe squares

// how many counts each piece's mobility table has, by piece index, and where each starts within mobility's values.
constexpr u32 mobilityCounts[6] = { 0, 9, 14, 15, 28, 0 };
constexpr u32 mobilityStarts[6] = { 0, 0, 9, 23, 38, 66 };

// passed pawns, by relative rank 2-7. Structure: isolated, doubled and backward by file mirrored a-d, then
// connected by relative rank 2-7.
constexpr u32 pawnsPassedOffset = mobilityOffset + mobilityStarts[5];
constexpr u32 pawnsStructureOffset = pawnsPassedOffset + 6;
constexpr u32 isolatedOffset = pawnsStructureOffset;
constexpr u32 doubledOffset = isolatedOffset + 4;
constexpr u32 backwardOffset = doubledOffset + 4;
constexpr u32 connectedOffset = backwardOffset + 4;

// passed pawn extras: own & enemy king distance 0-7, blocked & free path by relative rank 2-7, rook behind own & enemy.
constexpr u32 pawnsPassedExtrasOffset = connectedOffset + 6;
constexpr u32 passedOwnKingOffset = pawnsPassedExtrasOffset;
constexpr u32 passedEnemyKingOffset = passedOwnKingOffset + 8;
constexpr u32 passedBlockedOffset = passedEnemyKingOffset + 8;
constexpr u32 passedFreePathOffset = passedBlockedOffset + 6;
constexpr u32 passedRookBehindOffset = passedFreePathOffset + 6;
constexpr u32 count = passedRookBehindOffset + 2;

// where each group's values start, in Group order, and count where the last one ends.
constexpr u32 groupCount = static_cast<u32>(Group::Count);
constexpr std::array<u32, groupCount + 1> groupStarts = {
    materialOffset, psqtOffset, mobilityOffset, pawnsPassedOffset, pawnsStructureOffset, pawnsPassedExtrasOffset, count
};

constexpr u32 material(u32 pieceId) { return materialOffset + pieceId; }
constexpr u32 psqt(u32 pieceId, u32 tableSqr) { return psqtOffset + pieceId * 64 + tableSqr; }
constexpr u32 mobility(u32 pieceId, u32 safeSquares) { return mobilityOffset + mobilityStarts[pieceId] + safeSquares; }

// relRank 0-7 from the pawn's own side (rank 2 = 1), file 0-7 (a = 0), folded so a and h share a value.
constexpr u32 mirroredFile(u32 file) { return file < 4 ? file : 7 - file; }
constexpr u32 passed(u32 relRank) { return pawnsPassedOffset + relRank - 1; }
constexpr u32 isolated(u32 file) { return isolatedOffset + mirroredFile(file); }
constexpr u32 doubled(u32 file) { return doubledOffset + mirroredFile(file); }
constexpr u32 backward(u32 file) { return backwardOffset + mirroredFile(file); }
constexpr u32 connected(u32 relRank) { return connectedOffset + relRank - 1; }
constexpr u32 passedOwnKing(u32 distance) { return passedOwnKingOffset + distance; }
constexpr u32 passedEnemyKing(u32 distance) { return passedEnemyKingOffset + distance; }
constexpr u32 passedBlocked(u32 relRank) { return passedBlockedOffset + relRank - 1; }
constexpr u32 passedFreePath(u32 relRank) { return passedFreePathOffset + relRank - 1; }
constexpr u32 passedRookBehind(u32 whose) { return passedRookBehindOffset + whose; }

constexpr Group groupOf(u32 index) {
    u32 group = 0;
    while (group + 1 < groupCount && index >= groupStarts[group + 1])
        ++group;
    return static_cast<Group>(group);
}

static_assert(groupOf(psqtOffset - 1) == Group::Material && groupOf(psqtOffset) == Group::Psqt);
static_assert(groupOf(mobilityOffset) == Group::Mobility && groupOf(pawnsPassedOffset - 1) == Group::Mobility);
static_assert(groupOf(passed(6)) == Group::Pawns_Passed && groupOf(isolated(0)) == Group::Pawns_Structure);
static_assert(groupOf(connected(6)) == Group::Pawns_Structure && groupOf(passedOwnKing(0)) == Group::Pawns_PassedExtras);
static_assert(groupOf(count - 1) == Group::Pawns_PassedExtras && passedRookBehind(1) == count - 1);

// the engine's current values in this layout, what tuning starts from.
inline std::vector<TaperedScore> current() {
    std::vector<TaperedScore> values(count, S(0, 0));
    for (u32 pieceId = 0; pieceId < 6; ++pieceId) {
        values[material(pieceId)] = evaluator_data::pestoMaterial[pieceId];
        for (u32 sqr = 0; sqr < 64; ++sqr)
            values[psqt(pieceId, sqr)] = evaluator_data::pestoTables[pieceId][sqr];
        for (u32 safe = 0; safe < mobilityCounts[pieceId]; ++safe)
            values[mobility(pieceId, safe)] = evaluator_data::mobilityTables[pieceId][safe];
    }
    for (u32 relRank = 1; relRank <= 6; ++relRank) {
        values[passed(relRank)] = evaluator_data::passedPawn[relRank - 1];
        values[connected(relRank)] = evaluator_data::connectedPawn[relRank - 1];
    }
    for (u32 file = 0; file < 4; ++file) {
        values[isolated(file)] = evaluator_data::isolatedPawn[file];
        values[doubled(file)] = evaluator_data::doubledPawn[file];
        values[backward(file)] = evaluator_data::backwardPawn[file];
    }
    for (u32 distance = 0; distance < 8; ++distance) {
        values[passedOwnKing(distance)] = evaluator_data::passedOwnKing[distance];
        values[passedEnemyKing(distance)] = evaluator_data::passedEnemyKing[distance];
    }
    for (u32 relRank = 1; relRank <= 6; ++relRank) {
        values[passedBlocked(relRank)] = evaluator_data::passedBlocked[relRank - 1];
        values[passedFreePath(relRank)] = evaluator_data::passedFreePath[relRank - 1];
    }
    for (u32 whose = 0; whose < 2; ++whose)
        values[passedRookBehind(whose)] = evaluator_data::passedRookBehind[whose];
    return values;
}

} // namespace params

/**
 * A parameter index and its coefficient in one position, packed into 16 bits: the index in the low 10 (up to 1024
 * parameters), the coefficient in the high 6 (-32 to 31). A table square is used at most once per side, material
 * at most by every piece of one kind, both well inside.    */
class Coefficient {
public:
    static constexpr u32 c_indexBits = 10;
    static constexpr i32 c_minValue = -32;
    static constexpr i32 c_maxValue = 31;
    static_assert(params::count <= (1u << c_indexBits), "Parameters don't fit a Coefficient's index any more.");

    Coefficient(u32 index, i32 value) {
        if (value < c_minValue || value > c_maxValue)
            throw std::out_of_range("Coefficient value out of range");
        m_packed = static_cast<u16>(index | (static_cast<u32>(value - c_minValue) << c_indexBits));
    }

    [[nodiscard]] u32 index() const { return m_packed & ((1u << c_indexBits) - 1); }
    [[nodiscard]] i32 value() const { return static_cast<i32>(m_packed >> c_indexBits) + c_minValue; }

private:
    u16 m_packed;
};

/**
 * Records one position's coefficients, handed as the trace to PestoAccumulator::computeFromScratch, mobilityScore,
 * pawnInfo and passerScore. clear() before each position, coefficients() after.    */
class EvalTrace {
public:
    // see NoPestoTrace in pesto_accumulator.hpp.
    void piece(u32 pieceId, u32 tableSqr, i32 sign) {
        add(params::material(pieceId), sign);
        add(params::psqt(pieceId, tableSqr), sign);
    }

    // see NoMobilityTrace in mobility.hpp.
    void mobility(u32 pieceId, u32 safeSquares, i32 sign) {
        add(params::mobility(pieceId, safeSquares), sign);
    }

    // see NoPawnTrace in pawn_structure.hpp.
    void passed(u32 relRank, i32 sign) { add(params::passed(relRank), sign); }
    void isolated(u32 file, i32 sign) { add(params::isolated(file), sign); }
    void doubled(u32 file, i32 sign) { add(params::doubled(file), sign); }
    void backward(u32 file, i32 sign) { add(params::backward(file), sign); }
    void connected(u32 relRank, i32 sign) { add(params::connected(relRank), sign); }

    // see NoPasserTrace in passed_pawns.hpp.
    void passedOwnKing(u32 distance, i32 sign) { add(params::passedOwnKing(distance), sign); }
    void passedEnemyKing(u32 distance, i32 sign) { add(params::passedEnemyKing(distance), sign); }
    void passedBlocked(u32 relRank, i32 sign) { add(params::passedBlocked(relRank), sign); }
    void passedFreePath(u32 relRank, i32 sign) { add(params::passedFreePath(relRank), sign); }
    void passedRookBehind(u32 whose, i32 sign) { add(params::passedRookBehind(whose), sign); }

    void clear() {
        for (u32 index : m_touched) {
            m_dense[index] = 0;
            m_isTouched[index] = false;
        }
        m_touched.clear();
    }

    // every parameter used with a non-zero coefficient, a white & a black piece on mirrored squares cancel out.
    [[nodiscard]] std::vector<Coefficient> coefficients() const {
        std::vector<Coefficient> result;
        result.reserve(m_touched.size());
        for (u32 index : m_touched) {
            if (m_dense[index] != 0)
                result.emplace_back(index, m_dense[index]);
        }
        return result;
    }

private:
    void add(u32 index, i32 sign) {
        // listed once, even when a coefficient drops back to 0 and is added to again.
        if (!m_isTouched[index]) {
            m_isTouched[index] = true;
            m_touched.push_back(index);
        }
        m_dense[index] += sign;
    }

    std::array<i32, params::count> m_dense{};
    std::array<bool, params::count> m_isTouched{};
    std::vector<u32> m_touched;
};

} // namespace tuner
