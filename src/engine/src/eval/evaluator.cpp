#include <eval/evaluator.hpp>

#include <algorithm>

#include <bitboard/bitboard_constants.hpp>
#include <bitboard/intrinsics.hpp>
#include <material/chess_piece.hpp>
#include <core/chessboard.hpp>
#include <eval/evaluator_data.hpp>
#include <eval/king_safety.hpp>
#include <eval/mobility.hpp>
#include <eval/passed_pawns.hpp>
#include <eval/pawn_hash.hpp>
#include <eval/pawn_structure.hpp>
#include <io/fen_parser.hpp>
#include <move/move.hpp>
#include <diagnostics/assert.hpp>

i16
Evaluator::Evaluate()
{
    // search never evaluates without both kings, move generation is legal; a position set up without one can only come
    // from the cli, and mop-up would read out of bounds on its missing king.
    ASSERT_MSG(m_position.material().whiteKing().count() == 1, "Evaluate needs white's king on the board.");
    ASSERT_MSG(m_position.material().blackKing().count() == 1, "Evaluate needs black's king on the board.");

    i16 score = static_cast<i16>(EvaluateTapered());

    score += MopUpValue(static_cast<i16>(m_accumulator.material));

    return score;
}

i32
Evaluator::EvaluateTapered() const
{
    // material and piece square sums & game phase are kept incrementally by search, see PestoAccumulator. Mobility,
    // pawn structure (cached by the pawns when search passes a table), passed pawn extras, king shelter & storm and
    // king attack are counted here, added to them packed and tapered once with them.
    const auto& material = m_position.material();
    const PawnInfo pawns = m_pawnHash != nullptr ? m_pawnHash->probe(material) : pawnInfo(material);
    // mobility's pass also counts each king zone's attackers, king attack scores them.
    KingZoneAttacks onKing[2];
    const TaperedScore mobility = mobilityScore(material, onKing);
    const TaperedScore score = m_accumulator.psqt + mobility + pawns.score + passerScore(material, pawns)
                             + kingSafetyScore(material) + kingAttackScore(material, onKing);
    return evaluator_data::taper(score, m_accumulator.phase);
}

i16 Evaluator::MopUpValue(i16 materialScore) const {
    i16 result = 0;
    result += MopUpValue<Set::WHITE>(materialScore);
    result -= MopUpValue<Set::BLACK>(static_cast<i16>(-materialScore));
    return result;
}

template<Set us>
i16 Evaluator::MopUpValue(i16 materialScore) const {
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

    return checked_cast<i16>(result);
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