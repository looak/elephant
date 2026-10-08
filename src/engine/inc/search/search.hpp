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

// forward-declare
struct ThreadSearchContext;

class GameContext;
template<Set us>
class SearchMoveSource;
class TimeManager;

struct SearchParameters {
    // search depth in half moves, a.k.a. ply or plies.
    // 0 = infinite
    u8 SearchDepth = 24;
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
};

class Search {
    friend class QSearchEnabled;
public:
    Search(GameContext& context);

    // entry point
    template<Set us>
    SearchResult go(SearchParameters params, TimeManager& clock);

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

    template<Set us>
    i16 quiescence(ThreadSearchContext& context, u8 depth, i16 alpha, i16 beta, u16 ply);

    // Returns the score to cut with when a reduced search after passing the move still fails high.
    template<Set us>
    std::optional<i16> tryNullMovePrune(ThreadSearchContext& context, u8 depth, i16 beta, u16 ply, i16 staticEval);

    void reportResult(SearchResult& searchResult, u32 itrDepth, u64 nodes, u64 elapsedTime) const;
    
    TranspositionTable& m_transpositionTable;
    GameContext& m_gameContext;

    PositionReader m_originPosition;
};