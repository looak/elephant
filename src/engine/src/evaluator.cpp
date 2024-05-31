#include "evaluator.h"

#include "bitboard_constants.hpp"
#include "chess_piece.h"
#include "chessboard.h"
#include "evaluator_data.h"
#include "fen_parser.h"
#include "intrinsics.hpp"
#include "move.h"
#include "move_generator.hpp"

Evaluator::Evaluator() {}

i32
Evaluator::Evaluate(const Chessboard& board, const MoveGenerator& movegen)
{
    i32 score = 0;
    score += EvaluateMaterial(board);
    LOG_DEBUG() << "Material score: " << score;
    i32 tmp = EvalutePiecePositions(board);
    score += tmp;
    LOG_DEBUG() << "Piece position score: " << tmp;
    tmp = EvaluatePawnStructure(board);
    score += tmp;
    LOG_DEBUG() << "Pawn structure score: " << tmp;

    tmp = EvaluateKingSafety(board, movegen);
    score += tmp;
    LOG_DEBUG() << "King safety score: " << tmp;
    LOG_DEBUG() << "Total score: " << score;
    LOG_DEBUG() << "Endgame Coeficient: " << board.calculateEndGameCoeficient();
    LOG_DEBUG() << "---------------------------------";
    return score;
}

i32 EvaluEvaluatePlus(const Chessboard&, const MoveGenerator&, PackedMove)
{
    return 0;
}

i32
Evaluator::EvaluateMaterial(const Chessboard& board) const
{
    const auto& material = board.readPosition().readMaterial();
    i32 score = 0;

    for (u32 pieceIndx = 0; pieceIndx < 6; pieceIndx++) {
        u32 pieceValue = ChessPieceDef::Value(pieceIndx);
        i32 whiteCount = material.read(Set::WHITE, pieceIndx).count();
        i32 blackCount = material.read(Set::BLACK, pieceIndx).count();

        score += pieceValue * whiteCount;
        score -= pieceValue * blackCount;
    }

    return score;
}

i32
Evaluator::EvaluateMove(Move) const {
    i32 score = 0;

    // if (move.isCheck())
    //     score += 200;  // arbitrary check value;
    // if (move.isPromotion())
    //     score += 400;  // arbitrary promotion value;

    return score;
}

i32
Evaluator::EvaluateKingSafety(const Chessboard& board, const MoveGenerator& movegen) const {
    const auto& material = board.readPosition().readMaterial();
    i32 score = 0;
    // evaluate pawn wall around king
    Bitboard whiteKing = material.whiteKing();
    Bitboard whitePawns = material.whitePawns();
    Bitboard whitePawnWallMask = whiteKing.shiftNorthRelative<Set::WHITE>();
    whitePawnWallMask |= whitePawnWallMask.shiftEastRelative<Set::WHITE>();
    whitePawnWallMask |= whitePawnWallMask.shiftWestRelative<Set::WHITE>();

    Bitboard whitePawnWall = whitePawns & whitePawnWallMask;
    score += whitePawnWall.count() * 25;

    Bitboard blackKing = material.blackKing();
    Bitboard blackPawns = material.blackPawns();
    Bitboard blackPawnWallMask = blackKing.shiftSouthRelative<Set::BLACK>();
    blackPawnWallMask |= blackPawnWallMask.shiftEastRelative<Set::BLACK>();
    blackPawnWallMask |= blackPawnWallMask.shiftWestRelative<Set::BLACK>();

    Bitboard blackPawnWall = blackPawns & blackPawnWallMask;
    score -= blackPawnWall.count() * 25;

    // evaluate pins and checks
    const auto& whiteThreats = movegen.readKingPinThreats<Set::WHITE>();
    Bitboard whitePins = whiteThreats.pins() & material.black();
    score -= whitePins.count() * 50;

    const auto& blackThreats = movegen.readKingPinThreats<Set::BLACK>();
    Bitboard blackPins = blackThreats.pins() & material.white();
    score += blackPins.count() * 50;

    return score;
}

