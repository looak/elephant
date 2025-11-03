#pragma once

#include "search/search_constants.hpp"
#include "search/search_results.hpp"
#include "search/transposition_table.hpp"

#include "util/clock.hpp"
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
    static bool should_reduce(u32 depth, u32 move_index, bool is_capture, bool in_check) {
        return depth > 2 && move_index > 3 && !is_capture && !in_check;
    }
    
    static u32 get_reduction(u32 depth, u32 move_index) {
        u32 reduction = 1;
        if (move_index > 6) reduction++;
        if (depth > 6) reduction++;
        return std::min(reduction, depth - 1); 
    }
};

class LmrDisabled {
public:
    static bool should_reduce(u32, u32, bool, bool) { return false; }
    static u32 get_reduction(u32, u32) { return 0; }
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
    static bool should_prune(u32 depth, bool in_check) {
        return !in_check && depth >= 3;
    }
    static u32 get_reduction(u32 depth) {
        return (depth > 6) ? 3 : 2;
    }
};

class NmpDisabled {
public:
    static bool should_prune(u32, bool) { return false; }
    static u32 get_reduction(u32) { return 0; }
};

// --- Quiescence Search Policies ---

class QSearchEnabled {
public:
    static constexpr bool enabled = true;
};

class QSearchDisabled {
public:
    static constexpr bool enabled = false;
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

    static void popClock(u64 nodes, u64& nps) {
        if (!m_searchClocks.empty()) {
            Clock& clock = m_searchClocks.top();
            clock.Stop();
            nps = clock.calcNodesPerSecond(nodes);
            m_searchClocks.pop();
        }
    }

    static void reportNps(u64 nps) {
        LOG_INFO() << "NPS: " << nps << "\n";
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
    static u64 popClock(u64) {
        m_dummyClock.Stop();
        return 0;
    }

    static void reportNps(u64) {
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
