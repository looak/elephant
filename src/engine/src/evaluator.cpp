#include "evaluator.h"

#include "chessboard.h"
#include "chess_piece.h"
#include "evaluator_data.h"
#include "move.h"

Evaluator::Evaluator()
{
    evaluator_data::init_tables();
}

i32 Evaluator::Evaluate(const Chessboard& board, Move prevMove) const
{
    i32 score = 0;
    score += EvaluateMaterial(board);
    score += EvaluateMove(prevMove);
    //score += evaluator_data::eval(board);
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
    // we assume that white is the maximizing player
    i32 maximizing = move.Piece.set() == 0/*white*/ ? 1 : -1;
    i32 score = 0;

    if (move.isCapture())
        score += (ChessPieceDef::Value(move.CapturedPiece.index())
                - ChessPieceDef::Value(move.Piece.index())) + 10; // arbitrary capture value;
    if (move.isCheck())
        score += 20; // arbitrary check value;
    if (move.isPromotion())
        score += 40; // arbitrary promotion value;
    if (move.isCheckmate())
        score += 100; // arbitrary checkmate value;
    
    return score * maximizing;
}