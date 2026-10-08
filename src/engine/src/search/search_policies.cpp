#include <search/search_policies.hpp>
#include <search/search_constants.hpp>
#include <search/search_heuristic_structures.hpp>
#include <search/transposition_table.hpp>

#include <bitboard/bitboard_constants.hpp>
#include <material/chess_piece.hpp>
#include <material/material_mask.hpp>
#include <move/move.hpp>
#include <move/generation/move_ordering_view.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

namespace search_policies {

namespace {
// Mate scores in search are relative to the root (-mate + ply). In the TT they're stored relative to the node, so a
// position reached at a different ply reads the right distance to mate.
i16 scoreToTT(i16 score, u16 ply) {
    if (score >= c_checkmateMinScore)
        return static_cast<i16>(score + ply);
    if (score <= -c_checkmateMinScore)
        return static_cast<i16>(score - ply);
    return score;
}

i16 scoreFromTT(i16 score, u16 ply) {
    if (score >= c_checkmateMinScore)
        return static_cast<i16>(score - ply);
    if (score <= -c_checkmateMinScore)
        return static_cast<i16>(score + ply);
    return score;
}
} // namespace

// --- Transposition Table Policies ---
void TT::assign(TranspositionTable& tt) {
    m_table = &tt;
}

std::optional<i16> TT::probe(u64 hash, u16 requiredDepth, u16 ply, i16 alpha, i16 beta, TranspositionFlag& flag, PackedMove& outMove) {
    i16 score;
    u8 depth;
    if (m_table->probe(hash, outMove, score, depth, flag) == false)
        return std::nullopt;

    if (depth < requiredDepth)
        return std::nullopt;

    score = scoreFromTT(score, ply);

    switch (flag) {
        case TTF_CUT_EXACT:
            return score;
        case TTF_CUT_BETA:
            if (score >= beta)
                return score;
            break;
        case TTF_CUT_ALPHA:
            if (score <= alpha)
                return score;
            break;
        default:
            break;
    }

    return std::nullopt;
}

bool TT::probeMove(u64 hash, PackedMove& outMove) {
    i16 dummyScore;
    u8 dummyDepth;
    TranspositionFlag dummyFlag;
    return m_table->probe(hash, outMove, dummyScore, dummyDepth, dummyFlag);
}

void TT::update(u64 hash, const PackedMove& move, i16 score, u8 depth, u16 ply, const TranspositionFlag& flag)
{
    m_table->store(hash, move, scoreToTT(score, ply), depth, flag);
}

void TT::printStats() 
{
#ifdef DEBUG_TRANSITION_TABLE
    m_table->print_stats();
#endif
}

// --- Late Move Reduction (LMR) Policies ---
namespace {
// [depth][move index], both capped at 63.
const std::array<std::array<u8, 64>, 64> s_lmrTable = [] {
    std::array<std::array<u8, 64>, 64> table{};
    for (u32 depth = 1; depth < 64; ++depth)
        for (u32 index = 1; index < 64; ++index)
            table[depth][index] = static_cast<u8>(lmr_params::base + std::log(depth) * std::log(index) / lmr_params::divisor);
    return table;
}();
} // namespace

u8 LMR::reduction(u8 depth, u16 moveIndex, bool isPV, bool quiet, bool inCheck, bool givesCheck) {
    if (depth < lmr_params::minDepth || moveIndex < lmr_params::fullDepthMoves || !quiet || inCheck || givesCheck)
        return 0;

    i32 r = s_lmrTable[std::min<u32>(depth, 63)][std::min<u32>(moveIndex, 63)];
    if (isPV)
        r--;

    // the reduced child is searched at depth - 1 - r, keep it out of quiescence.
    return static_cast<u8>(std::clamp(r, 0, depth - 2));
}

// --- Move Ordering Heuristics (Killers/History) Policies ---
bool MoveOrdering::isQuiet(PackedMove move) {
    return !move.isCapture() && !move.isPromotion();
}

void MoveOrdering::updateQuietCutoff(MoveOrderingHeuristic& heuristic, Set us, PackedMove move, std::span<const PackedMove> quietsTried, u8 depth, u16 ply) {
    // killers keep their original rule, castling isn't one.
    if (move.isQuiet())
        heuristic.killers.push(move, ply);

    const i32 bonus = std::min(history_params::bonusScale * depth * depth, history_params::maxBonus);
    heuristic.history.update(us, move, bonus);
    for (PackedMove tried : quietsTried)
        heuristic.history.update(us, tried, -bonus);
}

void MoveOrdering::prime(const MoveOrderingHeuristic& heuristic, MoveOrderingView& view, u16 ply) {
    heuristic.killers.retrieve(ply, view.killers[0], view.killers[1]);
    view.history = heuristic.history.data();
}

// --- Reverse Futility Pruning (RFP) Policies ---
bool RFP::prune(bool isPV, bool inCheck, u8 depth, i16 staticEval, i16 beta) {
    if (isPV || inCheck || depth > rfp_params::maxDepth)
        return false;
    if (beta >= c_checkmateMinScore || beta <= -c_checkmateMinScore)
        return false;

    return staticEval - rfp_params::margin * depth >= beta;
}

// --- Futility Pruning Policies ---
bool Futility::nodeIsFutile(bool isPV, bool inCheck, u8 depth, i16 staticEval, i16 alpha) {
    if (isPV || inCheck || depth > futility_params::maxDepth)
        return false;
    if (alpha >= c_checkmateMinScore || alpha <= -c_checkmateMinScore)
        return false;

    const i32 margin = futility_params::base + futility_params::perDepth * depth;
    return staticEval + margin <= alpha;
}

bool Futility::skipMove(bool futileNode, u16 moveIndex, PackedMove move, bool givesCheck) {
    return futileNode && moveIndex > 0 && MoveOrdering::isQuiet(move) && !givesCheck;
}

u8 QuiescencePolicy::maxDepth = quiescence_params::defaultMaxDepth;

// --- Quiescence Search Policies ---
namespace {
// a promotion's gain on top of anything it captures.
constexpr i32 c_promotionGain = piece_constants::value[queenId] - piece_constants::value[pawnId];

bool deltaPrune(i32 standPat, i32 gain, i16 alpha) {
    return standPat + gain + quiescence_params::deltaMargin < alpha;
}

i32 mostValuablePiece(const MaterialPositionMask& material, Set set) {
    // signed, an unsigned index would wrap past the pawn when only the king is left.
    for (i32 pieceIndx = queenId; pieceIndx >= pawnId; --pieceIndx) {
        if (material.read(set, static_cast<u8>(pieceIndx)).empty() == false)
            return piece_constants::value[pieceIndx];
    }
    return 0;
}
} // namespace

bool QuiescencePolicy::deltaPruneNode(const MaterialPositionMask& material, Set us, i16 standPat, i16 alpha) {
    const Set them = us == Set::WHITE ? Set::BLACK : Set::WHITE;
    const u64 seventhRank = board_constants::rankMasks[us == Set::WHITE ? 6 : 1];
    const bool canPromote = (material.read(us, pawnId) & seventhRank).empty() == false;

    const i32 bestGain = mostValuablePiece(material, them) + (canPromote ? c_promotionGain : 0);
    return deltaPrune(standPat, bestGain, alpha);
}

bool QuiescencePolicy::deltaPruneMove(const MaterialPositionMask& material, PackedMove move, i16 standPat, i16 alpha) {
    // an empty target is en passant or a quiet promotion, counted as a pawn, erring on the safe side.
    const ChessPiece victim = material.pieceAt(move.targetSqr());
    i32 gain = victim.isValid() ? piece_constants::value[victim.index()] : piece_constants::value[pawnId];
    if (move.isPromotion())
        gain += c_promotionGain;

    return deltaPrune(standPat, gain, alpha);
}


} // namespace search_policies