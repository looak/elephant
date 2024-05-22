#include "search.hpp"

#include "chessboard.h"
#include "clock.hpp"
#include "evaluator.h"
#include "game_context.h"
#include "move_generator.hpp"

#include <future>
#include <limits>
#include <sstream>
#include <thread>
#include <utility>

static constexpr u32 c_maxSearchDepth = 5;
static constexpr i32 c_maxScore = 32000;
static constexpr i32 c_checmkateConstant = 24000;
//static constexpr i32 c_pvScore = 10000;

PerftResult
Search::Perft(GameContext& context, int depth)
{
    if (depth == 0) {
        return PerftResult{};
    }

    PerftResult result;
    MoveGenerator generator(context);
    generator.generate();
    generator.forEachMove([&](const PrioratizedMove& mv) {
        context.MakeMove(mv.move);
        result.Nodes++;
        if (mv.move.isCapture())
            result.Captures++;
        if (mv.move.isEnPassant())
            result.EnPassants++;
        if (mv.move.isCastling())
            result.Castles++;
        if (mv.move.isPromotion())
            result.Promotions++;
        if (mv.isCheck())
            result.Checks++;
        // if (mv.isCheckmate())
        //     result.Checkmates++;

        result += Perft(context, depth - 1);
        context.UnmakeMove();
    });

    return result;
}

PerftResult
Search::PerftDivide(GameContext& context, int depth)
{
    if (depth == 0) {
        return PerftResult{};
    }

    PerftResult result;
    MoveGenerator generator(context);
    generator.generate();
    generator.forEachMove([&](const PrioratizedMove& mv) {
        context.MakeMove(mv.move);
        if (depth == 1) {
            result.Nodes++;
            if (mv.move.isCapture())
                result.Captures++;
            if (mv.move.isEnPassant())
                result.EnPassants++;
            if (mv.move.isCastling())
                result.Castles++;
            if (mv.move.isPromotion())
                result.Promotions++;
            if (mv.isCheck())
                result.Checks++;
            // if (mv.isCheckmate())
            //     result.Checkmates++;
        }

        result += PerftDivide(context, depth - 1);
        context.UnmakeMove();
    });

    return result;
}

std::map<PieceKey, std::vector<Move>>
Search::OrganizeMoves(const std::vector<Move>& moves) const
{
    std::map<PieceKey, std::vector<Move>> ret;

    for (auto&& mv : moves) {
        PieceKey key = {mv.Piece, Notation(mv.SourceSquare)};
        if (!ret.contains(key))
            ret.insert(std::make_pair(key, std::vector<Move>()));

        ret.at(key).push_back(mv);
    }

    return ret;
}

i32 Search::QuiescenceNegamax(GameContext& context, u32 depth, i32 alpha, i32 beta, bool maximizingPlayer, u32 ply)
{
    MoveGenerator generator(context.readChessboard().readPosition(), context.readToPlay(), PieceType::NONE, MoveTypes::CAPTURES_ONLY);
    generator.generate();
    auto move = generator.generateNextMove();

    if (move.isNull() || ply >= c_maxSearchDepth || (depth <= 0 && generator.isChecked() == false)) {
        Evaluator evaluator;
        i32 perspective = maximizingPlayer ? 1 : -1;
        return evaluator.Evaluate(context.readChessboard()) * perspective;
    }

    i32 maxEval = -c_maxScore;
    do {
        context.MakeMove(move);
        i32 eval = -QuiescenceNegamax(context, depth - 1, -beta, -alpha, !maximizingPlayer, ++ply);
        context.UnmakeMove();

        maxEval = std::max(maxEval, eval);
        alpha = std::max(alpha, eval);

        if (beta <= alpha)
            break;

        move = generator.generateNextMove();
    } while (move.isNull() == false);

    return maxEval;
}
#ifdef DEBUG_SEARCHING
static int evaluation_hits = 0;
static float failHigh = 0.f;
static float failHighFirst = 0.f;
#endif

// template<bool UseCache>
// SearchResult Search::AlphaBetaNegmax(GameContext& context, SearchContext& searchContext, u32 depth, u32 ply, i32 alpha, i32 beta, std::vector<ScoredMove>& pv, u32 doNullMove) {

