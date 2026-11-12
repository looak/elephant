// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021-2023  Alexander Loodin Ek

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
#include <material/chess_piece_defines.hpp>
#include <position/position_accessors.hpp>
#include <eval/pawn_hash.hpp>
#include <search/core/search_policies.hpp>
#include <search/search_listener.hpp>
#include <search/search_request.hpp>
#include <search/search_results.hpp>
#include <search/state/search_heuristic_structures.hpp>
#include <search/state/transposition_table_fwd.hpp>

#include <optional>
#include <atomic>
#include <vector>

// forward-declare
struct ThreadContext;
struct MoveOrderingView;

template<Set us>
class SearchMoveSource;
class TimeManager;

// What the threads of one search share. The main thread (id 0) sets stop once it's done so the helpers unwind, every
// thread adds its node count to nodes as it searches.
struct SharedSearchState {
    std::atomic<bool> stop{false};
    std::atomic<u64> nodes{0};
};

// Where a node is in the tree, decided at compile time so the root's special cases cost the rest of the tree nothing.
enum class NodeKind {
    NORMAL,
    ROOT,
    // the root of a go searchmoves, only the moves in the request are searched.
    RESTRICTED_ROOT
};

// A search thread's tables and the search itself. It holds what outlives a go, the history tables & the pawn hash, and
// builds what a go needs on the thread's stack, a ThreadContext, so nothing of one search can leak into the next without
// being meant to. It isn't a thread, a Monad starts one for each Worker when a search starts. Big, it belongs on the heap.
class Worker {
    friend class QuiescenceProbe;
public:
    // Games are independent: zeroes the tables, keeping the memory.
    void newGame();

    // Searches request on the calling thread until it's done. Thread 0 is the main thread: it reports, manages time and
    // sets shared.stop when it's done, its result is the one played. request, clock & tt are read throughout.
    SearchResult run(const SearchRequest& request, const TimeManager& clock, TranspositionTable& tt, SharedSearchState& shared,
                     SearchListener& listener, u16 threadId, u16 threadCount);

private:
    struct NodeInputs {      // from the caller, passed by copy
        i16 alpha = 0;
        i16 beta = 0;
        u8 depth = 0;
        u16 ply = 0;
    };

    struct NodePlan {        // decided by prepareNode, passed by const reference        
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
    SearchResult iterativeDeepening(ThreadContext& context, SearchListener& listener);

    // internal seach methods
    template<Set us, NodeKind kind = NodeKind::NORMAL>
    i16 alphaBeta(ThreadContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv);
    
    template<Set us, typename Moves>
    NodeResult searchMoves(Moves& moves, ThreadContext& context, NodeInputs node, const NodePlan& plan, PVLine* pv);

    // Quiescence search from context's position with the whole window, line set to the line it plays. context's moves are
    // made & unmade, it's left as it was. See QuiescenceProbe.
    template<Set us>
    static i16 quiescenceLine(ThreadContext& context, PVLine& line);

    // moves, when given, is an unstarted move source for this position, alphaBeta passes the one it built at a leaf so
    // the check & pin info isn't computed twice. line records the line played, NoQuiescenceLine records nothing.
    template<Set us, typename Line>
    static i16 quiescence(ThreadContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, Line& line,
                          SearchMoveSource<us>* moves = nullptr);

    // Returns the score to cut with when a reduced search after passing the move still fails high.
    template<Set us>
    std::optional<i16> tryNullMovePrune(ThreadContext& context, u8 depth, i16 beta, u16 ply, i16 staticEval);

    ThreadHeuristics m_heuristics;
    PawnHashTable m_pawnHash;
};
