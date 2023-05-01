#include "move_generator.h"

#include "chessboard.h"
#include "clock.hpp"
#include "evaluator.h"
#include "game_context.h"

#include <future>
#include <limits>
#include <sstream>
#include <thread>
#include <utility>

int 
MoveGenerator::Perft(GameContext& context, int depth)
{
    if (depth == 0)
    {
        return 1;
    }

    int count = 0;

    auto moves = GeneratePossibleMoves(context);
    count = 0;
    if (depth == 1)
    {
        return (i32)moves.size();
    }
    else
    if (depth > 1)
    {
        for (auto mv : moves)
        {
            FATAL_ASSERT(context.MakeMove(mv));
            count += Perft(context, depth - 1);
            context.UnmakeMove(mv);
        }
    }

    return count;
}

MoveCount
MoveGenerator::CountMoves(const std::vector<Move>& moves, MoveCount::Predicate predicate) const
{
    MoveCount result;

    for (auto&& mv : moves)
    {
        if (!predicate(mv))
            continue;

        if ((mv.Flags & MoveFlag::Capture) == MoveFlag::Capture)
            result.Captures++;
        if ((mv.Flags & MoveFlag::Promotion) == MoveFlag::Promotion)
            result.Promotions++;
        if ((mv.Flags & MoveFlag::EnPassant) == MoveFlag::EnPassant)
            result.EnPassants++;
        if ((mv.Flags & MoveFlag::Castle) == MoveFlag::Castle)
            result.Castles++;
        if ((mv.Flags & MoveFlag::Check) == MoveFlag::Check)
            result.Checks++;
        if ((mv.Flags & MoveFlag::Checkmate) == MoveFlag::Checkmate)
        {
            result.Checks++;
            result.Checkmates++;
        }

        result.Moves++;
    }

    return result;
}

std::map<PieceKey, std::vector<Move>>
MoveGenerator::OrganizeMoves(const std::vector<Move>& moves) const
{
    std::map<PieceKey, std::vector<Move>> ret;

    for (auto&& mv : moves)
    {
        PieceKey key = { mv.Piece, Notation(mv.SourceSquare) };
        if (!ret.contains(key))
            ret.insert(std::make_pair(key, std::vector<Move>()));

        ret.at(key).push_back(mv);
    }

    return ret;
}

std::vector<Move>
MoveGenerator::GeneratePossibleMoves(const GameContext& context, bool captureMoves) const
{
    std::vector<Move> retMoves;
    auto currentSet = context.readToPlay();
    const auto& board = context.readChessboard();

    auto moves = board.GetAvailableMoves(currentSet, captureMoves);
    return moves;
}

i32 
MoveGenerator::QuiescenceSearch(GameContext& context, u32 depth, i32 alpha, i32 beta, i32 perspective, u64& count)
{
    // something that we aren't considering here is moves that put opponent in check.
    i32 value = -64000;
    
    // generate capture moves
    auto moves = GeneratePossibleMoves(context, true);
    Evaluator evaluator;

    if (moves.size() == 0 || depth == 0)
        return perspective * evaluator.Evaluate(context.readChessboard(), perspective);
    else
    {
        for (auto&& mv : moves)
        {
            context.MakeLegalMove(mv);
            i32 score = std::max(value, -QuiescenceSearch(context, depth -1, -beta, -alpha, -perspective, count));
            context.UnmakeMove(mv);

            ++count;
            
            alpha = std::max(alpha, score);

            if (alpha >= beta) break;
        }
    }

    return alpha;
}

static int evaluation_hits = 0;

