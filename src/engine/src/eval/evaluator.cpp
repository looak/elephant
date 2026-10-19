#include <eval/evaluator.hpp>

#include <algorithm>

#include <bitboard/bitboard_constants.hpp>
#include <eval/evaluator_data.hpp>
#include <eval/mobility.hpp>
#include <eval/passed_pawns.hpp>
#include <eval/pawn_hash.hpp>
#include <eval/pawn_structure.hpp>
#include <material/chess_piece.hpp>
#include <position/position.hpp>
#include <search/search_constants.hpp>

namespace {

// mop-up starts below this game phase, a rook & a minor piece each, and grows to all of it at 0.
constexpr i32 c_mopUpPhase = 8;

} // namespace

i16
Evaluator::Evaluate()
{
    const auto& material = m_position.material();
    if (material.blackKing().count() == 0) {
        return c_checkmateConstant;
    }
    else if (material.whiteKing().count() == 0) {
        return -c_checkmateConstant;
    }

    // the tapered terms, PeSTO's tables carry their own material values. Mop-up on top, it isn't tuned.
    i16 score = static_cast<i16>(EvaluateTapered());
    score += MopUpValue(static_cast<i16>(m_accumulator.material));
    return score;
}

i32
Evaluator::EvaluateTapered() const
{
    // material and piece square sums & game phase are kept incrementally by search, see PestoAccumulator. Mobility,
    // pawn structure (cached by the pawns when search passes a table) and passed pawn extras are counted here, added to
    // them packed and tapered once with them.
    const auto& material = m_position.material();
    const PawnInfo pawns = m_pawnHash != nullptr ? m_pawnHash->probe(material) : pawnInfo(material);
    const TaperedScore score =
        m_accumulator.psqt +
        mobilityScore(material) +
        pawns.score +
        passerScore(material, pawns);
    return evaluator_data::taper(score, m_accumulator.phase);
}

i32
Evaluator::gamePhase() const
{
    return std::min(m_accumulator.phase, evaluator_data::maxGamePhase);
}

i16
Evaluator::MopUpValue(i16 materialScore) const
{
    // ramped in by game phase instead of switched on at a threshold, a step in the evaluation would have search chase
    // or avoid the trade that crosses it: nothing from c_mopUpPhase up, all of it at 0.
    const i32 phase = gamePhase();
    if (phase >= c_mopUpPhase)
        return 0;

    const i32 bonus = MopUpValue<Set::WHITE>(materialScore) - MopUpValue<Set::BLACK>(static_cast<i16>(-materialScore));
    return static_cast<i16>(bonus * (c_mopUpPhase - phase) / c_mopUpPhase);
}

// the side more than two pawns ahead brings its king to the enemy's and to the centre, to mate where search can't
// see it. 0 for the other side.
template<Set us>
i16
Evaluator::MopUpValue(i16 materialScore) const
{
    if (materialScore <= 2 * piece_constants::value[pawnId])
        return 0;

    const auto& material = m_position.material();
    const u32 usKingSqr = material.king<us>().lsbIndex();
    const u32 opKingSqr = material.king<opposing_set<us>()>().lsbIndex();
    return static_cast<i16>((14 - board_constants::manhattanDistances[usKingSqr][opKingSqr]) * 8
                            + evaluator_data::center_bias[usKingSqr]);
}

template i16 Evaluator::MopUpValue<Set::WHITE>(i16) const;
template i16 Evaluator::MopUpValue<Set::BLACK>(i16) const;
