#include <eval/evaluator.hpp>

#include <algorithm>

#include <diagnostics/assert.hpp>
#include <eval/evaluator_data.hpp>
#include <eval/king_safety.hpp>
#include <eval/mobility.hpp>
#include <eval/mop_up.hpp>
#include <eval/passed_pawns.hpp>
#include <eval/pawn_hash.hpp>
#include <eval/pawn_structure.hpp>
#include <eval/pieces.hpp>
#include <position/position.hpp>

i16
Evaluator::Evaluate()
{
    // search never evaluates without both kings, move generation is legal; a position set up without one can only come
    // from the cli, and mop-up would read out of bounds on its missing king.
    ASSERT_MSG(m_position.material().whiteKing().count() == 1, "Evaluate needs white's king on the board.");
    ASSERT_MSG(m_position.material().blackKing().count() == 1, "Evaluate needs black's king on the board.");

    // mop-up isn't tapered, it's added to the tapered terms, see mop_up.hpp.
    return static_cast<i16>(EvaluateTapered() + mopUpScore(m_position.material(), m_accumulator));
}

i32
Evaluator::EvaluateTapered() const
{
    // material and piece square sums & game phase are kept incrementally by search, see PestoAccumulator. Mobility,
    // pawn structure (cached by the pawns when search passes a table), passed pawn extras, king shelter & storm, king
    // attack and the piece terms are counted here, added to them packed and tapered once with them.
    const auto& material = m_position.material();
    const PawnInfo pawns = m_pawnHash != nullptr ? m_pawnHash->probe(material) : pawnInfo(material);
    // mobility's pass also counts each king zone's attackers, king attack scores them.
    KingZoneAttacks onKing[2];
    const TaperedScore mobility = mobilityScore(material, onKing);
    const TaperedScore score = m_accumulator.psqt + mobility + pawns.score + passerScore(material, pawns)
                             + kingSafetyScore(material) + kingAttackScore(material, onKing) + pieceScore(material, pawns);
    return evaluator_data::taper(score, m_accumulator.phase);
}
