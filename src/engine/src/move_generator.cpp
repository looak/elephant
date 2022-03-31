#include "move_generator.h"
#include "game_context.h"

std::vector<Move> MoveGenerator::GeneratePossibleMoves(const GameContext& context) const
{
    std::vector<Move> retMoves;
    auto currentSet = context.readToPlay();
    const auto& board = context.readChessboard();

    auto&& itr = board.begin();
    while (itr != board.end())
    {       
        auto piece = (*itr).readPiece();
        if (piece.getSet() == currentSet && piece.getType() != PieceType::NON)
        {
            auto moves = board.GetAvailableMoves((*itr).readPosition(), piece);
            retMoves.insert(retMoves.end(), moves.begin(), moves.end());
        }

        ++itr;
    }

    return retMoves;
}