i32
Evaluator::EvalutePiecePositions(const Chessboard& board) const
{
    const auto& material = board.readPosition().readMaterial();
    i32 score = 0;
    float endgameCoeficient = board.calculateEndGameCoeficient();

    Bitboard whitePawns = material.read(Set::WHITE, pawnId);
    while (whitePawns.empty() == false) {
        i32 sqr = whitePawns.popLsb();
        score += evaluator_data::pawnPositionTaperedScoreTable[sqr] * endgameCoeficient;
    }

    Bitboard blackPawns = material.read(Set::BLACK, pawnId);
    while (blackPawns.empty() == false) {
        i32 sqr = blackPawns.popLsb();
        sqr = evaluator_data::flip(sqr);
        score -= evaluator_data::pawnPositionTaperedScoreTable[sqr] * endgameCoeficient;
    }

    for (u32 pieceIndx = 1; pieceIndx < kingId; ++pieceIndx) {
        Bitboard whitePieces = material.read(Set::WHITE, pieceIndx);

        while (whitePieces.empty() == false) {
            i32 sqr = whitePieces.popLsb();
            score += evaluator_data::pestoTables[pieceIndx][sqr];
        }

        Bitboard blackPieces = material.read(Set::BLACK, pieceIndx);
        while (blackPieces.empty() == false) {
            i32 sqr = blackPieces.popLsb();
            sqr = evaluator_data::flip(sqr);
            score -= evaluator_data::pestoTables[pieceIndx][sqr];
        }
    }

    Bitboard whiteKing = material.read(Set::WHITE, kingId);
    while (whiteKing.empty() == false) {
        i32 sqr = whiteKing.popLsb();
        score += evaluator_data::kingPositionTaperedScoreTable[sqr] * endgameCoeficient;
    }

    Bitboard blackKing = material.read(Set::BLACK, kingId);
    while (blackKing.empty() == false) {
        i32 sqr = blackKing.popLsb();
        sqr = evaluator_data::flip(sqr);
        score -= evaluator_data::kingPositionTaperedScoreTable[sqr] * endgameCoeficient;
    }

    return score;
}

i32
Evaluator::EvaluatePawnStructure(const Chessboard& board)
{
    i32 result = 0;
    float egCoeficient = board.calculateEndGameCoeficient();

    Bitboard whitePawns = board.readPosition().readMaterial().whitePawns();
    Bitboard blackPawns = board.readPosition().readMaterial().blackPawns();

    for (i8 idx = 0; idx < 8; ++idx) {
        // popcnt >> 1, if we have 1 pawn this will result in 0, if we have 2 pawns, this will
        // result in 1 if we have 3 pawns this will result in 1. Maybe we should use and 2?
        result += (evaluator_data::doubledPawnScore * egCoeficient) *
            (intrinsics::popcnt(whitePawns.read() & board_constants::fileMasks[idx]) >> 1);
        result -= (evaluator_data::doubledPawnScore * egCoeficient) *
            (intrinsics::popcnt(blackPawns.read() & board_constants::fileMasks[idx]) >> 1);

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
    //             u64 opposingNeighbours = blackPawns.read() & neighbourMask;
    //             if (opposingNeighbours == 0) {
    //                 result += evaluator_data::passedPawnScore * egCoeficient;
    //             }
    //             else {
    //                 i32 pawnSqr = intrinsics::msbIndex(whitePawns.read() & board_constants::fileMasks[idx]);
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
    //             u64 opposingNeighbours = whitePawns.read() & neighbourMask;
    //             if (opposingNeighbours == 0) {
    //                 result += evaluator_data::passedPawnScore * egCoeficient;
    //             }
    //             else {
    //                 i32 pawnSqr = intrinsics::lsbIndex(blackPawns.read() & board_constants::fileMasks[idx]);
    //                 if (EvaluatePassedPawn<std::greater<i32>>(board, pawnSqr, opposingNeighbours))
    //                     result += evaluator_data::passedPawnScore * egCoeficient;
    //             }
    //         }
    //     }
    }

    return result;
}

// template<Set set>
// i32 Evaluator::EvaluatePassedPawn(const Chessboard& board) {
//     int perspective = 0;
//     if constexpr (set == Set::WHITE) {
//         perspective = 1;
//     }
//     else {
//         perspective = -1;
//     }

//     Bitboard pawns = board.readPosition().readMaterial().pawns<us>();

//     while (pawns.empty() == false) {
//         i32 pawnSqr = pawns.popLsb();
//         Bitboard pawnMask = squareMaskTable[pawnSqr];

//         pawnMask = pawnMask.shiftNorthRelative<set>();
//     }


// }

// template i32 Evaluator::EvaluatePassedPawn<Set::WHITE>(const Chessboard&);
// template i32 Evaluator::EvaluatePassedPawn<Set::BLACK>(const Chessboard&);