//     if (depth == 0) {
//         Evaluator evaluator;
//         i32 score = evaluator.Evaluate(context.readChessboard());
//         //i32 score = -QuiescenceSearch<UseCache>(context, 3, ply, -beta, -alpha, searchContext.count);
//         return { score, PackedMove::NullMove() };
//     }

//     i32 bestScore = -c_maxScore;
//     // i32 oldAlpha = alpha;
//     PackedMove bestMove;

//     //// avoid null moves in endgame positions
//     // float egCoefficient = context.readChessboard().calculateEndGameCoeficient();
//     // if (doNullMove > 0 && !isChecked && egCoefficient < .75f && depth > 2) {
//     //     std::vector<PackedMove> localPv(depth + 1);
//     //     auto cpy = context;
//     //     Move nullMove{};
//     //     // cpy.MakeNullMove(nullMove);
//     //     auto nullResult = AlphaBetaNegmax<UseCache>(cpy, searchContext, depth - 2, ply + 1, -beta, -beta + 1, -perspective,
//     //                                                 localPv, doNullMove - 1);
//     //     i32 score = -nullResult.score;
//     //     // cpy.UnmakeNullMove(nullMove);

//     //     if (score >= beta)
//     //         return {beta, bestMove};
//     // }

//     // if (m_transpositionTable.probe(context.readChessboard().readHash(), depth, alpha, beta,
//     // bestScore))
//     //     return { bestScore, bestMove };

//     //    auto moves = GeneratePossibleMoves(context);

//     MoveGenerator generator(context);
//     auto move = generator.generateNextMove();

//     if (move.isNull()) {
//         if (generator.isChecked())
//             return {-c_checmkateConstant + (i32)ply, PackedMove::NullMove()};  // negative "infinity" since we're in checkmate

//         return {0, PackedMove::NullMove()};  // we're in stalemate
//     }

//     std::vector<ScoredMove> localPv(depth + 1);
//     u32 localCount = 0;

//     do {
//         context.MakeMove(move);

//         SearchResult result;
//         result = AlphaBetaNegmax<UseCache>(context, searchContext, depth - 1, ply + 1, -beta, -alpha, localPv, doNullMove);
//         i32 score = -result.score;
//         context.UnmakeMove();

//         ++localCount;

//         if (score > bestScore) {
//             bestScore = score;
//             bestMove = move;
//             // copy all lower moves to pv.

//             pv[0] = ScoredMove(move, score);
//             for (u32 i = 1; i < depth; ++i)
//                 pv[i] = localPv[i - 1];
//         }

//         alpha = std::max(alpha, bestScore);

//         if (alpha >= beta) {
// #ifdef DEBUG_SEARCHING
//             if (localCount == 1)
//                 failHighFirst += 1.f;

//             failHigh += 1.f;
// #endif
//             //            m_transpositionTable.store(context.readChessboard().readHash(), mv, depth,
//             //            score, TTF_CUT_BETA);
//             break;
//         }

//         move = generator.generateNextMove();
//     } while (move.isNull() == false);

//     searchContext.count += localCount;

//     // if (alpha != oldAlpha)
//     //     m_transpositionTable.store(context.readChessboard().readHash(), bestMove, depth,
//     //     bestScore, TTF_CUT_EXACT);
//     // else
//     //     m_transpositionTable.store(context.readChessboard().readHash(), Move::Invalid(), depth,
//     //     bestScore, TTF_CUT_ALPHA);

//     return {bestScore, bestMove};
// }



// SearchResult
// Search::CalculateBestMove(GameContext& context, SearchParameters params)
// {
//     // auto& stream = std::cout;  // should pass this forward from the outer uci calls or something?

//     bool isWhite = context.readToPlay() == Set::WHITE;
//     LOG_DEBUG() << "to play: " << (isWhite ? "White" : "Black");

//     u32 depth = c_maxSearchDepth;
//     bool useMoveTime = false;
//     u32 moveTime = 0;
//     u32 timeIncrement = 0;

