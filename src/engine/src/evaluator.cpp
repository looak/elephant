#include "evaluator.h"

#include "bitboard_constants.h"
#include "chess_piece.h"
#include "chessboard.h"
#include "evaluator_data.h"
#include "fen_parser.h"
#include "intrinsics.hpp"
#include "move.h"

Evaluator::Evaluator() {}

i32
Evaluator::Evaluate(const Chessboard& board, i32)
{
    i32 score = 0;
    // Set set = perspective > 0 ? Set::WHITE : Set::BLACK;

    /*if (board.isCheckmated(set))
        return 24000;
    else if (board.isStalemated(set))
        return 0;*/

    score += EvaluateMaterial(board);
    score += EvalutePiecePositions(board);

    // simple mobility evaluation
    // auto blackMoves = board.GetAvailableMoves(Set::BLACK);
    // score += -25 * blackMoves.size();

    // auto whiteMoves = board.GetAvailableMoves(Set::WHITE);
    // score += 25 * whiteMoves.size();
    score += EvaluatePawnStructure(board);

    return score;
}

i32
Evaluator::EvaluateMaterial(const Chessboard& board) const
{
    // const auto& whiteMaterial = board.readMaterial((Set)0);
    // const auto& blackMaterial = board.readMaterial((Set)1);

    // i32 score = 0;
    // for (u32 pieceIndx = 0; pieceIndx < 6; pieceIndx++) {
    //     u32 pieceValue = ChessPieceDef::Value(pieceIndx);
    //     u32 count = (u32)whiteMaterial.getPieceCount((PieceType)(pieceIndx + 1));
    //     score += pieceValue * count;

    //     count = (u32)blackMaterial.getPieceCount((PieceType)(pieceIndx + 1));
    //     score -= pieceValue * count;
    // }
    // return score;
    return 0;
}

i32
Evaluator::EvaluateMove(Move move) const
{
    i32 score = 0;

    if (move.isCheck())
        score += 200;  // arbitrary check value;
    if (move.isPromotion())
        score += 400;  // arbitrary promotion value;

    return score;
}

i32
Evaluator::EvalutePiecePositions(const Chessboard& board) const
{
    i32 score = 0;

    // for (u32 pieceIndx = 0; pieceIndx < pieceIndexMax; ++pieceIndx) {
    //     const auto& whiteMat = board.readMaterial(Set::WHITE);
    //     u64 whitePieceBitboard = whiteMat.readPieceBitboard(pieceIndx);

    //     while (whitePieceBitboard > 0) {
    //         i32 sqr = whiteMat.readNextPiece(whitePieceBitboard);
    //         score += evaluator_data::pestoTables[pieceIndx][sqr];
    //     }

    //     const auto& blackMat = board.readMaterial(Set::BLACK);
    //     u64 blackPieceBitboard = whiteMat.readPieceBitboard(pieceIndx);

    //     while (blackPieceBitboard > 0) {
    //         i32 sqr = blackMat.readNextPiece(blackPieceBitboard);
    //         score += evaluator_data::pestoTables[pieceIndx][sqr];
    //     }
    // }

    return score;
}

template<typename Comparator>
bool
Evaluator::EvaluatePassedPawn(const Chessboard&, u32 pawnSqr, u64 opponentsPawns) const
{
    // this code is wrong, in blacks case we want to use the first lsb, but in whites case we want
    // to remove all lsbs until there's only one bit left, and that is the pawn we're interested in.
    bool passed = true;
    pawnSqr = pawnSqr / 8;

    u64 opponentCopy = opponentsPawns;
    while (opponentCopy != 0) {
        i32 oppSqr = intrinsics::lsbIndex(opponentCopy);
        opponentCopy = opponentCopy & (opponentCopy - 1);

        oppSqr = oppSqr / 8;

        Comparator comp{};
        if (comp(pawnSqr, oppSqr)) {
            passed = false;
            break;
        }
    }

    return passed;
}

i32
Evaluator::EvaluatePawnStructure(const Chessboard& board)
{
    i32 result = 0;
    // float egCoeficient = board.calculateEndGameCoeficient();

    // u64 whitePawns = board.readBitboard().GetMaterial({Set::WHITE, PieceType::PAWN});
    // u64 blackPawns = board.readBitboard().GetMaterial({Set::BLACK, PieceType::PAWN});

    // for (i8 idx = 0; idx < 8; ++idx) {
    //     // popcnt >> 1, if we have 1 pawn this will result in 0, if we have 2 pawns, this will
    //     // result in 1 if we have 3 pawns this will result in 1. Maybe we should use and 2?
    //     result += (evaluator_data::doubledPawnScore * egCoeficient) *
    //               (intrinsics::popcnt(whitePawns & board_constants::fileMasks[idx]) >> 1);
    //     result -= (evaluator_data::doubledPawnScore * egCoeficient) *
    //               (intrinsics::popcnt(blackPawns & board_constants::fileMasks[idx]) >> 1);

    //     // build neighbour files mask
    //     u64 neighbourMask = 0;
    //     if (idx > 0)
    //         neighbourMask |= board_constants::fileMasks[idx - 1];
    //     if (idx < 7)
    //         neighbourMask |= board_constants::fileMasks[idx + 1];

    //     if (whitePawns & board_constants::fileMasks[idx]) {
    //         // figure out if pawn is isolated
    //         if ((whitePawns & neighbourMask) == 0) {
    //             result += evaluator_data::isolatedPawnScore * egCoeficient;
    //         }

    //         // figure out if pawn is passed
    //         if ((blackPawns & board_constants::fileMasks[idx]) == 0) {
    //             u64 opposingNeighbours = blackPawns & neighbourMask;
    //             if (opposingNeighbours == 0) {
    //                 result += evaluator_data::passedPawnScore * egCoeficient;
    //             }
    //             else {
    //                 i32 pawnSqr = intrinsics::msbIndex(whitePawns & board_constants::fileMasks[idx]);
    //                 if (EvaluatePassedPawn<std::less<i32>>(board, pawnSqr, opposingNeighbours))
    //                     result += evaluator_data::passedPawnScore * egCoeficient;
    //             }
    //         }
    //     }

    //     if (blackPawns & board_constants::fileMasks[idx]) {
    //         // figure out if pawn is isolated
    //         if (blackPawns & neighbourMask) {
    //             result += evaluator_data::isolatedPawnScore * egCoeficient;
    //         }

    //         // figure out if pawn is passed
    //         if ((whitePawns & board_constants::fileMasks[idx]) == 0) {
    //             u64 opposingNeighbours = whitePawns & neighbourMask;
    //             if (opposingNeighbours == 0) {
    //                 result += evaluator_data::passedPawnScore * egCoeficient;
    //             }
    //             else {
    //                 i32 pawnSqr = intrinsics::lsbIndex(blackPawns & board_constants::fileMasks[idx]);
    //                 if (EvaluatePassedPawn<std::greater<i32>>(board, pawnSqr, opposingNeighbours))
    //                     result += evaluator_data::passedPawnScore * egCoeficient;
    //             }
    //         }
    //     }
    // }

    return result;
}
