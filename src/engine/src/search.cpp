#include "search.hpp"

#include "chessboard.h"
#include "clock.hpp"
#include "evaluator.h"
#include "fen_parser.h"
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

void Search::ReportSearchResult(GameContext& context, SearchResult& searchResult, u32 depth, u64 nodes, const Clock& clock) const {
    i64 et = clock.getElapsedTime();

    // build the principal variation string.
    u32 madeMoves = 0;
    std::stringstream pvSS;
    for (u32 i = 0; i < depth; ++i) {
        u64 hash = context.readChessboard().readHash();
        auto pvMove = m_transpositionTable.probe(hash);
        if (pvMove.isNull())
            break;
        context.MakeMove(pvMove);
        pvSS << " " << pvMove.toString();
        madeMoves++;
    }

    for (u32 i = 0; i < madeMoves; ++i) {
        context.UnmakeMove();
    }

    i32 checkmateDistance = c_checmkateConstant - abs((int)searchResult.score);
    checkmateDistance = abs(checkmateDistance);
    if ((u32)checkmateDistance <= depth) {
        // found checkmate within depth.
        searchResult.ForcedMate = true;
        checkmateDistance /= 2;
        std::cout << "info mate " << checkmateDistance << " depth " << depth << " nodes " << nodes
            << " time " << et << " pv" << pvSS.str() << "\n";

        return;
    }

    i32 centipawn = searchResult.score;
    std::cout << "info score cp " << centipawn << " depth " << depth
        << " nodes " << nodes << " time " << et << " pv" << pvSS.str() << "\n";
}

i32 Search::CalculateMove(GameContext& context, bool maximizingPlayer, u32 depth)
{
    i32 alpha = -c_maxScore;
    i32 beta = c_maxScore;
    u32 ply = 1;
    u64 nodeCount = 0;
    std::vector<PackedMove> pv;
    pv.resize(depth);
    auto eval = AlphaBetaNegamax(context, depth, alpha, beta, !maximizingPlayer, ply, nodeCount);

    return eval.score;
}

SearchResult Search::CalculateBestMove(GameContext& context, SearchParameters params)
{
    SearchResult result;
    for (u32 itrDepth = 1; itrDepth <= params.SearchDepth; ++itrDepth) {
        Clock clock;
        clock.Start();
        u64 nodeCount = 0;

        auto itrResult = CalculateBestMoveIterration(context, itrDepth, nodeCount);

        clock.Stop();
        ReportSearchResult(context, itrResult, itrDepth, nodeCount, clock);
        if (itrResult.ForcedMate)
            return itrResult;

        result = itrResult;
        u32 timelimit = context.readToPlay() == Set::WHITE ? params.WhiteTimelimit : params.BlackTimelimit;
        i32 timeInc = context.readToPlay() == Set::WHITE ? params.WhiteTimeIncrement : params.BlackTimeIncrement;

        // TODO;
        // for now we use Infinite as a flag to indicate that we should continue searching until we reach
        // the given depth and ignore the any time limit.
        if (params.Infinite == false && TimeManagement(clock.getElapsedTime(), timelimit, timeInc, itrDepth) == false)
            break;
    }

    return result;
}

SearchResult Search::CalculateBestMoveIterration(GameContext& context, u32 depth, u64& nodeCount) {
    bool maximizingPlayer = context.readToPlay() == Set::WHITE;

    u32 ply = 1;

    auto result = AlphaBetaNegamax(context, depth, -c_maxScore, c_maxScore, maximizingPlayer, ply, nodeCount);

    return result;
}

