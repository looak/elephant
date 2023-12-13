#include "search.h"

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

static constexpr u32 c_maxSearchDepth = 8;
static constexpr i32 c_maxScore = 32000;
static constexpr i32 c_checmkateConstant = 24000;
static constexpr i32 c_pvScore = 10000;

PerftResult
Search::Perft(GameContext& context, int depth)
{
    if (depth == 0) {
        return PerftResult{};
    }

    PerftResult result;
    MoveGenerator generator(context);
    generator.generate();
    generator.forEachMove([&](const PackedMove mv) {
        context.MakeMove(mv);
        result.Nodes++;
        if (mv.isCapture())
            result.Captures++;
        if (mv.isEnPassant())
            result.EnPassants++;
        if (mv.isCastling())
            result.Castles++;
        if (mv.isPromotion())
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

std::vector<Move>
Search::GeneratePossibleMoves(const GameContext& context, bool captureMoves) const
{
    std::vector<Move> moves;
    // auto currentSet = context.readToPlay();
    // const auto& board = context.readChessboard();

    // auto moves = board.GetAvailableMoves(currentSet, captureMoves);
    return moves;
}

template<bool UseCache>
i32
Search::QuiescenceSearch(GameContext& context, u32 depth, u32 ply, i32 alpha, i32 beta, i32 perspective, u32& count)
{
    // something that we aren't considering here is moves that put opponent in check.
    i32 score = -c_maxScore;

    bool isChecked = false;  // context.readChessboard().isChecked(context.readToPlay());
    if (depth == 0 && isChecked)
        depth++;

    // generate capture moves
    auto moves = GeneratePossibleMoves(context, true);
    if (depth == 0 || moves.empty()) {
        if constexpr (UseCache) {
            auto tpItr = m_evaluationTable.find(context.readChessboard().readHash());
            if (tpItr != m_evaluationTable.end()) {
                EvaluationEntry& entry = tpItr->second;
                return entry.score;
            }
            else {
                Evaluator evaluator;

                i32 staticEval = perspective * evaluator.Evaluate(context.readChessboard(), perspective);
                m_evaluationTable.emplace(context.readChessboard().readHash(), EvaluationEntry{staticEval});
                return staticEval;
            }
        }
        else {
            Evaluator evaluator;

            i32 staticEval = perspective * evaluator.Evaluate(context.readChessboard(), perspective);
            return staticEval;
        }
    }

    for (auto&& mv : moves) {
        // context.MakeLegalMove(mv);
        score = std::max(score, -QuiescenceSearch<UseCache>(context, depth - 1, ply + 1, -beta, -alpha, -perspective, count));
        // context.UnmakeMove(mv);

        ++count;

        alpha = std::max(alpha, score);

        if (alpha >= beta)
            break;
    }

    return score;
}
#ifdef DEBUG_SEARCHING
static int evaluation_hits = 0;
static float failHigh = 0.f;
static float failHighFirst = 0.f;
#endif

template<bool UseCache>
SearchResult
Search::AlphaBetaNegmax(GameContext& context, SearchContext& searchContext, u32 depth, u32 ply, i32 alpha, i32 beta,
                        i32 perspective, std::vector<Move>& pv, u32 doNullMove)
{
    const bool isChecked = false;  // context.readChessboard().isChecked(context.readToPlay());

    if (depth == 0) {
        if (!isChecked) {
            if constexpr (UseCache) {
                Evaluator evaluator;

                auto tpItr = m_evaluationTable.find(context.readChessboard().readHash());
                if (tpItr != m_evaluationTable.end()) {
                    EvaluationEntry& entry = tpItr->second;
#ifdef DEBUG_SEARCHING
                    evaluation_hits++;
#endif
                    return {entry.score, Move()};
                }
                else {
                    i32 score = QuiescenceSearch<UseCache>(context, 5, ply, alpha, beta, perspective, searchContext.count);
                    m_evaluationTable.emplace(context.readChessboard().readHash(), EvaluationEntry{score});
                    return {score, Move()};
                }
            }
            else {
                i32 score = QuiescenceSearch<UseCache>(context, 5, ply, alpha, beta, perspective, searchContext.count);
                return {score, Move()};
            }
        }

        depth++;
    }

    i32 bestScore = -c_maxScore;
    // i32 oldAlpha = alpha;
    Move bestMove;

    //// avoid null moves in endgame positions
    float egCoefficient = context.readChessboard().calculateEndGameCoeficient();
    if (doNullMove > 0 && !isChecked && egCoefficient < .75f && depth > 2) {
        std::vector<Move> localPv(depth + 1);
        auto cpy = context;
        Move nullMove{};
        // cpy.MakeNullMove(nullMove);
        auto nullResult = AlphaBetaNegmax<UseCache>(cpy, searchContext, depth - 2, ply + 1, -beta, -beta + 1, -perspective,
                                                    localPv, doNullMove - 1);
        i32 score = -nullResult.score;
        // cpy.UnmakeNullMove(nullMove);

        if (score >= beta)
            return {beta, bestMove};
    }

    // if (m_transpositionTable.probe(context.readChessboard().readHash(), depth, alpha, beta,
    // bestScore))
    //     return { bestScore, bestMove };

    auto moves = GeneratePossibleMoves(context);

    if (moves.size() == 0) {
        if (isChecked)
            return {-c_checmkateConstant + (i32)ply, Move()};  // negative "infinity" since we're in checkmate

        return {0, Move()};  // we're in stalemate
    }

    OrderMoves(searchContext, moves, depth, ply);

    std::vector<Move> localPv(depth + 1);
    u32 localCount = 0;

    for (auto&& mv : moves) {
        // context.MakeLegalMove(mv);
        // FATAL_ASSERT(context.MakeMove(mv));
        SearchResult result;
        result = AlphaBetaNegmax<UseCache>(context, searchContext, depth - 1, ply + 1, -beta, -alpha, -perspective, localPv,
                                           doNullMove);
        i32 score = -result.score;
        // context.UnmakeMove(mv);

        ++localCount;

        if (score > alpha) {
            alpha = score;
            bestScore = score;
            bestMove = mv;

            // copy all lower moves to pv.
            pv[0] = std::move(mv);
            for (u32 i = 1; i < depth; ++i)
                pv[i] = std::move(localPv[i - 1]);
        }

        if (alpha >= beta) {
#ifdef DEBUG_SEARCHING
            if (localCount == 1)
                failHighFirst += 1.f;

            failHigh += 1.f;
#endif
            //            m_transpositionTable.store(context.readChessboard().readHash(), mv, depth,
            //            score, TTF_CUT_BETA);
            break;
        }
    }

    searchContext.count += localCount;

    // if (alpha != oldAlpha)
    //     m_transpositionTable.store(context.readChessboard().readHash(), bestMove, depth,
    //     bestScore, TTF_CUT_EXACT);
    // else
    //     m_transpositionTable.store(context.readChessboard().readHash(), Move::Invalid(), depth,
    //     bestScore, TTF_CUT_ALPHA);

    return {bestScore, bestMove};
}

SearchResult
Search::CalculateBestMove(GameContext& context, SearchParameters params)
{
    // auto& stream = std::cout;  // should pass this forward from the outer uci calls or something?

    bool isWhite = context.readToPlay() == Set::WHITE;
    LOG_DEBUG() << "to play: " << (isWhite ? "White" : "Black");

    u32 depth = c_maxSearchDepth;
    bool useMoveTime = false;
    u32 moveTime = 0;
    u32 timeIncrement = 0;

    if (params.SearchDepth != 0) {
        if (params.SearchDepth > c_maxSearchDepth)
            params.SearchDepth = c_maxSearchDepth;

        depth = params.SearchDepth;
    }

    if (params.MoveTime != 0) {
        depth = c_maxSearchDepth;
        moveTime = params.MoveTime;
        useMoveTime = true;
    }

    if (params.WhiteTimelimit != 0) {
        // strategies around using these move times.
        depth = c_maxSearchDepth;
        useMoveTime = true;

        if (isWhite) {
            moveTime = params.WhiteTimelimit;
            if (params.WhiteTimeIncrement != 0) {
                timeIncrement = params.WhiteTimeIncrement;
            }
        }
        else {
            moveTime = params.BlackTimelimit;
            if (params.BlackTimeIncrement != 0) {
                timeIncrement = params.BlackTimeIncrement;
            }
        }
    }

    if (params.Infinite) {
        depth = 100;
    }

    LOG_DEBUG() << "search depth: " << depth;

    Clock clock;
    clock.Start();

    i32 alpha = -c_maxScore;
    i32 beta = c_maxScore;
    i32 perspective = isWhite ? 1 : -1;
#ifdef DEBUG_SEARCHING
    evaluation_hits = 0;
    failHigh = 0.f;
    failHighFirst = 0.f;
#endif

    SearchResult bestResult;
    bestResult.score = -c_maxScore;

    SearchContext searchContext;
    searchContext.count = 0;
    searchContext.pv.reserve(depth);
    searchContext.killerMoves.reserve(depth);

    searchContext.pv.push_back(Move::Invalid());

    for (u32 itrDepth = 1; itrDepth <= depth; ++itrDepth) {
        std::vector<Move> localPv(itrDepth + 1);
        SearchResult result = AlphaBetaNegmax<false>(context, searchContext, itrDepth, 1, alpha, beta, perspective, localPv, 1);

        searchContext.pv = std::move(localPv);
        bestResult = result;

        u64 nps = clock.calcNodesPerSecond(searchContext.count);
        std::cout << "info nps " << nps << "\n";

        std::stringstream pvSS;
        for (u32 i = 0; i < itrDepth; ++i) {
            pvSS << " " << searchContext.pv[i].toString();
            searchContext.pv[i].Score = c_pvScore;
        }

        i64 et = clock.getElapsedTime();
        // figure out if we found a mate move order.
        i32 checkmateDistance = c_checmkateConstant - abs((int)bestResult.score);
        if ((u32)checkmateDistance <= depth) {
            // found checkmate within depth.
            if (bestResult.score < 0) {
                // we can pretty safely assume that if we find a mate against us it is forced.
                checkmateDistance = -checkmateDistance;
                bestResult.ForcedMate = true;
            }
            checkmateDistance /= 2;
            std::cout << "info mate " << checkmateDistance << " depth " << itrDepth << " nodes " << searchContext.count
                      << " time " << et << " pv" << pvSS.str() << "\n";
            break;  // don't need to search further if we found a forced mate.
        }
        else {
            float centipawn = bestResult.score / 100.f;
            std::cout << "info score cp " << std::fixed << std::setprecision(2) << centipawn << " depth " << itrDepth
                      << " nodes " << searchContext.count << " time " << et << " pv" << pvSS.str() << "\n";
        }

        if (useMoveTime != false &&
            TimeManagement(et, moveTime, timeIncrement, itrDepth, context.readMoveCount(), bestResult.score) == false)
            break;

        m_evaluationTable.clear();
    }

#ifdef DEBUG_SEARCHING
    i64 finalTime = clock.getElapsedTime();
    LOG_DEBUG() << "Elapsed time: " << finalTime << " ms";
    LOG_DEBUG() << "Nodes evaluated: " << searchContext.count;
    u64 nps = clock.calcNodesPerSecond(searchContext.count);
    LOG_DEBUG() << "Nodes per second: " << nps << " nps";
    LOG_DEBUG() << "Fail high ratio: " << failHighFirst / failHigh;
    LOG_DEBUG() << "Evaluation Cache Hits: " << evaluation_hits;

    m_transpositionTable.debugStatistics();
#endif

    return bestResult;
}

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