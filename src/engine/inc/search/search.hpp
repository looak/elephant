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
#include <search/search_policies.hpp>
#include <search/search_results.hpp>

#include <optional>
#include <atomic>
#include <vector>

// forward-declare
struct ThreadSearchContext;

class GameContext;
template<Set us>
class SearchMoveSource;
class TimeManager;

struct SearchParameters {
    // search depth in half moves, a.k.a. ply or plies.
    // 0 = no depth limit, the search runs until stopped by time or reaches the ply limit.
    u8 SearchDepth = 0;
    // u8 QuiescenceDepth = quiescence_params::defaultMaxDepth;
    u16 ThreadCount = 1;

    // total amount of time allowed to search for a move in milliseconds.
    // 0 = no time limit
    u32 MoveTime = 0;

    // time limit for white and black in milliseconds including increments.
    // 0 = no time limit.
    u32 WhiteTimelimit = 0;
    u32 BlackTimelimit = 0;
    u32 WhiteTimeIncrement = 0;
    u32 BlackTimeIncrement = 0;

    u32 MovesToGo = 0;
    bool Infinite = false;

    // stop once a search thread has visited this many nodes (nodes + qnodes), 0 = no node limit. The first iteration
    // always completes, so the count can overshoot when the limit is smaller than a depth 1 search.
    u64 NodeLimit = 0;

    // restrict the root to these moves, empty = all legal moves. Only source, target & promotion are compared, moves
    // that aren't legal in the root position are dropped and if none are left the restriction is ignored.
    std::vector<PackedMove> SearchMoves;
    // uci debug on, the helper threads also report their completed iterations as info strings.
    bool ThreadInfo = false;
};

// What the threads of one search share. The main thread (id 0) sets stop once it's done so the helpers unwind, every
// thread adds its node count to nodes as it searches.
struct SharedSearchState {
    std::atomic<bool> stop{false};
    std::atomic<u64> nodes{0};
};

class Search {
    friend class QSearchEnabled;
public:
    Search(GameContext& context);

    // entry point
    template<Set us>
    SearchResult go(SearchParameters params, TimeManager& clock);

    // Quiescence search from context's position with the whole window, line set to the line it plays. For the tuner,
    // which fits the evaluation on the quiet position at its end. context's moves are made & unmade, it's left as it was.
    template<Set us>
    static i16 quiescenceLine(ThreadSearchContext& context, PVLine& line);

    // debugging parameters -- maybe we wrap these in the development_build define
    std::atomic<u64> scout_search_count = 0;
    std::atomic<u64> scout_re_search_count = 0;
private:
    // start of actual search
    template<Set us>
    SearchResult iterativeDeepening(ThreadSearchContext& context, SearchParameters params);

    // internal seach methods
    template<Set us>
    i16 alphaBeta(ThreadSearchContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv);
    template<Set us>
    // futile: the node failed the futility check, quiet moves that don't give check are skipped.
    i16 searchMoves(SearchMoveSource<us>& moves, ThreadSearchContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv, TranspositionFlag& flag, PackedMove& outMove, bool futile);

    // moves, when given, is an unstarted move source for this position, alphaBeta passes the one it built at a leaf so
    // the check & pin info isn't computed twice. line records the line played, NoQuiescenceLine records nothing.
    template<Set us, typename Line>
    static i16 quiescence(ThreadSearchContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, Line& line,
                          SearchMoveSource<us>* moves = nullptr);

    // Returns the score to cut with when a reduced search after passing the move still fails high.
    template<Set us>
    std::optional<i16> tryNullMovePrune(ThreadSearchContext& context, u8 depth, i16 beta, u16 ply, i16 staticEval);

    void reportResult(SearchResult& searchResult, u32 itrDepth, u64 nodes, u64 elapsedTime) const;
    
    TranspositionTable& m_transpositionTable;
    GameContext& m_gameContext;

    PositionReader m_originPosition;
    SharedSearchState m_shared;
};