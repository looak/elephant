/******************************************************************************
 * Elephant Gambit Chess Engine - a Chess AI
 * Copyright(C) 2025  Alexander Loodin Ek
 *
 * This program is free software : you can redistribute it and /or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.If not, see < http://www.gnu.org/licenses/>.
 *****************************************************************************/

#pragma once
#include <memory>
#include <vector>

#include <eval/pawn_hash.hpp>
#include <search/state/search_heuristic_structures.hpp>
#include <search/state/transposition_table.hpp>

// What search keeps beyond a single go, owned by UCI next to the GameContext. Everything here is one per thread and
// never shared between threads. The TT, shared by every thread, still lives in the GameContext.
struct SearchContext {
    // killers, butterfly & continuation history, indexed by thread id. Heap allocated, ~1.2 MB each is too much for a
    // thread's stack.
    std::vector<std::unique_ptr<ThreadHeuristics>> threadHeuristics;
    // pawn structure scores, indexed by thread id.
    std::vector<std::unique_ptr<PawnHashTable>> pawnHashes;

    // one fresh, zeroed set per thread. Does nothing when the count already matches, so the tables carry over.
    void initialize(u16 threadCount) {
        if (m_threadCount == threadCount)
            return;

        release();
        for (u16 i = 0; i < threadCount; ++i) {
            threadHeuristics.emplace_back(std::make_unique<ThreadHeuristics>());
            pawnHashes.emplace_back(std::make_unique<PawnHashTable>());
        }
        m_threadCount = threadCount;
    }

    // frees every thread's tables, the next initialize allocates them again.
    void release() {
        threadHeuristics.clear();
        pawnHashes.clear();
        m_threadCount = 0;
    }

    void reset() {
        for (auto& heuristics : threadHeuristics)
            heuristics->reset();
        for (auto& pawnHash : pawnHashes)
            pawnHash->clear();
    }

private:
    // the thread count the tables were allocated for, 0 when released.
    u16 m_threadCount = 0;
};
