#include <eval/evaluator.hpp>

#include <bitboard/bitboard_constants.hpp>
#include <bitboard/intrinsics.hpp>
#include <material/chess_piece.hpp>
#include <core/chessboard.hpp>
#include <eval/evaluator_data.hpp>
#include <io/fen_parser.hpp>
#include <move/move.hpp>
#include <move/generation/move_generator.hpp>

i16
Evaluator::Evaluate()
{
    i16 score = 0;
    i16 materialScore = EvaluateMaterial();
    score += materialScore;
//    LOG_DEBUG() << "Material score: " << score;

    i16 tmp = EvaluatePiecePositions();
    score += tmp;
    // LOG_DEBUG() << "Piece position score: " << tmp;

    tmp = EvaluatePawnStructure();
    score += tmp;
    // LOG_DEBUG() << "Pawn structure score: " << tmp;

    tmp = MopUpValue(materialScore);
    score += tmp;
    // LOG_DEBUG() << "Mop up value: " << tmp;

    // tmp = EvaluateKingSafety(); // broken
    // score += tmp;
    // LOG_DEBUG() << "King safety score: " << tmp;

    // LOG_DEBUG() << "Total score: " << score;
    // LOG_DEBUG() << "Endgame Coeficient: " << calculateEndGameCoeficient();
    // LOG_DEBUG() << "---------------------------------";

    return score;
}

i16 Evaluator::EvaluatePlus(PackedMove)
{
    return 0;
}

i16
Evaluator::EvaluateMaterial() const
{
    const auto& material = m_position.material();
    i16 score = 0;

    for (u8 pieceIndx = 0; pieceIndx < 6; pieceIndx++) {
        u16 pieceValue = piece_constants::value[pieceIndx];
        i32 whiteCount = material.read(Set::WHITE, pieceIndx).count();
        i32 blackCount = material.read(Set::BLACK, pieceIndx).count();

        score += pieceValue * whiteCount;
        score -= pieceValue * blackCount;
    }

    return score;
}

i16
Evaluator::EvaluatePiecePositions() const
{
    const auto& material = m_position.material();
    i16 score = 0;
    float endgameCoeficient = calculateEndGameCoeficient();

    Bitboard whitePawns = material.read(Set::WHITE, pawnId);
    while (whitePawns.empty() == false) {
        u32 sqr = whitePawns.popLsb();
        score += evaluator_data::pawnPositionTaperedScoreTable[sqr] * endgameCoeficient;
    }

    Bitboard blackPawns = material.read(Set::BLACK, pawnId);
    while (blackPawns.empty() == false) {
        u32 sqr = blackPawns.popLsb();
        sqr = evaluator_data::flip(sqr);
        score -= evaluator_data::pawnPositionTaperedScoreTable[sqr] * endgameCoeficient;
    }

    for (u8 pieceIndx = 1; pieceIndx < kingId; ++pieceIndx) {
        Bitboard whitePieces = material.read(Set::WHITE, pieceIndx);

        while (whitePieces.empty() == false) {
            u32 sqr = whitePieces.popLsb();
            score += evaluator_data::pestoTables[pieceIndx][sqr];
        }

        Bitboard blackPieces = material.read(Set::BLACK, pieceIndx);
        while (blackPieces.empty() == false) {
            u32 sqr = blackPieces.popLsb();
            sqr = evaluator_data::flip(sqr);
            score -= evaluator_data::pestoTables[pieceIndx][sqr];
        }
    }

    Bitboard whiteKing = material.read(Set::WHITE, kingId);
    while (whiteKing.empty() == false) {
        u32 sqr = whiteKing.popLsb();
        score += evaluator_data::kingPositionTaperedScoreTable[sqr] * endgameCoeficient;
    }

    Bitboard blackKing = material.read(Set::BLACK, kingId);
    while (blackKing.empty() == false) {
        u32 sqr = blackKing.popLsb();
        sqr = evaluator_data::flip(sqr);
        score -= evaluator_data::kingPositionTaperedScoreTable[sqr] * endgameCoeficient;
    }

    return score;
}

