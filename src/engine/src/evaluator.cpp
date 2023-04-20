#include "evaluator.h"

#include "chessboard.h"
//#include "evaluator_data.h"

i32 Evaluator::Evaluate(const Chessboard& board) const
{
    i32 score = 0;
    score += EvaluateMaterial(board);
    return score;
}

i32 Evaluator::EvaluateMaterial(const Chessboard& board) const
{
    const auto& whiteMaterial = board.readMaterial(Set::WHITE);
    const auto& blackMaterial = board.readMaterial(Set::BLACK);

    i32 score = 0;
    for (int pieceIndx = 0; pieceIndx < 6; pieceIndx++)
    {
        int pieceValue = ChessPieceDef::Value(pieceIndx);
        int count = whiteMaterial.getPieceCount((PieceType)pieceIndx);
        score += pieceValue * count;

        count = blackMaterial.getPieceCount((PieceType)pieceIndx);
        score -= pieceValue * count;        
    }
    return score;
}