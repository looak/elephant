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
#include <search/search_constants.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <vector>

#include <diagnostics/assert.hpp>

struct MoveHistory {
private:
    std::vector<u64> recentHashes;

public:
    MoveHistory() {
        // rarely will we have a game that goes this deep.
        recentHashes.reserve(128);
    }

    void push(u64 hash) {
        recentHashes.push_back(hash);
    }
    void pop() {
        if (!recentHashes.empty()) {
            recentHashes.pop_back();
        }
    }

    // Threefold repetition of the last pushed position, hashKey. A capture or pawn move can't be undone, so only the
    // positions since the last one (halfmoveClock half moves back) can repeat it, and of those only the ones with the
    // same side to move, every second one.
    bool isRepetition(u64 hashKey, u16 halfmoveClock) const {
        ASSERT_MSG(!recentHashes.empty() && recentHashes.back() == hashKey, "the position checked has to be the last pushed.");
        const size_t last = recentHashes.size() - 1;
        const size_t window = std::min<size_t>(halfmoveClock, last);

        int occurrences = 1;  // the position itself
        for (size_t back = 2; back <= window; back += 2) {
            if (recentHashes[last - back] == hashKey && ++occurrences >= 3)
                return true;
        }
        return false;
    }
};

struct KillerMoves {
    private:
    PackedMove m_killers[c_maxSearchDepth][2];

    public:
    KillerMoves() {
        clear();
    }
    void clear() {
        for (u32 i = 0; i < c_maxSearchDepth; ++i) {
            m_killers[i][0] = PackedMove::NullMove();
            m_killers[i][1] = PackedMove::NullMove();
        }
    }
    void push(PackedMove move, u16 ply) {
        if (m_killers[ply][0] != move) {
            m_killers[ply][1] = m_killers[ply][0];
            m_killers[ply][0] = move;
        }
    }

    void retrieve(u16 ply, PackedMove& outFirst, PackedMove& outSecond) const {
        outFirst = m_killers[ply][0];
        outSecond = m_killers[ply][1];
    }
};

// History heuristic: quiet moves that caused beta cutoffs, indexed [side][from][to] the way MoveOrderingView reads it.
struct HistoryTable {
    std::array<i32, 2 * 64 * 64> table{};

    const i32* data() const { return table.data(); }

    // Gravity update, the bonus shrinks as the entry nears the bound so scores stay within [-max, max] and recent
    // cutoffs outweigh old ones. A negative bonus is a malus.
    void update(Set side, PackedMove move, i32 bonus) {
        i32& entry = table[static_cast<u32>(side) * 64 * 64 + static_cast<u32>(move.sourceSqr()) * 64 + static_cast<u32>(move.targetSqr())];
        entry += bonus - entry * std::abs(bonus) / MoveOrderingView::historyMax;
    }
};

struct MoveOrderingHeuristic {
    KillerMoves killers;
    HistoryTable history;
};