i16 Evaluator::EvaluatePawnStructure() const {
    i16 result = 0;
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
i16 Evaluator::EvaluatePawnManhattanDistance() const {
    i16 result = 0;
    const auto& material = m_position.material();
    Bitboard whitePawns = material.whitePawns();

    i16 distance = 0;
    while (whitePawns.empty() == false) {
        u32 whitePawnSqr = whitePawns.popLsb();
        Bitboard otherPawns = material.whitePawns();
        while (otherPawns.empty() == false) {
            u32 otherPawn = otherPawns.popLsb();
            distance += board_constants::manhattanDistances[whitePawnSqr][static_cast<size_t>(otherPawn)];
        }
    }

    result -= distance;
    distance = 0;
    Bitboard blackPawns = material.blackPawns();
    while (blackPawns.empty() == false) {
        u32 blackPawnSqr = blackPawns.popLsb();
        Bitboard otherPawns = material.blackPawns();
        while (otherPawns.empty() == false) {
            u32 otherPawn = otherPawns.popLsb();
            distance += board_constants::manhattanDistances[blackPawnSqr][static_cast<size_t>(otherPawn)];
        }
    }

    result += distance;
    return result;
}

i16 Evaluator::EvaluateKingSafety() const {
    static const i16 pawnWallFactor = 8;
    //static const i16 pinFactor = 12;
    const auto& material = m_position.material();
    i16 score = 0;
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
    // const auto& whiteThreats = movegen.readKingPinThreats<Set::WHITE>();
    // Bitboard whitePins = whiteThreats.pins() & material.black();
    // score -= whitePins.count() * pinFactor;

    // const auto& blackThreats = movegen.readKingPinThreats<Set::BLACK>();
    // Bitboard blackPins = blackThreats.pins() & material.white();
    // score += blackPins.count() * pinFactor;

    return score;
}

i16 Evaluator::MopUpValue(i16 materialScore) const {
    i16 result = 0;
    result += MopUpValue<Set::WHITE>(materialScore);
    result -= MopUpValue<Set::BLACK>(-materialScore);
    return result;
}


template<Set us>
i16 Evaluator::EvaluatePassedPawn() const {
    i16 result = 0;
    Bitboard usPawns = m_position.material().pawns<us>();
    Bitboard opPawns = m_position.material().pawns<opposing_set<us>()>();
    const size_t usIndx = static_cast<size_t>(us);

    while (usPawns.empty() == false) {
        u32 pawnSqr = usPawns.popLsb();
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
            i16 pawnProtection = EvaluatePawnProtection<us>(squareMaskTable[pawnSqr]);
            pawnProtection *= evaluator_data::guardedPassedPawnBonus;
            result += static_cast<i16>(pawnProtection);
        }
    }



    return result;
}

template i16 Evaluator::EvaluatePassedPawn<Set::WHITE>() const;
template i16 Evaluator::EvaluatePassedPawn<Set::BLACK>() const;

template<Set us>
i16 Evaluator::EvaluatePawnProtection(Bitboard pawns) const {
    auto guardedSquares = m_position.material().topology<us>().computeThreatenedSquaresPawnBulk();
    guardedSquares &= pawns; // guarded pawns
    return guardedSquares.count() * evaluator_data::guardedPawnScore;
}

template<Set us>
i16 Evaluator::MopUpValue(i16 materialScore) const {
    i16 result = 0;
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

template i16 Evaluator::MopUpValue<Set::WHITE>(i16) const;
template i16 Evaluator::MopUpValue<Set::BLACK>(i16) const;


float Evaluator::calculateEndGameCoeficient() const {
    // if (m_moveCount > 64) // if we're past 64 moves, treat the game as end game.
    //     return 1.f;

    auto material = m_position.material();

    if (material.combine().count() <= 12) // if we have less than 12 pieces on the board, treat the game as end game.
        return 1.f;

    static constexpr i16 defaultPosValueOfMaterial = piece_constants::value[0] * 16    // pawn
        + piece_constants::value[1] * 4   // knight
        + piece_constants::value[2] * 4   // bishop
        + piece_constants::value[3] * 6   // rook 6 instead of 4 here to push the coeficient towards endgame a little
        + piece_constants::value[4] * 2;  // queens

    // check if we have promoted a pawn because that will screw with this endgame coeficient
    // calculation. and probably, at the point we're looking for promotions, we're most likely in a
    // endgame already should just return 1.f

    i16 boardMaterialCombinedValue = 0;
    for (u8 index = 0; index < 5; ++index) {
        boardMaterialCombinedValue += piece_constants::value[index] * material.read<Set::WHITE>(index).count();
        boardMaterialCombinedValue += piece_constants::value[index] * material.read<Set::BLACK>(index).count();
    }

    // removed move count influence on endgame coeficient, because I don't think it's needed. This note is here
    // for future reference if we want to add it back.   
    
    return 1.f - ((float)boardMaterialCombinedValue / (float)defaultPosValueOfMaterial);
}