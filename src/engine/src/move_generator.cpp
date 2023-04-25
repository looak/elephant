#include "move_generator.h"

#include "chessboard.h"
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
MoveGenerator::GeneratePossibleMoves(const GameContext& context) const
{
    std::vector<Move> retMoves;
    auto currentSet = context.readToPlay();
    const auto& board = context.readChessboard();

    auto moves = board.GetAvailableMoves(currentSet);
    return moves;
}

i32 
MoveGenerator::AlphaBetaNegmax(GameContext& context, Move prevMove, u32 depth, i32 alpha, i32 beta, i32 perspective)
{
    Evaluator evaluator;
    if (depth == 0 || context.GameOver())
    {
        return evaluator.Evaluate(context.readChessboard(), prevMove, perspective) * -perspective;
    }

    i32 value = std::numeric_limits<i32>::min();
   
    auto moves = GeneratePossibleMoves(context);
    for (auto&& mv : moves)
    {
        context.MakeMove(mv);

        value = std::max(value, -AlphaBetaNegmax(context, mv, depth - 1, -beta, -alpha, -perspective));

        context.UnmakeMove(mv);
        
        alpha = std::max(alpha, value);
        if (alpha > beta) break;        
    }

    return value;
}

Move MoveGenerator::CalculateBestMove(GameContext& context, int depth)
{
    bool isWhite = context.readToPlay() == Set::WHITE;
    Move bestMove;
    
    i32 bestValue = isWhite ? std::numeric_limits<i32>::min() : std::numeric_limits<i32>::max();
    i32 alpha = std::numeric_limits<i32>::min();
    i32 beta = std::numeric_limits<i32>::max();

    auto moves = GeneratePossibleMoves(context);
    for (auto&& mv : moves)
    {
        context.MakeMove(mv);
        int value = AlphaBetaNegmax(context, mv, depth - 1, -beta, -alpha, isWhite ? 1 : -1);
        context.UnmakeMove(mv);

        if (isWhite)
        {
            if (value > bestValue)
            {
                bestValue = value;
                bestMove = mv;
            }

            if (value > alpha)
                alpha = value;
        }
        else
        {
            if (value < bestValue)
            {
                bestValue = value;
                bestMove = mv;
            }

            if (value < beta)
                beta = value;
        }
    }

    std::cout << " Best value: " << bestValue << std::endl;

    return bestMove;    
}