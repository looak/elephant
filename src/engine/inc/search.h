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

#include <functional>
#include <map>
#include <optional>
#include <vector>

#include "defines.h"
#include "evaluation_table.hpp"
#include "move.h"
#include "transposition_table.hpp"

class Chessboard;
class GameContext;
struct SearchParameters;

// #ifndef DEBUG_SEARCHING
//     #define DEBUG_SEARCHING
// #endif
struct PieceKey {
    ChessPiece Piece;
    Notation SourceSqr;

    bool operator<(const PieceKey& rhs) const
    {
        if (Piece == rhs.Piece)
            return SourceSqr < rhs.SourceSqr;

        return Piece < rhs.Piece;
    }
};

struct SearchParameters {
    SearchParameters() :
        SearchDepth(5),
        MoveTime(0)
    {
    }

    // search depth in half moves, a.k.a. ply or plies.
    // 0 = infinite
    u32 SearchDepth = 5;

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
    Move move;
    bool ForcedMate = false;
};

struct SearchContext {
    u32 count;
    std::vector<Move> pv;
    std::vector<std::array<Move, 3>> killerMoves;
    // maybe add history heuristic as well.
};

// class MoveGenerator {};

class Search {
public:
    std::vector<Move> GeneratePossibleMoves(const GameContext& context, bool captureMoves = false) const;

    int Perft(GameContext& context, int depth);

    std::map<PieceKey, std::vector<Move>> OrganizeMoves(const std::vector<Move>& moves) const;

    SearchResult CalculateBestMove(GameContext& context, SearchParameters params);

private:
    template<bool UseCache>
    SearchResult AlphaBetaNegmax(GameContext& context, SearchContext& searchContext, u32 depth, u32 ply, i32 alpha, i32 beta,
                                 i32 perspective, std::vector<Move>& pv, u32 doNullMove);

    template<bool UseCache>
    i32 QuiescenceSearch(GameContext& context, u32 depth, u32 ply, i32 alpha, i32 beta, i32 perspective, u32& count);

    bool TimeManagement(i64 elapsedTime, i64 timeleft, i32 timeInc, u32 moveCount, u32 depth, i32 score);

    void OrderMoves(SearchContext& searchContext, std::vector<Move>& moves, u32 depth, u32 ply) const;

    EvaluationTable m_evaluationTable;
    TranspositionTable m_transpositionTable;
};