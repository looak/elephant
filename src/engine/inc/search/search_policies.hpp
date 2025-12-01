/******************************************************************************
* Elephant Gambit Chess Engine - a Chess AI
* Copyright(C) 2025  Alexander Loodin Ek,
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

#include <optional>
#include <search/transposition_table_fwd.hpp>
#include <search/search_constants.hpp>

// Forward-declare Search class for policy callbacks
class Search;

struct KillerMoves;
struct MoveOrderingView;
struct PackedMove;

enum TranspositionFlag : u8;

/**
 * implements functions to support search heuristic policies, allows enabling and disabling them individually. */

namespace search_policies {
namespace enabled_policies {
    inline constexpr bool TT = true;
    inline constexpr bool LMR = false;
    inline constexpr bool NMP = false;
    inline constexpr bool Quiescence = true;
}

// --- Transposition Table Policies ---
class TT {
public:
    static constexpr bool enabled = enabled_policies::TT;
    
    static void assign(TranspositionTable& tt);
    static std::optional<i16> probe(u64 hash, u16 requiredDepth, i16 alpha, i16 beta, TranspositionFlag& flag, PackedMove& outMove);
    static bool probeMove(u64 hash, PackedMove& outMove);
    static void update(u64 hash, const PackedMove& move, i16 score, u8 depth, const TranspositionFlag& flag);
    static void printStats();

private:
    static inline TranspositionTable* m_table;
};

// --- Late Move Reduction (LMR) Policies ---
class LMR {
public:
    static constexpr bool enabled = enabled_policies::LMR;

    static bool shouldReduce(u32 depth, const PackedMove& move, u16 index, bool isChecked, bool /*isChecking */);
    static u8 getReduction(u8 depth);
};

// --- Move Ordering Heuristics (Killers/History) Policies ---
class MoveOrdering {
public:
    static void push(KillerMoves& killers, PackedMove move, u16 ply);
    static void prime(const KillerMoves& killers, MoveOrderingView& view, u16 ply);
};

// --- Null Move Pruning (NMP) Policies ---

class NMP {
public:
    static constexpr bool enabled = enabled_policies::NMP;

    static bool shouldPrune(u32 depth, bool inCheck, bool hasNonPawnMaterial) { return !inCheck && depth >= 3 && hasNonPawnMaterial; }
    static u8 getReduction(u8 depth) { return (depth > 6) ? 3 : 2; }
};

// --- Quiescence Search Policies ---
class QuiescencePolicy {
public:
    static constexpr bool enabled = enabled_policies::Quiescence;
    static u8 maxDepth;

    static bool futile(u8 depth, i32 eval, i16 alpha) {
        return depth > 0
        && (depth < quiescence_params::futilityDepthMargin)
        && (eval + quiescence_params::futilityMargin < alpha);
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

} // namespace search_policies