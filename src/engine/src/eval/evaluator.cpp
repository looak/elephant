#include <eval/evaluator.hpp>

#include <algorithm>

#include <bitboard/bitboard_constants.hpp>
#include <bitboard/intrinsics.hpp>
#include <material/chess_piece.hpp>
#include <core/chessboard.hpp>
#include <eval/evaluator_data.hpp>
#include <eval/mobility.hpp>
#include <eval/pawn_hash.hpp>
#include <eval/pawn_structure.hpp>
#include <io/fen_parser.hpp>
#include <move/move.hpp>
#include <search/search_constants.hpp>

i16
Evaluator::Evaluate()
{
    auto material = m_position.material(); 
    if (material.blackKing().count() == 0) {
        return c_checkmateConstant;
    }
    else if (material.whiteKing().count() == 0) {
        return -c_checkmateConstant;
    }

    // material, piece positions & mobility, PeSTO's tables carry their own material values.
    i16 score = static_cast<i16>(EvaluateTapered());
    // LOG_DEBUG() << "PeSTO score: " << score;

    // piece_constants material, only decides whether mop up applies.
    i16 materialScore = EvaluateMaterial();

    // i16 tmp = EvaluatePawnStructure();
    // score += tmp;

    i16 tmp = MopUpValue(materialScore);
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
    // piece_constants material, kept incrementally by the position.
    return static_cast<i16>(m_accumulator.material);
}

i32
Evaluator::EvaluateTapered() const
{
    // material and piece square sums & game phase are kept incrementally by search, see PestoAccumulator. Mobility and
    // pawn structure are counted here, added to them packed and tapered once with them.
    const auto& material = m_position.material();
    const TaperedScore pawns = m_pawnHash != nullptr ? m_pawnHash->score(material) : pawnScore(material);
    return evaluator_data::taper(m_accumulator.psqt + mobilityScore(material) + pawns, m_accumulator.phase);
}

i32
Evaluator::gamePhase() const
{
    return std::min(m_accumulator.phase, evaluator_data::maxGamePhase);
}

i16 Evaluator::EvaluatePawnStructure() const {
    i16 result = 0;
    const i16 doubledPawn = static_cast<i16>(evaluator_data::taper(evaluator_data::doubledPawnScore, gamePhase()));

    Bitboard whitePawns = m_position.material().whitePawns();
    Bitboard blackPawns = m_position.material().blackPawns();

    for (i8 idx = 0; idx < 8; ++idx) {
        // popcnt >> 1, if we have 1 pawn this will result in 0, if we have 2 pawns, this will
        // result in 1 if we have 3 pawns this will result in 1. Maybe we should use and 2?
        result += static_cast<i16>(doubledPawn *
            (intrinsics::popcnt(whitePawns.read() & board_constants::fileMasks[idx]) >> 1));
        result -= static_cast<i16>(doubledPawn *
            (intrinsics::popcnt(blackPawns.read() & board_constants::fileMasks[idx]) >> 1));
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

    // evaluate pins, not enabled: tusk::CheckInfo<us>(position).pinned() is the pieces of us pinned to our king,
    // score -= popcnt(CheckInfo<WHITE>.pinned()) * pinFactor and += for black.

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
        if ((pawnMask & board_constants::boundsRelativeMasks[usIndx][cardinal_constants::west]).empty())
            pawnMask |= pawnMask.shiftWestRelative<us>();
        if ((pawnMask & board_constants::boundsRelativeMasks[usIndx][cardinal_constants::east]).empty())
            pawnMask |= pawnMask.shiftEastRelative<us>();

        while ((pawnMask & board_constants::boundsRelativeMasks[usIndx][cardinal_constants::north]).empty()) {
            pawnMask |= pawnMask.shiftNorthRelative<us>();
        }

        if ((pawnMask & opPawns).empty()) {
            result += static_cast<i16>(evaluator_data::taper(evaluator_data::passedPawnScore, gamePhase()));
            i16 pawnProtection = EvaluatePawnProtection<us>(squareMaskTable[pawnSqr]);
            i16 newPawnProtection = static_cast<i16>(pawnProtection * evaluator_data::guardedPassedPawnBonus);
            result += static_cast<i16>(newPawnProtection);
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
    return static_cast<i16>(guardedSquares.count()) * evaluator_data::guardedPawnScore;
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

    // piece count & material totals are kept incrementally by search, see PestoAccumulator.
    const PestoAccumulator& pesto = m_accumulator;

    if (pesto.pieceCount <= 12) // if we have less than 12 pieces on the board, treat the game as end game.
        return 1.f;

    static constexpr i16 defaultPosValueOfMaterial = piece_constants::value[0] * 16    // pawn
        + piece_constants::value[1] * 4   // knight
        + piece_constants::value[2] * 4   // bishop
        + piece_constants::value[3] * 6   // rook 6 instead of 4 here to push the coeficient towards endgame a little
        + piece_constants::value[4] * 2;  // queens

    // check if we have promoted a pawn because that will screw with this endgame coeficient
    // calculation. and probably, at the point we're looking for promotions, we're most likely in a
    // endgame already should just return 1.f

    const i32 boardMaterialCombinedValue = pesto.nonKingMaterial;

    // removed move count influence on endgame coeficient, because I don't think it's needed. This note is here
    // for future reference if we want to add it back.   
    
    return 1.f - ((float)boardMaterialCombinedValue / (float)defaultPosValueOfMaterial);
}