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
#include <search/state/search_context.hpp>
#include <search/core/search_policies.hpp>
#include <search/search_listener.hpp>
#include <search/search_request.hpp>
#include <search/search_results.hpp>

#include <optional>
#include <atomic>
#include <vector>

// forward-declare
struct ThreadContext;

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

class Search {
    friend class QuiescenceProbe;
public:
    Search(SearchContext& searchContext, TranspositionTable& transpositionTable,
           SearchListener& listener = noSearchListener());

    // entry point, one thread for every thread the SearchContext was sized for. Blocks until they are done, request and
    // clock are read throughout.
    SearchResult go(const SearchRequest& request, TimeManager& clock);

private:
    // Quiescence search from context's position with the whole window, line set to the line it plays. context's moves are
    // made & unmade, it's left as it was. See QuiescenceProbe.
    template<Set us>
    static i16 quiescenceLine(ThreadContext& context, PVLine& line);

    template<Set us>
    SearchResult run(const SearchRequest& request, TimeManager& clock);

    // start of actual search
    template<Set us>
    SearchResult iterativeDeepening(ThreadContext& context);

    // internal seach methods
    template<Set us, NodeKind kind = NodeKind::NORMAL>
    i16 alphaBeta(ThreadContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv);
    // futile: the node failed the futility check, quiet moves that don't give check are skipped.
    template<Set us, typename Moves>
    i16 searchMoves(Moves& moves, ThreadContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv, TranspositionFlag& flag, PackedMove& outMove, bool futile);

    // moves, when given, is an unstarted move source for this position, alphaBeta passes the one it built at a leaf so
    // the check & pin info isn't computed twice. line records the line played, NoQuiescenceLine records nothing.
    template<Set us, typename Line>
    static i16 quiescence(ThreadContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, Line& line,
                          SearchMoveSource<us>* moves = nullptr);

    // Returns the score to cut with when a reduced search after passing the move still fails high.
    template<Set us>
    std::optional<i16> tryNullMovePrune(ThreadContext& context, u8 depth, i16 beta, u16 ply, i16 staticEval);

    
    TranspositionTable& m_transpositionTable;
    SearchContext& m_searchContext;
    SearchListener& m_listener;

    SharedSearchState m_shared;
};