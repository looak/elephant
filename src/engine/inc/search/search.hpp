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
#include <move/generation/move_generator.hpp>
#include <move/move_executor.hpp>
#include <search/transposition_table.hpp>
#include <search/search_results.hpp>
#include <search/search_policies.hpp>
#include <position/position_accessors.hpp>
#include <position/position.hpp>
#include <core/game_context.hpp>
#include <core/chessboard.hpp>
#include <util/clock.hpp>
#include <eval/evaluator.hpp>

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
    bool UseTranspositionTable = true;
    bool UseIterativeDeepening = true;
    bool UseQuiescenceSearch = true;
    bool UseNullMovePruning = true;
    bool UseLateMoveReduction = true;
    bool UseMoveOrdering = true;
};

typedef std::function<bool()> CancelSearchCondition;

struct ThreadSearchContext {
    ThreadSearchContext(Position position, u64 nodeCount, bool whiteToMove)
        : position(position), nodeCount(nodeCount) {
            gameState.whiteToMove = whiteToMove;
        }
    Position position;
    GameState gameState;
    GameHistory gameHistory;
    u64 nodeCount;
    
};

class Search {
    friend class QSearchEnabled;
public:
    Search(PositionReader position, TranspositionTable& tt) : 
        m_originPosition(position), m_transpositionTable(tt) 
        { clear(); }

    // entry point
    template<Set us>
    SearchResult go(SearchParameters params = {});

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
    SearchResult runSearchWithConfig(ThreadSearchContext& context, SearchParameters params);

    // internal seach methods
    template<Set us, typename config>
    SearchResult recursiveAlphaBetaNegamax(ThreadSearchContext& context, u32 depth, i32 alpha, i32 beta, u16 ply);
    template<Set us, typename config>
    i32 recursiveQuiescenceNegamax(ThreadSearchContext& context, u32 depth, i32 alpha, i32 beta, u16 ply);
    
    void pushKillerMove(PackedMove mv, u16 ply);
    void putHistoryHeuristic(u8 set, u8 src, u8 dst, u32 depth);
    
    
    bool TimeManagement(i64 elapsedTime, i64 timeleft, i32 timeInc, u32 depth);
    CancelSearchCondition buildCancellationFunction(Set perspective, const SearchParameters& params, const Clock& clock) const;
    i32 Extension(const Chessboard& board, const PrioritizedMove& prioratized, u16 ply) const;
    void ReportSearchResult(ThreadSearchContext& context, SearchResult& searchResult, u32 searchDepth, u32 itrDepth, u64 nodes, const Clock& clock) const;
    void setConfig(const SearchParameters& params);

    EvaluationTable m_evaluationTable;
    TranspositionTable& m_transpositionTable;

    PackedMove m_killerMoves[4][64];
    u32 m_historyHeuristic[2][64][64];

    PositionReader m_originPosition;
};

template<Set us>
SearchResult Search::go(SearchParameters params)
{    
    Clock searchClock;
    searchClock.Start();

    u64 nodeCount = 0;
    u64 ply = 1;
    //std::function<bool()> cancellationFunc = buildCancellationFunction(us, params, searchClock);
    ThreadSearchContext searchContext(m_originPosition.copy(), nodeCount, us == Set::WHITE);
    SearchResult result;

    result = dispatchSearch<us>(searchContext, params);

    result.count = nodeCount;
    return result;
}

template<Set us, typename config>
SearchResult Search::runSearchWithConfig(ThreadSearchContext& context, SearchParameters params) {
    
    SearchResult result;
    // iterative deepening loop -- might make this optional.
    int ply = 1;
    for (u32 itrDepth = 1; itrDepth <= params.SearchDepth; ++itrDepth) {

        config::Debug_Policy::pushClock();

        auto itrResult = recursiveAlphaBetaNegamax<us, config>(context, itrDepth, -c_maxScore, c_maxScore, ply);

        u64 nps = config::Debug_Policy::popClock(context.nodeCount);
        config::Debug_Policy::reportNps(nps);

        // bool cancelled = cancellationFunc();
        // if (cancelled) {
        //     itrResult = result;
        // }

        // ReportSearchResult(searchContext, itrResult, params.SearchDepth, itrDepth, nodeCount, clock);

        i32 checkmateDistance = c_checkmateConstant - abs((int)itrResult.score);
        checkmateDistance = abs(checkmateDistance);
        if ((u32)checkmateDistance <= c_maxSearchDepth) {
            // found checkmate within depth.
            itrResult.ForcedMate = true;
            //checkmateDistance /= 2;
//         std::cout << "info mate " << checkmateDistance << " depth " << itrDepth << " nodes " << nodes
//             << " time " << et << " pv" << pvSS.str() << "\n";

        }





        if (itrResult.ForcedMate) {
#ifdef DEBUG_TRANSITION_TABLE
            context.editTranspositionTable().debugStatistics();
#endif
            return itrResult;
        }

        // if (cancelled == true)
        //     break;

        result = itrResult;
    }

#ifdef DEBUG_TRANSITION_TABLE
    context.editTranspositionTable().debugStatistics();
#endif

    return result;
}

