// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021-2026  Alexander Loodin Ek

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
 * @file search_node.hpp
 * @brief The typed steps of one search node: open, gate, plan, search, record.
 *
 * @author Alexander Loodin Ek    */
#pragma once

#include <cassert>
#include <optional>
#include <utility>
#include <material/chess_piece_defines.hpp>
#include <move/generation/move_ordering_view.hpp>
#include <move/move.hpp>
#include <search/core/search_policies.hpp>
#include <system/platform.hpp>

struct PVLine;
struct ThreadContext;

namespace tusk {
template<Set us>
class CheckInfo;
}

// Where a node is in the tree, decided at compile time so the root's special cases cost the rest of the tree nothing.
enum class NodeKind {
    NORMAL,
    ROOT,
    // the root of a go searchmoves, only the moves in the request are searched.
    RESTRICTED_ROOT
};

struct Cut { i16 score; };

template<typename Next>
class [[nodiscard]] StageResult {
public:
    StageResult(Cut c) : m_score(c.score) {};
    static StageResult cut(i16 score)  { return StageResult{score}; }
    static StageResult carryOn(Next n) { return StageResult{std::move(n)}; }

    bool  isCut() const { return !m_next.has_value(); }
    i16   score() const { assert(isCut());  return m_score; }
    Next& next()        { assert(!isCut()); return *m_next; }
private:
    explicit StageResult(i16 s)  : m_score(s) {}
    explicit StageResult(Next n) : m_next(std::move(n)) {}
    std::optional<Next> m_next;
    i16 m_score = 0;
};


struct NodeInputs {      // from the caller, passed by copy
    i16 alpha = 0;
    i16 beta = 0;
    u8 depth = 0;
    u16 ply = 0;

    // the child's inputs, the window flipped to its side one ply further down.
    [[nodiscard]] NodeInputs makeChildInputs(u8 childDepth) const {
        return { static_cast<i16>(-beta), static_cast<i16>(-alpha), childDepth, static_cast<u16>(ply + 1) };
    }
    // the child's inputs with a zero window just above alpha, it only has to show whether the move beats alpha.
    [[nodiscard]] NodeInputs makeScoutInputs(u8 childDepth) const {
        return { static_cast<i16>(-alpha - 1), static_cast<i16>(-alpha), childDepth, static_cast<u16>(ply + 1) };
    }
};

struct NodePlan {
    bool futile = false;  // futile: the node failed the futility check, quiet moves that don't give check are skipped.        
    u8 singularExtension = 0;
    MoveOrderingView orderingView{};  //  ordering: the view moves was started with, it reads the history tables as they are now.
};

struct NodeResult {      // produced by the move loop
    i16 score = 0;
    PackedMove bestMove{};
    TranspositionFlag flag{};
    // PVLine* pv = nullptr; // todo maybe rethink pv not being part of the result.
};


template<Set us>
struct NodeEnv {                 // borrowed from alphaBeta's stack, same for every stage
    ThreadContext& context;
    const tusk::CheckInfo<us>& checkInfo;
    PVLine* pv;
};

template<Set us, NodeKind kind>
struct GateNode {
    NodeEnv<us> env;
    NodeInputs inputs;
    PackedMove pvHint;
    bool isPv;
    std::optional<search_policies::TT::ProbeResult> probeResult;
};

template<Set us, NodeKind kind>
struct OpenNode {
    NodeEnv<us> env;
    NodeInputs inputs;

    StageResult<GateNode<us, kind>> gate();
};