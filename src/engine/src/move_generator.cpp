#include "move_generator.h"
#include "game_context.h"
#include <sstream>
#include <utility>

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

    auto [isChecked, checkCount] = board.IsInCheck(currentSet);
    u64 threatenedMask = board.GetThreatenedMask(ChessPiece::FlipSet(currentSet));

    u64 kingMask = board.GetKingMask(currentSet);

    auto boardCopy = context.copyChessboard();

    auto&& itr = board.begin();
    while (itr != board.end())
    {       
        const auto& piece = (*itr).readPiece();
        if (piece.isValid() && piece.getSet() == currentSet)
        {
            bool isPinnedOrChecked = isChecked;
            u64 threatCopy = threatenedMask;
            const auto& pos = (*itr).readPosition();
            // // are we pinned
            // u64 sqrMask = UINT64_C(1) << pos.index();
            // if (sqrMask & kingMask)
            // {
            //     threatCopy &= kingMask;
            //     isPinnedOrChecked = true;
            // }

            auto moves = board.GetAvailableMoves(pos, piece, threatCopy, isPinnedOrChecked, kingMask);
            
            for (auto mv : moves)
            {
                boardCopy.MakeMove(mv);
                auto [cpyChecked, cpyCount] = board.IsInCheck(currentSet);
                if (cpyChecked == false)
                    retMoves.push_back(mv);
                boardCopy.UnmakeMove(mv);
            }

            //retMoves.insert(retMoves.end(), moves.begin(), moves.end());
        }

        ++itr;
    }

    return retMoves;
}