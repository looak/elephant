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

void Search::ReportSearchResult(SearchResult& searchResult, const std::vector<PackedMove>& pv, u32 depth, u64 nodes, const Clock& clock) const {
    i64 et = clock.getElapsedTime();

    // build the principal variation string.
    std::stringstream pvSS;
    for (u32 i = 0; i < pv.size(); ++i) {
        pvSS << " " << pv[i].toString();
    }

    i32 checkmateDistance = c_checmkateConstant - abs((int)searchResult.score);
    if ((u32)checkmateDistance <= depth) {
        // found checkmate within depth.
        if (searchResult.score < 0) {
            // we can pretty safely assume that if we find a mate against us it is forced.
            checkmateDistance = -checkmateDistance;
            searchResult.ForcedMate = true;
        }
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
    auto eval = AlphaBetaNegamax(context, depth, alpha, beta, !maximizingPlayer, ply, nodeCount, pv);

    return eval.score;
}

SearchResult Search::CalculateBestMove(GameContext& context, SearchParameters params)
{
    SearchResult result;
    for (u32 itrDepth = 1; itrDepth <= params.SearchDepth; ++itrDepth) {
        Clock clock;
        clock.Start();
        u64 nodeCount = 0;

        auto [itrResult, pv] = CalculateBestMoveIterration(context, itrDepth, nodeCount);

        clock.Stop();
        ReportSearchResult(itrResult, pv, itrDepth, nodeCount, clock);
        result = itrResult;

        u32 timelimit = context.readToPlay() == Set::WHITE ? params.WhiteTimelimit : params.BlackTimelimit;
        i32 timeInc = context.readToPlay() == Set::WHITE ? params.WhiteTimeIncrement : params.BlackTimeIncrement;

        if (TimeManagement(clock.getElapsedTime(), timelimit, timeInc, itrDepth) == false)
            break;
    }

    return result;
}

Search::ResultPair Search::CalculateBestMoveIterration(GameContext& context, u32 depth, u64& nodeCount) {
    bool maximizingPlayer = context.readToPlay() == Set::WHITE;

    u32 ply = 1;

    std::vector<PackedMove> pv;
    pv.resize(depth);

    auto result = AlphaBetaNegamax(context, depth, -c_maxScore, c_maxScore, maximizingPlayer, ply, nodeCount, pv);

    return { result, pv };
}

SearchResult Search::AlphaBetaNegamax(GameContext& context, u32 depth, i32 alpha, i32 beta, bool maximizingPlayer, u32 ply, u64& nodeCount, std::vector<PackedMove>& pv) {
    if (depth == 0) {
        // at depth zero we start the quiet search to get a better evaluation.
        // this search will try to go as deep as possible until it finds a quiet position.
        i32 score = QuiescenceNegamax(context, 3, alpha, beta, maximizingPlayer, 1, nodeCount);
        return { .score = score, .move = PackedMove::NullMove() };
    }

    // initialize the move generator.
    MoveGenerator generator(context);
    auto move = generator.generateNextMove();

    // if there are no moves to make, we're either in checkmate or stalemate.
    if (move.isNull()) {
        if (generator.isChecked())
            return { -c_checmkateConstant + (i32)ply, PackedMove::NullMove() };  // negative "infinity" since we're in checkmate
        return { .score = 0, .move = PackedMove::NullMove() };  // we're in stalemate
    }

    // local principal variation. We need to cache this and only overwrite the global pv if we find a better move.
    // other wise we'll just cache the best move at each depth which isn't necessarily the same as the predicted best move path.
    std::vector<PackedMove> localPv(depth);

    i32 maxEval = -c_maxScore;
    PackedMove bestMove;
    do {
        context.MakeMove(move);
        auto result = AlphaBetaNegamax(context, depth - 1, -beta, -alpha, !maximizingPlayer, ply + 1, nodeCount, localPv);
        auto eval = -result.score;
        nodeCount++;
        context.UnmakeMove();

        if (eval > maxEval)
        {
            maxEval = eval;
            bestMove = move;

            // copy pv to search result pv.
            pv[0] = bestMove;
            for (u32 i = 1; i < depth; ++i)
                pv[i] = localPv[i - 1];
        }

        alpha = std::max(alpha, eval);

        if (beta <= alpha)
            break;

        move = generator.generateNextMove();
    } while (move.isNull() == false);

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