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

#include "search/search_constants.hpp"
#include "search/search_heuristic_structures.hpp"
#include "search/search_results.hpp"
#include "search/transposition_table.hpp"

#include "util/clock.hpp"
#include "io/printer.hpp"
#include "core/game_context.hpp"

#include <optional>
#include <stack>

// TODO: Implementation here could be moved to a .cpp

// Forward-declare Search class for policy callbacks
class Search;

namespace search_policies {

// --- Transposition Table Policies ---
class TT {
public:
    static constexpr bool enabled = true;
    static void assign(TranspositionTable& tt) {
        m_table = &tt;        
    }

    static std::optional<i16> probe(u64 hash, u8 requiredDepth, i16 alpha, i16 beta, u16 ply, TranspositionFlag& flag, PackedMove& outMove) {        
        i16 score;
        u8 depth;
        if (m_table->probe(hash, outMove, score, depth, flag) == false)
            return std::nullopt;

        if (depth < requiredDepth)
            return std::nullopt;

        // mate score is already adjusted during search, so we don't need to adjust it again here.

        switch (flag) {
            case TTF_CUT_EXACT:
                return score;
            case TTF_CUT_BETA:
                if (score >= beta)
                    return score;
                break;
            case TTF_CUT_ALPHA:
                if (score <= alpha)
                    return score;
                break;
            default:
                break;
        }

        return std::nullopt;
    }

    static bool probeMove(u64 hash, PackedMove& outMove) {
        i16 dummyScore;
        u8 dummyDepth;
        TranspositionFlag dummyFlag;
        return m_table->probe(hash, outMove, dummyScore, dummyDepth, dummyFlag);
    }

    static void update(u64 hash, PackedMove move, i16 score, i32 ply, u8 depth, TranspositionFlag flag)
    {
        m_table->store(hash, move, score, depth, flag);
    }

    static void printStats() 
    {
    #ifdef DEBUG_TRANSITION_TABLE
        m_table->print_stats();
    #endif
    }

private:

    static inline TranspositionTable* m_table;
};

// --- Late Move Reduction (LMR) Policies ---
class LMR {
public:
    static constexpr bool enabled = true;
    static bool shouldReduce(u32 depth, PackedMove move, u16 index, bool isChecked, bool isChecking) {
        return depth > lmr_params::minDepth 
        && (move.isQuiet() || index > lmr_params::reduceAfterIndex)
        && isChecked == false;
        //&& isChecking == false;
    }

    static u32 getReduction(u32 depth) {
        u32 reduction = 1;
        if (depth > lmr_params::earlyReductionThreshold) reduction++;
        return std::min(reduction, depth - 1); 
    }
};


// --- Move Ordering Heuristics (Killers/History) Policies ---
class MoveOrdering {
public:
    static void push(KillerMoves& killers, PackedMove move, u32 ply) {
        killers.push(move, ply);
    }
    static void prime(const KillerMoves& killers, MoveOrderingView& view, u32 ply) {
        killers.retrieve(ply, view.killers[0], view.killers[1]);
    }
};

// --- Null Move Pruning (NMP) Policies ---

class NMP {
public:
    static constexpr bool enabled = true;
    static bool shouldPrune(u32 depth, bool inCheck, bool hasNonPawnMaterial) {
        return !inCheck && depth >= 3 && hasNonPawnMaterial;
    }
    static u32 getReduction(u32 depth) {
        return (depth > 6) ? 3 : 2;
    }
};

// --- Quiescence Search Policies ---

class QuiescencePolicy {
public:
    static constexpr bool enabled = true;
    static inline u16 maxDepth = 12;

    static bool futile(u8 depth, i16 eval, i16 alpha) {
        return (depth < quiescence_params::futilityDepthMargin)
        && (alpha < eval + quiescence_params::futilityMargin);
    }
};

// --- Debug Policies ---

#if defined(DEVELOPMENT_BUILD)
class DebugEnabled {
public:
    static const Clock& pushClock() {
        Clock clock;
        clock.Start();
        m_searchClocks.push(clock);
        return m_searchClocks.top();
    }

    static void popClock() {
        if (!m_searchClocks.empty()) {
            Clock& clock = m_searchClocks.top();
            clock.Stop();            
            m_searchClocks.pop();
        }
    }

    static void reportNps(u64 nodes, u64 qnodes) {
        const Clock& clock = m_searchClocks.top();

        std::cout << " ------------------------------ \n";
        std::cout << " Nodes: " << io::printer::formatReadableNumber(nodes)
                  << " QNodes: " << io::printer::formatReadableNumber(qnodes)
                  << " Total: " << io::printer::formatReadableNumber(nodes + qnodes) << "\n";
        std::cout << " NPS:   " << io::printer::formatReadableNumber(clock.calcNodesPerSecond(nodes))
                  << " QNPS: " << io::printer::formatReadableNumber(clock.calcNodesPerSecond(qnodes)) << "\n";
        std::cout << " Total NPS: " << io::printer::formatReadableNumber(clock.calcNodesPerSecond(nodes + qnodes)) << "\n";
    }

private:
    static inline std::stack<Clock> m_searchClocks;
};
#endif

class DebugDisabled {
public:
    static const Clock& pushClock() {
        m_dummyClock.Start();
        return m_dummyClock;
    }
    static u64 popClock() {
        m_dummyClock.Stop();
        return 0;
    }

    static void reportNps(u64, u64) {
        // Do nothing
    }

private:
    static inline Clock m_dummyClock;
};

} // namespace search_policies