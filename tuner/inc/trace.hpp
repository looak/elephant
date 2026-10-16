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
 * Fitted today: PeSTO's material and piece square tables, as the engine's PestoAccumulator adds them up.    */

#pragma once

#include <array>
#include <stdexcept>
#include <vector>

#include <eval/pesto_tables.hpp>
#include <eval/tapered_score.hpp>
#include <system/platform.hpp>

namespace tuner {

namespace params {

constexpr u32 materialOffset = 0;  // 6, by piece index pawn to king
constexpr u32 psqtOffset = 6;      // 6 * 64, by piece index then table square (A8 = 0, see pesto_tables.hpp)
constexpr u32 count = psqtOffset + 6 * 64;

constexpr u32 material(u32 pieceId) { return materialOffset + pieceId; }
constexpr u32 psqt(u32 pieceId, u32 tableSqr) { return psqtOffset + pieceId * 64 + tableSqr; }

// the engine's current values in this layout, what tuning starts from.
inline std::vector<TaperedScore> current() {
    std::vector<TaperedScore> values(count, S(0, 0));
    for (u32 pieceId = 0; pieceId < 6; ++pieceId) {
        values[material(pieceId)] = evaluator_data::pestoMaterial[pieceId];
        for (u32 sqr = 0; sqr < 64; ++sqr)
            values[psqt(pieceId, sqr)] = evaluator_data::pestoTables[pieceId][sqr];
    }
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
 * Records one position's coefficients, handed to PestoAccumulator::computeFromScratch as its trace. clear() before
 * each position, coefficients() after.    */
class PestoTrace {
public:
    // see NoPestoTrace in pesto_accumulator.hpp.
    void piece(u32 pieceId, u32 tableSqr, i32 sign) {
        add(params::material(pieceId), sign);
        add(params::psqt(pieceId, tableSqr), sign);
    }

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
