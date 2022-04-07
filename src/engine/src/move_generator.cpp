#include "move_generator.h"
#include "game_context.h"

MoveCount 
MoveGenerator::CountMoves(const std::vector<Move>& moves, MoveCount::Predicate predicate) const
{
    MoveCount result;

    for(auto mv : moves)
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

std::vector<Move> 
MoveGenerator::GeneratePossibleMoves(const GameContext& context) const
{
    std::vector<Move> retMoves;
    auto currentSet = context.readToPlay();
    const auto& board = context.readChessboard();

    bool isChecked = board.Checked(currentSet);
    u64 threatenedMask = board.GetThreatenedMask(ChessPiece::FlipSet(currentSet));

    u64 kingMask = board.GetKingMask(currentSet);

    if (isChecked)
    {
        // checking if we can block the check.
        if (kingMask > 0)
            threatenedMask &= kingMask;
    }

    auto&& itr = board.begin();
    while (itr != board.end())
    {       
        auto piece = (*itr).readPiece();
        if (piece.getSet() == currentSet && piece.getType() != PieceType::NON)
        {
            bool isPinnedOrChecked = isChecked;
            u64 threatCopy = threatenedMask;
            const auto& pos = (*itr).readPosition();
            // are we pinned
            u64 sqrMask = UINT64_C(1) << pos.index();
            if (sqrMask & kingMask)
            {
                threatCopy &= kingMask;
                isPinnedOrChecked = true;
            }

            auto moves = board.GetAvailableMoves(pos, piece, threatCopy, isPinnedOrChecked);
            retMoves.insert(retMoves.end(), moves.begin(), moves.end());
        }

        ++itr;
    }

    return retMoves;
}