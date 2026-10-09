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
#include <span>
#include <stack>

#include <io/printer.hpp>
#include <search/transposition_table_fwd.hpp>
#include <search/search_constants.hpp>
#include <system/clock.hpp>

// Forward-declare Search class for policy callbacks
class Search;

struct KillerMoves;
struct MaterialPositionMask;
struct MoveOrderingHeuristic;
struct MoveOrderingView;
struct PackedMove;

enum TranspositionFlag : u8;

/**
 * implements functions to support search heuristic policies, allows enabling and disabling them individually. */

namespace search_policies {
namespace enabled_policies {
    inline constexpr bool TT = true;
    inline constexpr bool LMR = true;
    inline constexpr bool NMP = true;
    inline constexpr bool Quiescence = true;
    inline constexpr bool DeltaPruning = true;
    inline constexpr bool QuiescenceSEEPruning = true;
    inline constexpr bool FutilityPruning = true;
    inline constexpr bool RFP = true;
    // true: search pulls moves from tusk::MoveGenerator, false: legacy MoveGenerator. See search_move_source.hpp.
    inline constexpr bool TuskMoveGen = true;
}

// --- Transposition Table Policies ---
class TT {
public:
    static constexpr bool enabled = enabled_policies::TT;
    
    static void assign(TranspositionTable& tt);
    // ply converts mate scores between root relative (search) and node relative (stored).
    static std::optional<i16> probe(u64 hash, u16 requiredDepth, u16 ply, i16 alpha, i16 beta, TranspositionFlag& flag, PackedMove& outMove);
    static bool probeMove(u64 hash, PackedMove& outMove);
    static void update(u64 hash, const PackedMove& move, i16 score, u8 depth, u16 ply, const TranspositionFlag& flag);
    static void printStats();

private:
    static inline TranspositionTable* m_table;
};

// --- Late Move Reduction (LMR) Policies ---
class LMR {
public:
    static constexpr bool enabled = enabled_policies::LMR;

    // Plies to take off a late move's search, 0 when it's searched at full depth. Only quiet moves late in the
    // ordering are reduced, never in check or when the move gives check, and the reduced search keeps depth >= 1.
    static u8 reduction(u8 depth, u16 moveIndex, bool isPV, bool quiet, bool inCheck, bool givesCheck);
};

// --- Move Ordering Heuristics (Killers/History) Policies ---
class MoveOrdering {
public:
    // quiet for move ordering, everything that lands in the quiet stage: no captures or promotions, castling included.
    static bool isQuiet(PackedMove move);

    // Killer & history update when a quiet move fails high, quietsTried are the quiets searched before it at this node.
    static void updateQuietCutoff(MoveOrderingHeuristic& heuristic, Set us, PackedMove move, std::span<const PackedMove> quietsTried, u8 depth, u16 ply);
    static void prime(const MoveOrderingHeuristic& heuristic, MoveOrderingView& view, u16 ply);
};

// --- Null Move Pruning (NMP) Policies ---

class NMP {
public:
    static constexpr bool enabled = enabled_policies::NMP;

    static bool shouldPrune(u32 depth, bool inCheck, bool hasNonPawnMaterial) { return !inCheck && depth >= nmp_params::minDepth && hasNonPawnMaterial; }
    static u8 getReduction(u8 depth) { return (depth > 6) ? 3 : 2; }
    static bool shouldVerify(u8 depth) { return depth >= nmp_params::verificationDepth; }
};

// --- Reverse Futility Pruning (RFP) Policies ---
class RFP {
public:
    static constexpr bool enabled = enabled_policies::RFP;

    // True when the static eval beats beta by a depth scaled margin at a shallow non-PV node, the node returns its
    // static eval without searching. Never in check or with beta at a mate score.
    static bool prune(bool isPV, bool inCheck, u8 depth, i16 staticEval, i16 beta);
};

// --- Futility Pruning Policies ---
class Futility {
public:
    static constexpr bool enabled = enabled_policies::FutilityPruning;

    // True when the static eval is so far below alpha at a shallow non-PV node that a quiet move isn't expected to
    // reach it. Never in check or with alpha at a mate score.
    static bool nodeIsFutile(bool isPV, bool inCheck, u8 depth, i16 staticEval, i16 alpha);
    // At a futile node quiet moves that don't give check are skipped, captures & promotions are still searched. The
    // first move is always searched so the node has a score.
    static bool skipMove(bool futileNode, u16 moveIndex, PackedMove move, bool givesCheck);
};

// --- Quiescence Search Policies ---
class QuiescencePolicy {
public:
    static constexpr bool enabled = enabled_policies::Quiescence;
    static constexpr bool deltaPruning = enabled_policies::DeltaPruning;
    // captures that lose material by static exchange are skipped, only when not in check.
    static constexpr bool seePruning = enabled_policies::QuiescenceSEEPruning;
    static u8 maxDepth;

    // Delta pruning, standing pat plus the material a capture can win and a margin still doesn't reach alpha.
    // Only when not in check, evasions are all searched.
    // Node: even winning the opponent's most valuable piece, or promoting, isn't enough. Nothing here is worth searching.
    static bool deltaPruneNode(const MaterialPositionMask& material, Set us, i16 standPat, i16 alpha);
    // Move: this capture (or promotion) alone isn't enough, skip it.
    static bool deltaPruneMove(const MaterialPositionMask& material, PackedMove move, i16 standPat, i16 alpha);
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