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

std::vector<std::string>
MoveGenerator::MoveAnnotations(const std::vector<Move>& moves, MoveCount::Predicate predicate) const
{
    std::vector<std::string> ret;
    // for (auto&& mv : moves)
    // {
    //     std::ostringstream notation;
    //     notation << "something";

    // }
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
MoveGenerator::QuiescenceSearch(GameContext& context, Move prevMove, u32 depth, i32 alpha, i32 beta, i32 perspective, u64& count)
{
    // something that we aren't considering here is moves that put opponent in check.
    i32 value = std::numeric_limits<i32>::min();
    
    // generate capture moves
    auto moves = GeneratePossibleMoves(context, true);
    Evaluator evaluator;

    if (moves.size() == 0)
        return perspective * evaluator.Evaluate(context.readChessboard(), prevMove, perspective);
    else
    {
        for (auto&& mv : moves)
        {
            context.MakeLegalMove(mv);
            i32 newValue = std::max(value, -QuiescenceSearch(context, mv, depth -1, -beta, -alpha, -perspective, count));            
            context.UnmakeMove(mv);

            ++count;
            
        if (newValue > value)
        {
            value = newValue;
            alpha = std::max(alpha, newValue);
        }
        
        if (alpha >= beta)
            break;
        }
    }

    return value;
}

i32 
MoveGenerator::AlphaBetaNegmax(GameContext& context, Move prevMove, u32 depth, i32 alpha, i32 beta, i32 perspective, u64& count)
{
    Evaluator evaluator;
    if (depth == 0)
    {
        if (prevMove.isCapture())
             return QuiescenceSearch(context, prevMove, 3, alpha, beta, perspective, count);
        else
            return perspective * evaluator.Evaluate(context.readChessboard(), prevMove, perspective);
    }

    i32 value = std::numeric_limits<i32>::min();   
    auto moves = GeneratePossibleMoves(context);
    
    if (moves.size() == 0)
    {
        if (context.readChessboard().isChecked(context.readToPlay()))
            return value; // negative "infinity" since we're in checkmate

        return 0; // we're in stalemate
    }

    for (auto&& mv : moves)
    {
        context.MakeLegalMove(mv);
        i32 newValue = -AlphaBetaNegmax(context, mv, depth -1, alpha, beta, -perspective, count);
        context.UnmakeMove(mv);
        
        ++count;

        if (newValue > value)
        {
            value = newValue;
            alpha = std::max(alpha, newValue);
        }
        
        if (alpha >= beta)
            break;
    }

    return value;
}

Move MoveGenerator::CalculateBestMove(GameContext& context, int depth)
{
    bool isWhite = context.readToPlay() == Set::WHITE;
    LOG_DEBUG() << "to play: " << (isWhite ? "White" : "Black");
    LOG_DEBUG() << "depth: " << depth;
    Move bestMove;
    
    i32 bestValue = std::numeric_limits<i32>::min();
    i32 alpha = std::numeric_limits<i32>::min();
    i32 beta = std::numeric_limits<i32>::max();

    Clock clock;
    clock.Start();

    u64 count = 0;
    auto moves = GeneratePossibleMoves(context);

    i32 perspective = isWhite ? 1 : -1;
    
    for (auto&& mv : moves)
    {
        context.MakeLegalMove(mv);
        i32 value = -AlphaBetaNegmax(context, mv, depth -1, std::numeric_limits<i32>::min(), -alpha, -perspective, count);
        value *= perspective;
        context.UnmakeMove(mv);
        count++;

        LOG_DEBUG() << mv.toString() << " value: " << value;

        if (value > alpha)
        {
            bestValue = value;
            bestMove = mv;
            alpha = value;
        } 
        
        if (alpha >= beta) 
            break;
    }

    i64 et = clock.getElapsedTime();
    LOG_INFO() << "Elapsed time: " <<  et << " ms";
    LOG_INFO() << "Nodes evaluated: " << count;
    // convert to seconds
    float etfloat = et / 1000.f;
    i64 nps = (i64)(count) / etfloat;
    LOG_INFO() << "Nodes per second: " << nps << " nps";

    return bestMove;
}