//     if (params.SearchDepth != 0) {
//         if (params.SearchDepth > c_maxSearchDepth)
//             params.SearchDepth = c_maxSearchDepth;

//         depth = params.SearchDepth;
//     }

//     if (params.MoveTime != 0) {
//         depth = c_maxSearchDepth;
//         moveTime = params.MoveTime;
//         useMoveTime = true;
//     }

//     if (params.WhiteTimelimit != 0) {
//         // strategies around using these move times.
//         depth = c_maxSearchDepth;
//         useMoveTime = true;

//         if (isWhite) {
//             moveTime = params.WhiteTimelimit;
//             if (params.WhiteTimeIncrement != 0) {
//                 timeIncrement = params.WhiteTimeIncrement;
//             }
//         }
//         else {
//             moveTime = params.BlackTimelimit;
//             if (params.BlackTimeIncrement != 0) {
//                 timeIncrement = params.BlackTimeIncrement;
//             }
//         }
//     }

//     if (params.Infinite) {
//         depth = 100;
//     }

//     LOG_DEBUG() << "search depth: " << depth;

//     Clock clock;
//     clock.Start();

//     i32 alpha = -c_maxScore;
//     i32 beta = c_maxScore;
// #ifdef DEBUG_SEARCHING
//     evaluation_hits = 0;
//     failHigh = 0.f;
//     failHighFirst = 0.f;
// #endif

//     SearchResult bestResult;
//     bestResult.score = -c_maxScore;

//     SearchContext searchContext;
//     searchContext.count = 0;
//     searchContext.pv.reserve(depth);
//     searchContext.killerMoves.reserve(depth);

//     searchContext.pv.emplace(searchContext.pv.end(), ScoredMove(PackedMove::NullMove(), 0));

//     for (u32 itrDepth = 1; itrDepth <= depth; ++itrDepth) {
//         std::vector<ScoredMove> localPv(itrDepth + 1);
//         SearchResult result = AlphaBetaNegmax<false>(context, searchContext, itrDepth, 1, alpha, beta, localPv, 1);

//         searchContext.pv = localPv;
//         bestResult = result;

//         u64 nps = clock.calcNodesPerSecond(searchContext.count);
//         std::cout << "info nps " << nps << "\n";

//         std::stringstream pvSS;
//         for (u32 i = 0; i < itrDepth; ++i) {
//             pvSS << " " << searchContext.pv[i].move.toString();
//             searchContext.pv[i].score = c_pvScore;
//         }

//         i64 et = clock.getElapsedTime();
//         // figure out if we found a mate move order.
//         i32 checkmateDistance = c_checmkateConstant - abs((int)bestResult.score);
//         if ((u32)checkmateDistance <= depth) {
//             // found checkmate within depth.
//             if (bestResult.score < 0) {
//                 // we can pretty safely assume that if we find a mate against us it is forced.
//                 checkmateDistance = -checkmateDistance;
//                 bestResult.ForcedMate = true;
//             }
//             checkmateDistance /= 2;
//             std::cout << "info mate " << checkmateDistance << " depth " << itrDepth << " nodes " << searchContext.count
//                       << " time " << et << " pv" << pvSS.str() << "\n";
//             break;  // don't need to search further if we found a forced mate.
//         }
//         else {
//             float centipawn = bestResult.score / 100.f;
//             std::cout << "info score cp " << std::fixed << std::setprecision(2) << centipawn << " depth " << itrDepth
//                       << " nodes " << searchContext.count << " time " << et << " pv" << pvSS.str() << "\n";
//         }

//         if (useMoveTime != false &&
//             TimeManagement(et, moveTime, timeIncrement, itrDepth, context.readMoveCount(), bestResult.score) == false)
//             break;

//         m_evaluationTable.clear();
//     }

// #ifdef DEBUG_SEARCHING
//     i64 finalTime = clock.getElapsedTime();
//     LOG_DEBUG() << "Elapsed time: " << finalTime << " ms";
//     LOG_DEBUG() << "Nodes evaluated: " << searchContext.count;
//     u64 nps = clock.calcNodesPerSecond(searchContext.count);
//     LOG_DEBUG() << "Nodes per second: " << nps << " nps";
//     LOG_DEBUG() << "Fail high ratio: " << failHighFirst / failHigh;
//     LOG_DEBUG() << "Evaluation Cache Hits: " << evaluation_hits;