template<Set us, typename config>
SearchResult Search::recursiveAlphaBetaNegamax(ThreadSearchContext& context, u32 depth, i32 alpha, i32 beta, u16 ply) {
    if (depth <= 0) {
        if constexpr (config::QSearch_Policy::enabled) {
            i32 score = recursiveQuiescenceNegamax<us, config>(context, depth, alpha, beta, ply);
            return { .score = score, .move = PackedMove::NullMove() };
        }
        else {
            Evaluator evaluator(context.position.read());
            i32 perspective = 0;
            if constexpr (us == Set::WHITE) {
                perspective = 1;
            }
            else {
                perspective = -1;
            }
            i32 score = evaluator.Evaluate() * perspective;
            return { .score = score, .move = PackedMove::NullMove() };
        }
    }

    PositionReader currentPos = context.position.read();

    // initialize the move generator.
    MoveGenParams genParams;
    MoveGenerator<us> generator(currentPos, genParams);
    auto prioratized = generator.generateNextMove();

    // if there are no moves to make, we're either in checkmate or stalemate.
    if (prioratized.move.isNull()) {
        if (generator.isChecked() == true)
            return { -c_checkmateConstant - (i32)ply, PackedMove::NullMove() };  // negative "infinity" since we're in checkmate
        return { .score = -c_drawConstant, .move = PackedMove::NullMove() };  // we're in stalemate
    }

    i32 bestEval = -c_maxScore;
    i32 eval = -c_maxScore;
    PackedMove bestMove;

    // probe transposition table.
    auto result = config::TT_Policy::probe(m_transpositionTable, currentPos.hash(), depth, alpha, beta, ply);
    if (result.has_value()) {
        return result.value();
    }

    auto flag = TranspositionFlag::TTF_CUT_EXACT;

// #if defined(ENABLE_LATE_MOVE_REDUCTION)
//     static const i8 depthReductionThreshold = 4;
//     i8 depthReductionCounter = 0;
// #endif

    MoveExecutor executor(context.position.edit());

    do {
        u32 extendedDepth = depth; // + Extension(chessboard, prioratized, ply);
        bool doFullSearch = true;

//#if defined(ENABLE_LATE_MOVE_REDUCTION)
        // should implement research on beta cutoffs.
        // if (config::LMR_Policy::should_reduce(depth, depthReductionCounter, prioratized.move.isCapture(), generator.isChecked())) {
        //     u32 reduction = config::LMR_Policy::get_reduction(depth, depthReductionCounter);
        //     extendedDepth -= reduction;
        // }
        // if (depth > 3 && depthReductionCounter >= depthReductionThreshold && extendedDepth == 0 && prioratized.move.isCapture() == false) {
        //     result = recursiveAlphaBetaNegamax<us, config>(context, extendedDepth - 1, -beta, -alpha, ply + 1);
        //     doFullSearch = result.score > alpha;
        // }

        // depthReductionCounter++;
//#endif
        MoveUndoUnit undoState;
        u16 movingPly = ply; // make move modifies ply according to chess rules, but we don't care about that modified ply in search.
        executor.makeMove(prioratized.move, undoState, movingPly);

        
        // if (context.gameHistory.IsRepetition(currentPos.hash()) == true) {
        //     eval = -c_drawConstant;
        //     result = { .score = eval, .move = prioratized.move };
        // }
        //else
        // if (doFullSearch) {
        eval = -recursiveAlphaBetaNegamax<opposing_set<us>(), config>(context, extendedDepth - 1, -beta, -alpha, ply + 1).score;
        //}

        executor.unmakeMove(undoState);
        context.nodeCount++;

        // if (context.cancel() == true)
        //     return { .score = 0, .move = PackedMove::NullMove() };        

        if (eval > bestEval) {
            bestEval = eval;
            bestMove = prioratized.move;

            if (bestEval > alpha) {
                alpha = bestEval;
                flag = TranspositionFlag::TTF_CUT_EXACT;
            }

            if (beta <= alpha) {
                config::TT_Policy::update(
                    m_transpositionTable,
                    currentPos.hash(),
                    bestMove,
                    context.gameHistory.age,
                    beta,
                    ply,
                    depth,
                    TranspositionFlag::TTF_CUT_BETA);
                
                // if (prioratized.move.isCapture() == false)
                //     pushKillerMove(prioratized.move, ply);

                //putHistoryHeuristic(static_cast<u8>(context.gameState.whiteToMove), prioratized.move.source(), prioratized.move.target(), depth);
                return { .score = bestEval, .move = bestMove };
            }
        }

        
        prioratized = generator.generateNextMove();
    } while (prioratized.move.isNull() == false);

    config::TT_Policy::update(
        m_transpositionTable,
        currentPos.hash(),
        bestMove,
        context.gameHistory.age,
        bestEval,
        ply,
        depth,
        flag);    

    return { .score = bestEval, .move = bestMove };
}

