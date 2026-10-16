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
 * @file tuner_data.hpp
 * @brief Phase 0 of tuning, preparing the data: every position of a dataset resolved to a quiet one, traced, and kept
 * as a compact entry. The FENs are dropped, the later phases (K, then the fit) work from the entries alone.    */

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <dataset.hpp>
#include <eval/tapered_score.hpp>
#include <system/platform.hpp>
#include <trace.hpp>

namespace tuner {

struct TunerEntry {
    float result;           // white's side, 1.0 win, 0.5 draw, 0.0 loss
    i16 staticEval;         // the engine's static evaluation of the quiet position, white's side
    i16 fixedOffset;        // staticEval minus the fitted terms' score, what no fitted value explains (mop-up)
    u32 coefficientStart;   // into TunerData::coefficients
    u8 coefficientCount;
    u8 phase;               // capped at maxGamePhase
};

struct TunerData {
    std::vector<TunerEntry> entries;
    std::vector<Coefficient> coefficients;

    // the fitted terms' score of an entry with these values, white's side: sum of coefficient * value, tapered.
    [[nodiscard]] i32 fittedScore(const TunerEntry& entry, const std::vector<TaperedScore>& values) const {
        TaperedScore sum = S(0, 0);
        for (u32 i = 0; i < entry.coefficientCount; ++i) {
            const Coefficient c = coefficients[entry.coefficientStart + i];
            sum += values[c.index()] * c.value();
        }
        return evaluator_data::taper(sum, entry.phase);
    }

    [[nodiscard]] std::size_t bytes() const {
        return entries.capacity() * sizeof(TunerEntry) + coefficients.capacity() * sizeof(Coefficient);
    }
};

struct PrepareReport {
    std::size_t positions = 0;      // read with a valid FEN
    std::size_t decided = 0;        // quiescence found a mate, dropped
    std::size_t resolved = 0;       // quiescence played moves to reach a quiet position
    std::size_t lineMoves = 0;      // moves played over all resolved positions
    std::size_t longestLine = 0;
    std::size_t withOffset = 0;     // entries whose static evaluation has unfitted terms (mop-up)
    std::size_t accumulatorMismatch = 0;  // incremental PeSTO sums after the line differ from the traced ones
    std::size_t rebuildMismatch = 0;      // coefficients * current values differ from the engine's PeSTO score
    double seconds = 0.0;
};

/**
 * Phase 0: streams the dataset at path (limit positions, 0 for all), resolves each to a quiet position with the
 * engine's quiescence search, traces its PeSTO terms and keeps an entry. Prints progress to stderr.    */
TunerData prepare(const std::string& path, std::size_t limit, LoadReport& load, PrepareReport& report);

} // namespace tuner