//     m_transpositionTable.debugStatistics();
// #endif

//     return bestResult;
// }

bool
Search::TimeManagement(i64 elapsedTime, i64 timeleft, i32, u32, u32 depth, i32)
{
    // should return false if we want to abort our search.
    // how do we manage time?
    // lots of magic numbers in here.
    const i64 c_maxTimeAllowed = (timeleft / 24);  // at 5min this is 12 seconds.
    if (elapsedTime > c_maxTimeAllowed) {
        // if score is negative we continue looking one more depth.
        // if (score < 0 && timeleft > (c_maxTimeAllowed * 6))
        //     return true;
        // else
        return false;
    }
    else {
        i64 avrgTime = elapsedTime / depth;
        avrgTime *= avrgTime;  // assume exponential time increase per depth.
        avrgTime /= 2;         // give some credit to the alpha beta search.
        if (avrgTime > c_maxTimeAllowed)
            return false;
        else
            return true;
    }

    return false;
}
struct MoveCompare {
    bool operator()(const Move& lhs, const Move& rhs) const
    {
        if (lhs.isCapture() == true && rhs.isCapture() == true) {
            return lhs.Score > rhs.Score;
        }
        else if (lhs.isCapture() == true) {
            return true;
        }

        return false;
    }
} s_moveComparer;

void
Search::OrderMoves(SearchContext&, std::vector<Move>& moves, u32, u32) const
{
    // ply = ply - 1; // 0-indexed
    //    const Move& pvMv = searchContext.pv[ply];
    for (auto& mv : moves) {
        // if (mv == pvMv)
        //     mv.Score = c_pvScore;
        // else
        if (mv.isCapture())
            mv.Score = mv.calcCaptureValue();
    }
    std::sort(moves.begin(), moves.end(), s_moveComparer);  // sort all captures to be upfront
}

SearchResult Search::CalculateBestMove(GameContext& context)
{
    bool maximizingPlayer = context.readToPlay() == Set::WHITE;
    u32 ply = 1;
    return AlphaBetaNegamax(context, 5, -c_maxScore, c_maxScore, maximizingPlayer, ply);
}

SearchResult Search::AlphaBetaNegamax(GameContext& context, u32 depth, i32 alpha, i32 beta, bool maximizingPlayer, u32 ply)
{
    if (depth == 0) {

        i32 score = QuiescenceNegamax(context, 3, alpha, beta, maximizingPlayer, 1);
        //Evaluator evaluator;
        //i32 perspective = maximizingPlayer ? 1 : -1;
        //i32 score = evaluator.Evaluate(context.readChessboard()) * perspective;
        return { score, PackedMove::NullMove() };
    }

    MoveGenerator generator(context);
    auto move = generator.generateNextMove();

    if (move.isNull()) {
        if (generator.isChecked())
            return { -c_checmkateConstant, PackedMove::NullMove() };  // negative "infinity" since we're in checkmate

        return { 0, PackedMove::NullMove() };  // we're in stalemate
    }

    i32 maxEval = -c_maxScore;
    PackedMove bestMove;
    do {
        context.MakeMove(move);
        auto result = AlphaBetaNegamax(context, depth - 1, -beta, -alpha, !maximizingPlayer, ++ply);
        auto eval = -result.score;
        context.UnmakeMove();

        if (eval > maxEval)
        {
            maxEval = eval;
            bestMove = move;
        }

        alpha = std::max(alpha, eval);

        if (beta <= alpha)
            break;

        move = generator.generateNextMove();
    } while (move.isNull() == false);

    return { maxEval, bestMove };
}

i32 Search::CalculateMove(GameContext& context, bool maximizingPlayer, u32 depth)
{
    i32 alpha = -c_maxScore;
    i32 beta = c_maxScore;
    u32 ply = 1;
    auto eval = AlphaBetaNegamax(context, depth, alpha, beta, !maximizingPlayer, ply);

    return eval.score;
}