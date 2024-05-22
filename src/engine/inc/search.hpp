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
    u32 QuiescenceDepth = 2;

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
};

struct SearchContext {
    u32 count;
    std::vector<ScoredMove> pv;
    std::vector<std::array<Move, 3>> killerMoves;
    // maybe add history heuristic as well.
};

struct PerftResult {
    u64 Nodes = 0;
    u64 Captures = 0;
    u64 EnPassants = 0;
    u64 Castles = 0;
    u64 Promotions = 0;
    u64 Checks = 0;
    u64 Checkmates = 0;
    u64 Depth = 0;

    void operator+=(const PerftResult& rhs)
    {
        this->Nodes += rhs.Nodes;
        this->Captures += rhs.Captures;
        this->EnPassants += rhs.EnPassants;
        this->Castles += rhs.Castles;
        this->Promotions += rhs.Promotions;
        this->Checks += rhs.Checks;
        this->Checkmates += rhs.Checkmates;
        this->Depth += rhs.Depth;
    }
};

// class MoveGenerator {};

class Search {
public:
    PerftResult Perft(GameContext& context, int depth);
    PerftResult PerftDivide(GameContext& context, int depth);

    std::map<PieceKey, std::vector<Move>> OrganizeMoves(const std::vector<Move>& moves) const;

    SearchResult CalculateBestMove(GameContext& context, SearchParameters params);
    SearchResult CalculateBestMove(GameContext& context);

    i32 CalculateMove(GameContext& context, bool maximizingPlayer, u32 depth);

private:
    template<bool UseCache>
    SearchResult AlphaBetaNegmax(GameContext& context, SearchContext& searchContext, u32 depth, u32 ply, i32 alpha, i32 beta, std::vector<ScoredMove>& pv, u32 doNullMove);

    template<bool UseCache>
    i32 QuiescenceSearch(GameContext& context, u32 depth, u32 ply, i32 alpha, i32 beta, u32& count);

    SearchResult AlphaBetaMinmax(GameContext& context, u32 depth, i32 alpha, i32 beta, bool maximizingPlayer);

    bool TimeManagement(i64 elapsedTime, i64 timeleft, i32 timeInc, u32 moveCount, u32 depth, i32 score);

    void OrderMoves(SearchContext& searchContext, std::vector<Move>& moves, u32 depth, u32 ply) const;

    EvaluationTable m_evaluationTable;
    TranspositionTable m_transpositionTable;
};