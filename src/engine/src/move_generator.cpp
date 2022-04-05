#include "move_generator.h"
#include "game_context.h"

MoveCount 
MoveGenerator::CountMoves(const std::vector<Move>& moves) const
{
    MoveCount result;

    for(auto mv : moves)
    {
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
    }

    result.Moves = moves.size();

    return result;
}

std::vector<Move> 
MoveGenerator::GeneratePossibleMoves(const GameContext& context) const
{
    std::vector<Move> retMoves;
    auto currentSet = context.readToPlay();
    const auto& board = context.readChessboard();

    u64 threatenedMask = board.GetThreatenedMask(ChessPiece::FlipSet(currentSet));

    auto&& itr = board.begin();
    while (itr != board.end())
    {       
        auto piece = (*itr).readPiece();
        if (piece.getSet() == currentSet && piece.getType() != PieceType::NON)
        {
            auto moves = board.GetAvailableMoves((*itr).readPosition(), piece, threatenedMask);
            retMoves.insert(retMoves.end(), moves.begin(), moves.end());
        }

        ++itr;
    }

    return retMoves;
}