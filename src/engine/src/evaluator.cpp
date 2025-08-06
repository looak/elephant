#include "evaluator.h"

#include "bitboard_constants.hpp"
#include <material/chess_piece.hpp>
#include "chessboard.h"
#include "evaluator_data.h"
#include <serializing/fen_parser.hpp>
#include "intrinsics.hpp"
#include <move/move.hpp>
#include <move_generation/move_generator.hpp>

i32
Evaluator::Evaluate(const MoveGenerator& movegen)
{
    i32 score = 0;
    i32 materialScore = EvaluateMaterial();
    score += materialScore;
    LOG_DEBUG() << "Material score: " << score;

    i32 tmp = EvalutePiecePositions();
    score += tmp;
    LOG_DEBUG() << "Piece position score: " << tmp;

    tmp = EvaluatePawnStructure();
    score += tmp;
    LOG_DEBUG() << "Pawn structure score: " << tmp;

    tmp = MopUpValue(materialScore);
    score += tmp;
    LOG_DEBUG() << "Mop up value: " << tmp;

    tmp = EvaluateKingSafety(movegen);
    score += tmp;
    LOG_DEBUG() << "King safety score: " << tmp;

    LOG_DEBUG() << "Total score: " << score;
    LOG_DEBUG() << "Endgame Coeficient: " << calculateEndGameCoeficient();
    LOG_DEBUG() << "---------------------------------";

    return score;
}

i32 EvaluEvaluatePlus(const Chessboard&, const MoveGenerator&, PackedMove)
{
    return 0;
}

i32
Evaluator::EvaluateMaterial() const
{
    const auto& material = m_position.material();
    i32 score = 0;

    for (u32 pieceIndx = 0; pieceIndx < 6; pieceIndx++) {
        u32 pieceValue = piece_constants::value[pieceIndx];
        i32 whiteCount = material.read(Set::WHITE, pieceIndx).count();
        i32 blackCount = material.read(Set::BLACK, pieceIndx).count();

        score += pieceValue * whiteCount;
        score -= pieceValue * blackCount;
    }

    return score;
}

