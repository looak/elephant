#include "move_generator.h"

#include "chessboard.h"
#include "game_context.h"

#include <sstream>
#include <utility>
#include <future>
#include <thread>

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
        return moves.size();
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

std::vector<Move> concurrentGeneratePossibleMoves(std::vector<Move> moves, Chessboard board, bool countingMoves, Set currentSet)
{
    std::vector<Move> result;
    if (countingMoves)
    {
        for (auto&& mv : moves)
        {   
            board.MakeMove(mv);

            if (!board.isChecked(currentSet))
                result.push_back(mv);

            board.UnmakeMove(mv);
        }
    }
    else
    {
        for (auto&& mv : moves)
        {   
            board.MakeMoveUnchecked(mv);

            if (!board.isChecked(currentSet))
                result.push_back(mv);

            board.UnmakeMove(mv);
        }
    }

    return result;
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