template<Set us, typename config>
i32 Search::recursiveQuiescenceNegamax(ThreadSearchContext& context, u32 depth, i32 alpha, i32 beta, u16 ply) {
    MoveGenParams genParams = MoveGenParams{ .moveFilter = MoveTypes::CAPTURES_ONLY };
    MoveGenerator<us> generator(context.position.read(), genParams);
    Evaluator evaluator(context.position.read());

    i32 perspective = 0;
    if constexpr (us == Set::WHITE) {
        perspective = 1;
    }
    else {
        perspective = -1;
    }

    i32 eval = evaluator.Evaluate() * perspective;
    if (eval >= beta)
        return beta;
    if (eval > alpha)
        alpha = eval;

    auto prioratized = generator.generateNextMove();

    // context.cancel() == true
    // && generator.isChecked() == false // continue as long as we're in check?

    if (prioratized.move.isNull() || ply >= c_maxSearchDepth || (depth <= 0))
        return eval;

    i32 maxEval = -c_maxScore;
    do {
        MoveExecutor executor(context.position.edit());
        MoveUndoUnit undoState;
        executor.makeMove(prioratized.move, undoState, ply);
        i32 eval = -recursiveQuiescenceNegamax<opposing_set<us>(), config>(context, depth - 1, -beta, -alpha, ply + 1);
        context.nodeCount++;
        executor.unmakeMove(undoState);

        maxEval = std::max(maxEval, eval);
        alpha = std::max(alpha, eval);

        if (beta <= alpha)
            return beta;

        prioratized = generator.generateNextMove();
    } while (prioratized.move.isNull() == false);

    return maxEval;
}

// --- Dispatcher Implementation ---
template<Set us>
SearchResult Search::dispatchSearch(ThreadSearchContext& context, SearchParameters params) {
    if (params.UseTranspositionTable) {
        return dispatchNMP<us, search_policies::TTEnabled>(context, params);
    } else {
        return dispatchNMP<us, search_policies::TTDisabled>(context, params);
    }
}


template<Set us, typename TT>
SearchResult Search::dispatchNMP(ThreadSearchContext& context, SearchParameters params) {
    if (params.UseNullMovePruning) {        
        return dispatchLMR<us, TT, search_policies::NmpEnabled>(context, params);
    } else {        
        return dispatchLMR<us, TT, search_policies::NmpDisabled>(context, params);
    }
}

template<Set us, typename TT, typename NMP>
SearchResult Search::dispatchLMR(ThreadSearchContext& context, SearchParameters params) {
    if (params.UseLateMoveReduction) {        
        return dispatchQSearch<us, TT, NMP, search_policies::LmrEnabled>(context, params);
    } else {
        return dispatchQSearch<us, TT, NMP, search_policies::LmrDisabled>(context, params);
    }
}

template<Set us, typename TT, typename NMP, typename LMR>
SearchResult Search::dispatchQSearch(ThreadSearchContext& context, SearchParameters params)
{
    if (params.UseQuiescenceSearch) {
        
        return dispatchDebug<us, TT, NMP, LMR, search_policies::QSearchEnabled>(context, params);
    } else {
        return dispatchDebug<us, TT, NMP, LMR, search_policies::QSearchDisabled>(context, params);
    }
}

template<Set us, typename TT, typename NMP, typename LMR, typename QSearch>
SearchResult Search::dispatchDebug(ThreadSearchContext& context, SearchParameters params) {    
#if defined(DEVELOPMENT_BUILD)
    using Config = SearchConfig<TT, NMP, LMR, QSearch, search_policies::DebugEnabled>;
    return runSearchWithConfig<us, Config>(context, params);
#elif
    using Config = SearchConfig<TT, NMP, LMR, QSearch, search_policies::DebugDisabled>;
    return runSearchWithConfig<us, Config>(context, params);
#endif
}

// // Level 4: Choose Ordering policy
// template<typename TT, typename LMR, typename NMP>
// SearchResult Search::dispatchOrdering(ThreadSearchContext& context, SearchParameters params) {
//     if (m_useMoveOrdering) {
//         return dispatchQSearch<TT, LMR, NMP, OrderingEnabledPolicy>(context, params);
//     } else {
//         return dispatchQSearch<TT, LMR, NMP, OrderingDisabledPolicy>(context, params);
//     }
// }

// // Level 5: Choose Quiescence Search policy
// template<typename TT, typename LMR, typename NMP, typename Ordering>
// SearchResult Search::dispatchQSearch(ThreadSearchContext& context, SearchParameters params) {
//     if (m_useQuiescenceSearch) {
//         using Config = SearchConfig<TT, LMR, NMP, Ordering, QSearchEnabledPolicy>;
//         return runSearchWithConfig<Config>(context, params);
//     } else {
//         using Config = SearchConfig<TT, LMR, NMP, Ordering, QSearchDisabledPolicy>;
//         return runSearchWithConfig<Config>(context, params);
//     }
// }


// TODO: Move all template implementations into a .inl?