SearchResult Search::AlphaBetaNegamax(GameContext& context, u32 depth, i32 alpha, i32 beta, bool maximizingPlayer, u32 ply, u64& nodeCount) {
    if (depth == 0) {
        // at depth zero we start the quiet search to get a better evaluation.
        // this search will try to go as deep as possible until it finds a quiet position.
        i32 score = QuiescenceNegamax(context, 3, alpha, beta, maximizingPlayer, 1, nodeCount);
        return { .score = score, .move = PackedMove::NullMove() };
    }

    // initialize the move generator.
    MoveGenerator generator(context);
    auto move = generator.generateNextMove();

    // for transposition table, cache old alpha.
    i32 oldAlpha = alpha;

    // if there are no moves to make, we're either in checkmate or stalemate.
    if (move.isNull()) {
        if (generator.isChecked())
            return { -c_checmkateConstant + (i32)ply, PackedMove::NullMove() };  // negative "infinity" since we're in checkmate
        return { .score = 0, .move = PackedMove::NullMove() };  // we're in stalemate
    }

    i32 maxEval = -c_maxScore;
    PackedMove bestMove;

    // probe transposition table.
    if (m_transpositionTable.probe(context.readChessboard().readHash(), depth, alpha, beta, bestMove, maxEval)) {
        return { .score = maxEval, .move = bestMove };
    }

    do {
        context.MakeMove(move);
        auto result = AlphaBetaNegamax(context, depth - 1, -beta, -alpha, !maximizingPlayer, ply + 1, nodeCount);
        auto eval = -result.score;
        nodeCount++;
        context.UnmakeMove();

        if (eval > maxEval)
        {
            maxEval = eval;
            bestMove = move;
        }

        alpha = std::max(alpha, eval);

        if (beta <= alpha) {
            m_transpositionTable.store(context.readChessboard().readHash(), bestMove, depth, maxEval, TTF_CUT_BETA);
            break;
        }

        move = generator.generateNextMove();
    } while (move.isNull() == false);

    if (oldAlpha != alpha)
        m_transpositionTable.store(context.readChessboard().readHash(), bestMove, depth, maxEval, TTF_CUT_EXACT);
    else
        m_transpositionTable.store(context.readChessboard().readHash(), bestMove, depth, alpha, TTF_CUT_ALPHA);

    return { .score = maxEval, .move = bestMove };
}

i32 Search::QuiescenceNegamax(GameContext& context, u32 depth, i32 alpha, i32 beta, bool maximizingPlayer, u32 ply, u64& nodeCount) {
    MoveGenerator generator(context.readChessboard().readPosition(), context.readToPlay(), PieceType::NONE, MoveTypes::CAPTURES_ONLY);
    generator.generate();
    auto move = generator.generateNextMove();

    if (move.isNull() || ply >= c_maxSearchDepth || (depth <= 0 && generator.isChecked() == false)) {
        Evaluator evaluator;
        i32 perspective = maximizingPlayer ? 1 : -1;
        return evaluator.Evaluate(context.readChessboard(), generator) * perspective;
    }

    i32 maxEval = -c_maxScore;
    do {
        context.MakeMove(move);
        i32 eval = -QuiescenceNegamax(context, depth - 1, -beta, -alpha, !maximizingPlayer, ply + 1, nodeCount);
        nodeCount++;
        context.UnmakeMove();

        maxEval = std::max(maxEval, eval);
        alpha = std::max(alpha, eval);

        if (beta <= alpha)
            break;

        move = generator.generateNextMove();
    } while (move.isNull() == false);

    return maxEval;
}

bool Search::TimeManagement(i64 elapsedTime, i64 timeleft, i32 timeInc, u32 depth) {
    // should return false if we want to abort our search.
    // how do we manage time?
    // lots of magic numbers in here.
    const i64 c_maxTimeAllowed = timeInc + (timeleft / 24);  // at 5min this is 12 seconds.    
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

void Search::pushKillerMove(PackedMove mv, u32 ply) {
    PackedMove* movesAtPly = &m_killerMoves[0][ply];

    movesAtPly[3] = movesAtPly[2];
    movesAtPly[2] = movesAtPly[1];
    movesAtPly[1] = movesAtPly[0];
    movesAtPly[0] = mv;
}