SearchResult
MoveGenerator::AlphaBetaNegmax(GameContext& context, u32 depth, u32 ply, i32 alpha, i32 beta, i32 perspective, u64& count, Move* pv)
{    
    Evaluator evaluator;
    if (depth == 0)
    {
        auto tpItr = m_table.find(context.readChessboard().readHash());
        if (tpItr != m_table.end())
        {
            EvaluationEntry& entry = tpItr->second;
            evaluation_hits++;
            return { entry.score, Move() };
        }
        else
        {
            i32 score = QuiescenceSearch(context, 3, alpha, beta, perspective, count);
            m_table.emplace(context.readChessboard().readHash(), EvaluationEntry{ score });
            return { score, Move() };
        }        
    }

    i32 c_checkmateconstant = -24000;
    i32 bestScore = -64000;
    i32 oldAlpha = alpha;
    Move bestMove;

    auto moves = GeneratePossibleMoves(context);
    std::sort(moves.begin(), moves.end(), s_moveComparer); // sort all captures to be upfront
    
    if (moves.size() == 0)
    {
        if (context.readChessboard().isChecked(context.readToPlay()))
            return { c_checkmateconstant + (i32)ply, Move() }; // negative "infinity" since we're in checkmate

        return { 0, Move() }; // we're in stalemate
    }

    Move localPV[depth+1];
    localPV[depth] = Move::Invalid();

    for (auto&& mv : moves)
    {
        context.MakeLegalMove(mv);
        SearchResult result;
        result = AlphaBetaNegmax(context, depth -1, ply+1, -beta, -alpha, -perspective, count, &localPV[0]);

        i32 score = -result.score;
        context.UnmakeMove(mv);
        
        ++count;

        if (score > alpha)
        {
            alpha = score;
            bestScore = score;
            bestMove = mv;

            // copy all lower moves to pv.            
            *pv = mv;
            for (u32 i = 1; i < depth; ++i)
                pv[i] = localPV[i-1];
        }

        if (alpha >= beta) break;
    }    

    return { bestScore, bestMove };
}


Move MoveGenerator::CalculateBestMove(GameContext& context, SearchParameters params)
{
    auto& stream = std::cout; // should pass this forward from the outer uci calls or something?

    bool isWhite = context.readToPlay() == Set::WHITE;
    LOG_DEBUG() << "to play: " << (isWhite ? "White" : "Black");
    
    static const u32 c_maxSearchDepth = 5;
    u32 depth = 0;
    bool useMoveTime = false;
    u32 moveTime = 0;

    bool useTimelimits = false;

    if (params.SearchDepth != 0)
    {
        if (params.SearchDepth > c_maxSearchDepth)
            params.SearchDepth = c_maxSearchDepth;

        depth = params.SearchDepth;
    }

    if (params.MoveTime != 0)
    {
        depth = c_maxSearchDepth;
        useMoveTime = true;
        moveTime = params.MoveTime;
    }
    
    if (params.WhiteTimelimit != 0)
    {
        // strategies around using these move times.
        depth = c_maxSearchDepth;
        useTimelimits = true;
        moveTime = params.WhiteTimelimit;
    }
    
    LOG_DEBUG() << "search depth: " << depth;
    
    Clock clock;
    clock.Start();
    
    i32 alpha = -64000;
    i32 beta = 64000;
    i32 perspective = isWhite ? 1 : -1;
    u64 count = 0;
    evaluation_hits = 0;

    SearchResult bestResult;
    bestResult.score = -64000;

    for (u32 itrDepth = 1; itrDepth <= depth; ++itrDepth)
    {
        Move pv[itrDepth+1];
        pv[itrDepth] = Move::Invalid(); // null move, null termination.
        SearchResult result = AlphaBetaNegmax(context, itrDepth, 1, alpha, beta, perspective, count, &pv[0]);

        if (result.score > bestResult.score)
            bestResult = result;
            
        LOG_DEBUG() << result.move.toString() << " value: " << result.score << " at depth: " << itrDepth;

        i64 et = clock.getElapsedTime();
        i64 nps = (i64)(count) / (et / 1000.f);
        stream << "info nps " << nps << "\n";
        
        std::stringstream pvSS;
        for (i32 i = 0; i < itrDepth; ++i)
        {
            pvSS << " " << pv[i].toString();
        }

        stream << "info depth " << itrDepth << " nodes " << count << " time " << et << " pv " << pvSS.str() << "\n";

        LOG_DEBUG() << "Principal variation:" << pvSS.str();
        LOG_DEBUG() << "Evaluation Cache Hits: " << evaluation_hits;
        evaluation_hits = 0;
    }

    i64 finalTime = clock.getElapsedTime();
    LOG_INFO() << "Elapsed time: " <<  finalTime << " ms";
    LOG_INFO() << "Nodes evaluated: " << count;
    // convert to seconds
    float finalfloat = finalTime / 1000.f;
    i64 nps = (i64)(count) / finalfloat;
    LOG_INFO() << "Nodes per second: " << nps << " nps";

    return bestResult.move;
}