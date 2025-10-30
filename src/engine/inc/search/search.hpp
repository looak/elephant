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
#include <eval/evaluation_table.hpp>
#include <move/move.hpp>
#include <search/transposition_table.hpp>
#include <position/position_accessors.hpp>
#include <position/position.hpp>

class Chessboard;
class Clock;
class GameContext;
struct SearchParameters;

// #ifndef DEBUG_SEARCHING
//     #define DEBUG_SEARCHING
// #endif
struct PieceKey {
    ChessPiece Piece;
    Square SourceSqr;

    bool operator<(const PieceKey& rhs) const
    {
        if (Piece == rhs.Piece)
            return SourceSqr < rhs.SourceSqr;

        return Piece < rhs.Piece;
    }
};

struct SearchParameters {
    // search depth in half moves, a.k.a. ply or plies.
    // 0 = infinite
    u8 SearchDepth = 6;
    u8 QuiescenceDepth = 4;

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

struct SearchResult {
    i32 score;
    PackedMove move;
    bool ForcedMate = false;
    u64 count = 0;
};

typedef std::function<bool()> CancelSearchCondition;

struct SearchContext {
    PositionEditor position;    
    CancelSearchCondition& cancel;
};

class Search {
public:
    Search(Position position) { clear(); }

    template<Set us>
    SearchResult go(SearchParameters params = {});

    // template<Set us>
    // SearchResult goIterativeDeepening(SearchParameters params = {});

    SearchResult CalculateBestMove(GameContext& context, SearchParameters params);
    i32 CalculateMove(GameContext& context, bool maximizingPlayer, u32 depth);

    void clear();
    bool isKillerMove(PackedMove move, u32 ply) const;
    u32 getHistoryHeuristic(u8 set, u8 src, u8 dst) const;

private:
    void ReportSearchResult(SearchContext& context, SearchResult& searchResult, u32 searchDepth, u32 itrDepth, u64 nodes, const Clock& clock) const;


    SearchResult    CalculateBestMoveIterration(SearchContext& context, u32 depth);

    template<Set us>
    SearchResult    recursiveAlphaBetaNegamax(SearchContext& context, u32 depth, i32 alpha, i32 beta, u32 ply);
    template<Set us>
    i32             recursiveQuiescenceNegamax(SearchContext& context, u32 depth, i32 alpha, i32 beta, u32 ply);

    bool TimeManagement(i64 elapsedTime, i64 timeleft, i32 timeInc, u32 depth);
    CancelSearchCondition buildCancellationFunction(Set perspective, const SearchParameters& params, const Clock& clock) const;


    i32 Extension(const Chessboard& board, const PrioritizedMove& prioratized, u32 ply) const;
    void pushKillerMove(PackedMove mv, u32 ply);
    void putHistoryHeuristic(u8 set, u8 src, u8 dst, u32 depth);

    EvaluationTable m_evaluationTable;
    // TranspositionTable m_transpositionTable;

    PackedMove m_killerMoves[4][64];
    u32 m_historyHeuristic[2][64][64];

};