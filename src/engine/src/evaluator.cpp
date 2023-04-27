#include "evaluator.h"

#include "chessboard.h"
#include "chess_piece.h"
#include "evaluator_data.h"
#include "fen_parser.h"
#include "move.h"

Evaluator::Evaluator()
{
}

i32 Evaluator::Evaluate(const Chessboard& board, Move prevMove, i32 perspective) const
{
    i32 score = 0;
    score += EvaluateMaterial(board);
    // score += perspective * EvaluateMove(prevMove);
    // score += perspective * EvalutePiecePosition(prevMove);

    return score;
}

i32 Evaluator::EvaluateMaterial(const Chessboard& board) const
{
    const auto& whiteMaterial = board.readMaterial((Set)0);
    const auto& blackMaterial = board.readMaterial((Set)1);

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

i32 Evaluator::EvaluateMove(Move move) const
{    
    i32 score = 0;

    if (move.isCapture())
        score += ChessPieceDef::Value(move.CapturedPiece.index());
                //- ChessPieceDef::Value(move.Piece.index())) + 500; // arbitrary capture value;
    if (move.isCheck())
        score += 200; // arbitrary check value;
    if (move.isPromotion())
        score += 400; // arbitrary promotion value;
    if (move.isCheckmate())
        score += 1000; // arbitrary checkmate value;
    
    return score;
}

i32 Evaluator::EvalutePiecePosition(Move move) const
{
    // todo, flip tables 180 degrees for black
    return evaluator_data::pestoTables[move.Piece.index()][move.TargetSquare.index()];
}