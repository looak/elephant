#include "move_generator.h"
#include "game_context.h"

std::vector<Move> MoveGenerator::GeneratePossibleMoves(const GameContext& context) const
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