#pragma once

#include "search/search_constants.hpp"
#include "search/search_results.hpp"
#include "search/transposition_table.hpp"

#include "util/clock.hpp"
#include "io/printer.hpp"
#include "core/game_context.hpp"

#include <optional>
#include <stack>

// Forward-declare Search class for policy callbacks
class Search;

namespace search_policies {
// --- Transposition Table Policies ---

class TTEnabled {
public:
    static std::optional<i32> probe(
        TranspositionTable& tt, u64 hash, u8 depth, i32 alpha, i32 beta, i32 ply) 
    {
        TranspositionEntry& entry = tt.editEntry(hash);
        if (auto result = entry.evaluate(hash, depth, alpha, beta); result.has_value()) {
            return entry.adjustedScore(ply);
        }
        return std::nullopt;
    }

    static void store(
        TranspositionTable& tt, u64 hash, PackedMove move, u16 age, 
        i16 score, i32 ply, u8 depth, TranspositionFlag flag)
    {
        TranspositionEntry& entry = tt.editEntry(hash);
        entry.update(hash, move, age, score, ply, depth, flag);
    }

    static void update(TranspositionTable& tt, u64 hash, PackedMove move, u16 age, i16 score, i32 ply, u8 depth, TranspositionFlag flag)
    {
        TranspositionEntry& entry = tt.editEntry(hash);
        entry.update(hash, move, age, score, ply, depth, flag);
    }
};

class TTDisabled {
public:
    static std::optional<i32> probe(TranspositionTable&, u64, u8, i32, i32, i32) 
    { return std::nullopt; }

    static void store(TranspositionTable&, u64, PackedMove, u16, i16, i32, u8, TranspositionFlag)
    { /* Do nothing */ }

    static void update(TranspositionTable&, u64, PackedMove, u16, i16, i32, u8, TranspositionFlag)
    { /* Do nothing */ }
};


// --- Late Move Reduction (LMR) Policies ---

class LmrEnabled {
public:
    static constexpr bool enabled = true;
    static bool shouldReduce(u32 depth, PackedMove move, bool isChecked) {
        return depth > 2 && move.isQuiet() && !isChecked;
    }

    static u32 getReduction(u32 depth) {
        u32 reduction = 1;
        if (depth > 6) reduction++;
        return std::min(reduction, depth - 1); 
    }
};

class LmrDisabled {
public:
    static constexpr bool enabled = false;
    static bool shouldReduce(u32, u32, bool, bool) { return false; }
    static u32 getReduction(u32, u32) { return 0; }
};


// --- Move Ordering Heuristics (Killers/History) Policies ---

// class OrderingEnabled {
// public:
//     static void store_killer(Search& search, PackedMove move, u32 ply) {
//         search.pushKillerMove(move, ply);
//     }
//     static void store_history(Search& search, Set set, PackedMove move, u32 depth) {
//         search.putHistoryHeuristic(static_cast<u8>(set), move.source(), move.target(), depth);
//     }
// };

// class OrderingDisabled {
// public:
//     static void store_killer(Search&, PackedMove, u32) { /* Do nothing */ }
//     static void store_history(Search&, Set, PackedMove, u32) { /* Do nothing */ }
// };

// --- Null Move Pruning (NMP) Policies ---

class NmpEnabled {
public:
    static constexpr bool enabled = true;
    static bool shouldPrune(u32 depth, bool inCheck, bool hasNonPawnMaterial) {
        return !inCheck && depth >= 3 && hasNonPawnMaterial;
    }
    static u32 getReduction(u32 depth) {
        return (depth > 6) ? 3 : 2;
    }
};

class NmpDisabled {
public:
    static constexpr bool enabled = false;
    static bool shouldPrune(u32, bool) { return false; }
    static u32 getReduction(u32) { return 0; }
};

// --- Quiescence Search Policies ---

class QSearchEnabled {
public:
    static constexpr bool enabled = true;
    static inline u16 maxDepth = 12;
};

class QSearchDisabled {
public:
    static constexpr bool enabled = false;
    static inline u16 maxDepth = 0;
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


template<
    typename TT,
    typename NMP,
    typename LMR,
    typename QSearch,
    typename Debug
>
struct SearchConfig {
    using TT_Policy = TT;
    using NMP_Policy = NMP;
    using LMR_Policy = LMR;
    using QSearch_Policy = QSearch;    
    using Debug_Policy = Debug;
};
