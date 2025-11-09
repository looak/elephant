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

#include <functional>
#include <map>
#include <optional>
#include <vector>

#include <defines.hpp>
#include <core/chessboard.hpp>
#include <core/game_context.hpp>
#include <eval/evaluator.hpp>
#include <eval/evaluation_table.hpp>
#include <move/move.hpp>
#include <move/generation/move_generator.hpp>
#include <move/move_executor.hpp>
#include <search/transposition_table.hpp>
#include <search/search_results.hpp>
#include <search/search_policies.hpp>
#include <position/position_accessors.hpp>
#include <position/position.hpp>
#include <util/clock.hpp>
#include <util/time_manager.hpp>

struct SearchParameters {
    // search depth in half moves, a.k.a. ply or plies.
    // 0 = infinite
    u8 SearchDepth = 24;
    u8 QuiescenceDepth = quiescence_params::defaultMaxDepth;

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
    bool UseTranspositionTable = true;    
    bool UseQuiescenceSearch = true;
    bool UseNullMovePruning = true;
    bool UseLateMoveReduction = true;
};

struct MoveHistory {
private:
    std::vector<u64> recentHashes;

public:
    MoveHistory() {
        // rarely will we have a game that goes this deep.
        recentHashes.reserve(128);
    }

    void push(u64 hash) {
        recentHashes.push_back(hash);
    }
    void pop() {
        if (!recentHashes.empty()) {
            recentHashes.pop_back();
        }
    }

    bool isRepetition(u64 hashKey) const {
        int occurrences = 0;
        for (auto it = recentHashes.rbegin(); it != recentHashes.rend(); ++it) {
            if (*it == hashKey) {
                occurrences++;
                if (occurrences >= 3)
                    return true;
            }
        }        
        return false;
    }
};

struct ThreadSearchContext {
    ThreadSearchContext(Position position, bool whiteToMove)
        : position(position) {
            gameState.whiteToMove = whiteToMove;
        }
    Position position;
    GameState gameState;
    MoveHistory history;
    u64 nodeCount = 0;
    u64 qNodeCount = 0;
    std::shared_ptr<TimeManager> clock;
};

class Search {
    friend class QSearchEnabled;
public:
    Search(GameContext& context)
        : m_transpositionTable(context.editTranspositionTable())
        , m_gameContext(context)
        , m_originPosition(context.readChessPosition())
    { clear(); }

    // entry point
    template<Set us>
    SearchResult go(SearchParameters params, std::shared_ptr<TimeManager> clock);

    void clear();
    bool isKillerMove(PackedMove move, u16 ply) const;
    u32 getHistoryHeuristic(u8 set, u8 src, u8 dst) const;

private:
    // these methods will run from top down just to keep things organized.
    template<Set us>
    SearchResult dispatchSearch(ThreadSearchContext& context, SearchParameters params);    
    template<Set us, typename TT>
    SearchResult dispatchNMP(ThreadSearchContext& context, SearchParameters params);    
    template<Set us, typename TT, typename NMP>
    SearchResult dispatchLMR(ThreadSearchContext& context, SearchParameters params);
    template<Set us, typename TT, typename NMP, typename LMR>
    SearchResult dispatchQSearch(ThreadSearchContext& context, SearchParameters params);
    template<Set us, typename TT, typename NMP, typename LMR, typename QSearch>
    SearchResult dispatchDebug(ThreadSearchContext& context, SearchParameters params);
    
    // start of actual search
    template<Set us, typename Config>
    SearchResult iterativeDeepening(ThreadSearchContext& context, SearchParameters params);

    // internal seach methods
    template<Set us, typename config>
    i16 alphaBeta(ThreadSearchContext& context, u16 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv);
    template<Set us, typename config>
    i16 searchMoves(MoveGenerator<us>& gen, ThreadSearchContext& context, u16 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv, TranspositionFlag& flag, PackedMove& outMove);

    template<Set us, typename config>
    i16 quiescence(ThreadSearchContext& context, u16 depth, i16 alpha, i16 beta, u16 ply);

    template<Set us, typename config>
    bool tryNullMovePrune(ThreadSearchContext& context, u16 depth, i16 alpha, i16 beta, u16 ply);
    template<Set us, typename config>
    i16 nullmove(ThreadSearchContext& context, u16 depth, i16 alpha, i16 beta, u16 ply);

    void pushKillerMove(PackedMove mv, u16 ply);
    void putHistoryHeuristic(u8 set, u8 src, u8 dst, u32 depth);
    
    void reportResult(SearchResult& searchResult, u32 itrDepth, u64 nodes, const Clock& clock) const;

    bool allowAnotherIteration(i64 elapsedTime, i64 timeleft, i32 timeInc, u16 depth) const;
  
    EvaluationTable m_evaluationTable;
    TranspositionTable& m_transpositionTable;
    GameContext& m_gameContext;

    PackedMove m_killerMoves[4][c_maxSearchDepth];
    u32 m_historyHeuristic[2][64][64];

    PositionReader m_originPosition;

};

#include <search/impl/search_impl.inl>
#include <search/impl/search_alphabeta.inl>
#include <search/impl/search_nullmove.inl>
#include <search/impl/search_quiescence.inl>