i32
Evaluator::EvalutePiecePositions() const
{
    const auto& material = m_position.material();
    i32 score = 0;
    float endgameCoeficient = calculateEndGameCoeficient();

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

i32 Evaluator::EvaluatePawnStructure() const {
    i32 result = 0;
    float egCoeficient = calculateEndGameCoeficient();

    Bitboard whitePawns = m_position.material().whitePawns();
    Bitboard blackPawns = m_position.material().blackPawns();

    for (i8 idx = 0; idx < 8; ++idx) {
        // popcnt >> 1, if we have 1 pawn this will result in 0, if we have 2 pawns, this will
        // result in 1 if we have 3 pawns this will result in 1. Maybe we should use and 2?
        result += (evaluator_data::doubledPawnScore * egCoeficient) *
            (intrinsics::popcnt(whitePawns.read() & board_constants::fileMasks[idx]) >> 1);
        result -= (evaluator_data::doubledPawnScore * egCoeficient) *
            (intrinsics::popcnt(blackPawns.read() & board_constants::fileMasks[idx]) >> 1);
    }

    //result += EvaluatePawnManhattanDistance(board);
    result += EvaluatePassedPawn<Set::WHITE>();
    result -= EvaluatePassedPawn<Set::BLACK>();

    result += EvaluatePawnProtection<Set::WHITE>(whitePawns);
    result -= EvaluatePawnProtection<Set::BLACK>(blackPawns);

    return result;
}

// idea is that keeping the pawns closer together is better.
i32 Evaluator::EvaluatePawnManhattanDistance() const {
    i32 result = 0;
    const auto& material = m_position.material();
    Bitboard whitePawns = material.whitePawns();

    i32 distance = 0;
    while (whitePawns.empty() == false) {
        i32 whitePawnSqr = whitePawns.popLsb();
        Bitboard otherPawns = material.whitePawns();
        while (otherPawns.empty() == false) {
            i32 otherPawn = otherPawns.popLsb();
            distance += board_constants::manhattanDistances[whitePawnSqr][static_cast<size_t>(otherPawn)];
        }
    }

    result -= distance;
    distance = 0;
    Bitboard blackPawns = material.blackPawns();
    while (blackPawns.empty() == false) {
        i32 blackPawnSqr = blackPawns.popLsb();
        Bitboard otherPawns = material.blackPawns();
        while (otherPawns.empty() == false) {
            i32 otherPawn = otherPawns.popLsb();
            distance += board_constants::manhattanDistances[blackPawnSqr][static_cast<size_t>(otherPawn)];
        }
    }

    result += distance;
    return result;
}

i32 Evaluator::EvaluateKingSafety(const MoveGenerator& movegen) const {
    static const i32 pawnWallFactor = 8;
    static const i32 pinFactor = 12;
    const auto& material = m_position.material();
    i32 score = 0;
    // evaluate pawn wall around king
    Bitboard whiteKing = material.whiteKing();
    Bitboard whitePawns = material.whitePawns();
    Bitboard whitePawnWallMask = whiteKing.shiftNorthRelative<Set::WHITE>();
    whitePawnWallMask |= whitePawnWallMask.shiftEastRelative<Set::WHITE>();
    whitePawnWallMask |= whitePawnWallMask.shiftWestRelative<Set::WHITE>();

    Bitboard whitePawnWall = whitePawns & whitePawnWallMask;
    score += whitePawnWall.count() * pawnWallFactor;

    Bitboard blackKing = material.blackKing();
    Bitboard blackPawns = material.blackPawns();
    Bitboard blackPawnWallMask = blackKing.shiftSouthRelative<Set::BLACK>();
    blackPawnWallMask |= blackPawnWallMask.shiftEastRelative<Set::BLACK>();
    blackPawnWallMask |= blackPawnWallMask.shiftWestRelative<Set::BLACK>();

    Bitboard blackPawnWall = blackPawns & blackPawnWallMask;
    score -= blackPawnWall.count() * pawnWallFactor;

    // evaluate pins and checks
    const auto& whiteThreats = movegen.readKingPinThreats<Set::WHITE>();
    Bitboard whitePins = whiteThreats.pins() & material.black();
    score -= whitePins.count() * pinFactor;

    const auto& blackThreats = movegen.readKingPinThreats<Set::BLACK>();
    Bitboard blackPins = blackThreats.pins() & material.white();
    score += blackPins.count() * pinFactor;

    return score;
}

i32 Evaluator::MopUpValue(i32 materialScore) const {
    i32 result = 0;
    result += MopUpValue<Set::WHITE>(materialScore);
    result -= MopUpValue<Set::BLACK>(-materialScore);
    return result;
}


template<Set us>
i32 Evaluator::EvaluatePassedPawn() const {
    i32 result = 0;
    Bitboard usPawns = m_position.material().pawns<us>();
    Bitboard opPawns = m_position.material().pawns<opposing_set<us>()>();
    const size_t usIndx = static_cast<size_t>(us);

    while (usPawns.empty() == false) {
        i32 pawnSqr = usPawns.popLsb();
        Bitboard pawnMask = squareMaskTable[pawnSqr];

        pawnMask = pawnMask.shiftNorthRelative<us>();
        if ((pawnMask & board_constants::boundsRelativeMasks[usIndx][west]).empty())
            pawnMask |= pawnMask.shiftWestRelative<us>();
        if ((pawnMask & board_constants::boundsRelativeMasks[usIndx][east]).empty())
            pawnMask |= pawnMask.shiftEastRelative<us>();

        while ((pawnMask & board_constants::boundsRelativeMasks[usIndx][north]).empty()) {
            pawnMask |= pawnMask.shiftNorthRelative<us>();
        }

        if ((pawnMask & opPawns).empty()) {
            result += evaluator_data::passedPawnScore * calculateEndGameCoeficient();
            result += EvaluatePawnProtection<us>(squareMaskTable[pawnSqr]) * evaluator_data::guardedPassedPawnBonus;
        }
    }



    return result;
}

template i32 Evaluator::EvaluatePassedPawn<Set::WHITE>() const;
template i32 Evaluator::EvaluatePassedPawn<Set::BLACK>() const;

template<Set us>
i32 Evaluator::EvaluatePawnProtection(Bitboard pawns) const {
    auto guardedSquares = m_position.material().topology<us>().computeThreatenedSquaresPawnBulk();
    guardedSquares &= pawns; // guarded pawns
    return guardedSquares.count() * evaluator_data::guardedPawnScore;
}

template<Set us>
i32 Evaluator::MopUpValue(i32 materialScore) const {
    i32 result = 0;
    const auto& material = m_position.material();

    if (materialScore > 2 * piece_constants::value[pawnId] && calculateEndGameCoeficient() > 0.5f)
    {
        u32 usKingSqr = material.king<us>().lsbIndex();
        u32 opKingSqr = material.king<opposing_set<us>()>().lsbIndex();

        result += (14 - board_constants::manhattanDistances[usKingSqr][opKingSqr]) * 8;
        auto tmp = evaluator_data::center_bias[usKingSqr];
        result += tmp;
    }

    return result;
}

template i32 Evaluator::MopUpValue<Set::WHITE>(i32) const;
template i32 Evaluator::MopUpValue<Set::BLACK>(i32) const;


float Evaluator::calculateEndGameCoeficient() const {
    // if (m_moveCount > 64) // if we're past 64 moves, treat the game as end game.
    //     return 1.f;

    auto material = m_position.material();

    if (material.combine().count() <= 12) // if we have less than 12 pieces on the board, treat the game as end game.
        return 1.f;

    static constexpr i32 defaultPosValueOfMaterial = piece_constants::value[0] * 16    // pawn
        + piece_constants::value[1] * 4   // knight
        + piece_constants::value[2] * 4   // bishop
        + piece_constants::value[3] * 6   // rook 6 instead of 4 here to push the coeficient towards endgame a little
        + piece_constants::value[4] * 2;  // queens

    // check if we have promoted a pawn because that will screw with this endgame coeficient
    // calculation. and probably, at the point we're looking for promotions, we're most likely in a
    // endgame already should just return 1.f

    i32 boardMaterialCombinedValue = 0;
    for (u8 index = 0; index < 5; ++index) {
        boardMaterialCombinedValue += piece_constants::value[index] * material.read<Set::WHITE>(index).count();
        boardMaterialCombinedValue += piece_constants::value[index] * material.read<Set::BLACK>(index).count();
    }

    // removed move count influence on endgame coeficient, because I don't think it's needed. This note is here
    // for future reference if we want to add it back.   
    
    return 1.f - ((float)boardMaterialCombinedValue / (float)defaultPosValueOfMaterial);
}