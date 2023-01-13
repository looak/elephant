#include "move_generator.h"
#include "game_context.h"
#include <sstream>
#include <utility>

int MoveGenerator::Perft(GameContext& context, int depth)
{
    if (depth == 0)
    {
        return 1;
    }

    int count = 0;

    auto moves = GeneratePossibleMoves(context);
    count = moves.size();

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

    for(auto&& mv : moves)
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
			result.Checkmates++;

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

    auto isChecked = board.IsInCheck(currentSet);
    u64 threatenedMask = board.GetThreatenedMask(ChessPiece::FlipSet(currentSet));

    u64 kingMask = board.GetKingMask(currentSet);

    auto boardCopy = context.copyChessboard();

    const auto& material = board.readMaterial(currentSet);
    for (u32 i = 1; i < (size_t)PieceType::NR_OF_PIECES; ++i)
    {
        ChessPiece p(currentSet, (PieceType)i);
        for (auto&& piecePos : material.getPlacementsOfPiece(p))
        {
			auto moves = board.GetAvailableMoves(piecePos, p, threatenedMask, isChecked, kingMask);
            /*retMoves.insert(retMoves.end(), moves.begin(), moves.end());
            continue;*/
            // validate our moves since in some situations we can generate illegal moves when the king is checked.
            //if (isChecked)
            {
                for (auto&& mv : moves)
                {
                    if (boardCopy.MakeMove(mv))
                    {
                        if (!boardCopy.IsInCheck(currentSet))
                            retMoves.push_back(mv);

                        boardCopy.UnmakeMove(mv);
                    }
                }
            }/*
            else
            {
                retMoves.insert(retMoves.end(), moves.begin(), moves.end());
            }*/
        }
    }
    
    return retMoves;
}