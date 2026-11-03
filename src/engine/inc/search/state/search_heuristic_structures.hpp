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
#pragma once
#include <system/platform.hpp>
#include <move/move.hpp>
#include <move/generation/move_ordering_view.hpp>
#include <search/core/search_constants.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <span>
#include <vector>

#include <diagnostics/assert.hpp>

// History heuristic: quiet moves that caused beta cutoffs, indexed [side][from][to] the way MoveOrderingView reads it.
struct HistoryTable {
    std::array<i32, 2 * 64 * 64> table{};

    void clear() {
        table.fill(0);
    }

    const i32* data() const { return table.data(); }

    // Gravity update, the bonus shrinks as the entry nears the bound so scores stay within [-max, max] and recent
    // cutoffs outweigh old ones. A negative bonus is a malus.
    void update(Set side, PackedMove move, i32 bonus) {
        i32& entry = table[static_cast<u32>(side) * 64 * 64 + static_cast<u32>(move.sourceSqr()) * 64 + static_cast<u32>(move.targetSqr())];
        entry += bonus - entry * std::abs(bonus) / MoveOrderingView::historyMax;
    }
};

// i16 entries: 12*64*12*64*2 ≈ 1.2 MB
using PieceToHistory = std::array<std::array<i16, 64>, 12>;          // [piece][to]
using ContinuationHistory = std::array<std::array<PieceToHistory, 64>, 12>; // [prevPiece][prevTo]

static_assert(sizeof(PieceToHistory) == 12 * 64 * sizeof(i16));
namespace heuristic_defaults {    
// Lookbacks past the root or across a null move point here. Read only, never updated.
inline PieceToHistory noPreviousMove{};
}

struct ThreadHeuristics {
    HistoryTable butterfly{};    
    ContinuationHistory continuationHistory{};

    void reset() {
        butterfly.clear();
        std::memset(&continuationHistory, 0, sizeof(continuationHistory));
    }
};
