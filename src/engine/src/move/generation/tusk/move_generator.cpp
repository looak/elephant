#include <move/generation/tusk/move_generator.hpp>

#include <algorithm>
#include <array>

#include <bitboard/attacks/attacks.hpp>
#include <bitboard/bitboard_constants.hpp>
#include <bitboard/intrinsics.hpp>
#include <bitboard/rays/rays.hpp>
#include <material/material_mask.hpp>
#include <position/castling_state_info.hpp>
#include <position/en_passant_state_info.hpp>
#include <position/position.hpp>

namespace tusk {
namespace {

namespace priority {
// stages hand out moves in order, priorities only need to be comparable within a stage.
constexpr u16 pvMove = 0x7FFF;
constexpr u16 ttMove = 0x7FFE;
constexpr u16 killerMove = 0x7FFD;
constexpr u16 givesCheck = 0x4000;  // quiets, added on top of history
// history in [-max, max] is mapped onto [0, max] so it stays below givesCheck and moves that keep failing sink.
constexpr i32 historyMax = MoveOrderingView::historyMax;
} // namespace priority

constexpr u64 fileA = board_constants::fileaMask;
constexpr u64 fileH = board_constants::filehMask;

constexpr std::array<u64, 64> computeKingAttacks() {
    std::array<u64, 64> table{};
    for (u32 sqr = 0; sqr < 64; ++sqr) {
        const u64 sqrMask = 1ull << sqr;
        // east & west, masking off squares that wrapped around the board.
        u64 attacks = ((sqrMask << 1) & ~fileA) | ((sqrMask >> 1) & ~fileH);
        const u64 row = attacks | sqrMask;
        attacks |= (row << 8) | (row >> 8);
        table[sqr] = attacks;
    }
    return table;
}

constexpr std::array<u64, 64> kingAttacks = computeKingAttacks();

// squares attacked by pawns of set s.
template<Set s>
constexpr u64 pawnAttacks(u64 pawns) {
    if constexpr (s == Set::WHITE)
        return ((pawns & ~fileH) << 9) | ((pawns & ~fileA) << 7);
    else
        return ((pawns & ~fileH) >> 7) | ((pawns & ~fileA) >> 9);
}

template<Set s>
constexpr u64 pawnPush(u64 pawns) {
    if constexpr (s == Set::WHITE)
        return pawns << 8;
    else
        return pawns >> 8;
}

inline PackedMove makeMove(u32 src, u32 dst, u16 flags) {
    return PackedMove(static_cast<u16>(src | (dst << 6) | (static_cast<u32>(flags) << 12)));
}

// true when sqr is attacked by the opponent of us. Pieces in removed are ignored as attackers, used for en passant
// where the captured pawn disappears.
template<Set us>
bool isAttacked(const MaterialPositionMask& material, u32 sqr, u64 occupancy, u64 removed = 0) {
    constexpr Set op = opposing_set<us>();
    const u64 sqrMask = 1ull << sqr;
    const u64 alive = ~removed;

    // an opponent pawn attacks sqr from the squares one of our pawns on sqr would attack.
    if (pawnAttacks<us>(sqrMask) & material.pawns<op>().read() & alive)
        return true;
    if (attacks::getKnightAttacks(sqr) & material.knights<op>().read() & alive)
        return true;
    if (kingAttacks[sqr] & material.king<op>().read())
        return true;

    const u64 queens = material.queens<op>().read();
    const u64 diagonals = (material.bishops<op>().read() | queens) & alive;
    if (diagonals && (attacks::getBishopAttacks(sqr, occupancy) & diagonals))
        return true;
    const u64 orthogonals = (material.rooks<op>().read() | queens) & alive;
    if (orthogonals && (attacks::getRookAttacks(sqr, occupancy) & orthogonals))
        return true;

    return false;
}

// per piece type, the squares a piece of ours would give direct check from.
template<Set us>
std::array<u64, 6> computeCheckSquares(const MaterialPositionMask& material, u64 occupancy) {
    constexpr Set op = opposing_set<us>();
    std::array<u64, 6> result{};
    const u64 opKing = material.king<op>().read();
    if (opKing == 0)
        return result;

    const u32 opKingSqr = intrinsics::lsbIndex(opKing);
    result[pawnId] = pawnAttacks<op>(opKing);
    result[knightId] = attacks::getKnightAttacks(opKingSqr);
    result[bishopId] = attacks::getBishopAttacks(opKingSqr, occupancy);
    result[rookId] = attacks::getRookAttacks(opKingSqr, occupancy);
    result[queenId] = result[bishopId] | result[rookId];
    return result;
}

inline u8 victimAt(const MaterialPositionMask& material, u64 sqrMask) {
    for (u8 pieceId = pawnId; pieceId < kingId; ++pieceId) {
        if (material.read(pieceId).read() & sqrMask)
            return pieceId;
    }
    return pawnId; // en passant, target square is empty
}

template<Set us>
ScoredMove scoreMove(PackedMove move, u8 pieceId, const MaterialPositionMask& material,
                          const std::array<u64, 6>& checkSquares, const MoveOrderingView* ordering) {
    const u64 dstMask = 1ull << move.target();
    const u8 checkingPiece = move.isPromotion() ? static_cast<u8>(move.readPromoteToPieceType() - 1) : pieceId;
    const bool check = (checkSquares[checkingPiece] & dstMask) != 0;

    i32 score = 0;
    if (move.isCapture() || move.isPromotion()) {
        if (move.isCapture())
            score += piece_constants::value[victimAt(material, dstMask)] * 8 - pieceId;
        if (checkingPiece == queenId && move.isPromotion())
            score += piece_constants::value[queenId] * 8;
    }
    else {
        if (check)
            score += priority::givesCheck;
        if (ordering != nullptr)
            score += (std::clamp(ordering->getHistoryScore(us, move.sourceSqr(), move.targetSqr()), -priority::historyMax, priority::historyMax)
                      + priority::historyMax) / 2;
    }

    return ScoredMove::make(move, static_cast<u16>(std::max(score, 0)), check);
}

} // namespace

////////////////////////////////////////////////////////////////
// MoveGenResult

template<Set us>
MoveGenResult<us>::MoveGenResult(const MoveGenerator<us>& generator, Stage startStage) :
    m_generator(&generator),
    m_stage(startStage)
{
    if (startStage == Stage::UNORDERED)
        m_generator->generateUnordered(*this);
}

template<Set us>
PrioritizedMove MoveGenResult<us>::next() {
    return m_generator->advance(*this);
}

template<Set us>
PackedMove MoveGenResult<us>::peek() {
    PrioritizedMove move = next();
    if (move)
        --m_current; // the move stays at m_current, next() hands it out again before anything else.
    return move.move;
}

template<Set us>
void MoveGenResult<us>::pickBest() {
    u32 best = m_current;
    for (u32 i = m_current + 1; i < m_end; ++i) {
        if (m_moves[i].priority() > m_moves[best].priority())
            best = i;
    }
    if (best != m_current)
        std::swap(m_moves[best], m_moves[m_current]);
}

template class MoveGenResult<Set::WHITE>;
template class MoveGenResult<Set::BLACK>;

////////////////////////////////////////////////////////////////
// MoveGenerator

template<Set us>
MoveGenerator<us>::MoveGenerator(PositionReader position, const KingPinThreats<us>& pinThreats, const MoveGenParams& params) :
    m_position(position),
    m_pinThreats(pinThreats),
    m_params(params)
{}

template<Set us>
MoveGenResult<us> MoveGenerator<us>::generate() const {
    return MoveGenResult<us>(*this, Stage::PV_MOVE);
}

template<Set us>
MoveGenResult<us> MoveGenerator<us>::generateAll() const {
    return MoveGenResult<us>(*this, Stage::UNORDERED);
}

template<Set us>
PrioritizedMove MoveGenerator<us>::advance(MoveGenResult<us>& result) const {
    const MoveOrderingView* ordering = m_params.ordering;
    const bool capturesOnly = m_params.moveFilter == MoveTypes::CAPTURES_ONLY;
    auto accepted = [&](PackedMove move) {
        return !capturesOnly || move.isCapture() || move.isPromotion();
    };

    while (true) {
        // pending moves, either a generated batch or single pv/tt/killer moves pushed by a stage below.
        if (result.m_current < result.m_end) {
            return result.m_moves[result.m_current++].toPrioritized();
        }

        switch (result.m_stage) {
        case Stage::PV_MOVE:
            result.m_stage = Stage::TT_MOVE;
            if (ordering != nullptr && ordering->pvMove && accepted(ordering->pvMove) && isLegal(ordering->pvMove))
                result.push(ScoredMove::make(ordering->pvMove, priority::pvMove));
            break;

        case Stage::TT_MOVE:
            result.m_stage = Stage::CAPTURES_GEN;
            if (ordering != nullptr && ordering->ttMove && ordering->ttMove != ordering->pvMove
                && accepted(ordering->ttMove) && isLegal(ordering->ttMove))
                result.push(ScoredMove::make(ordering->ttMove, priority::ttMove));
            break;

        case Stage::CAPTURES_GEN:
            result.m_stage = Stage::CAPTURES;
            generateScored<GenType::CAPTURES>(result);
            break;

        case Stage::CAPTURES:
            result.m_stage = capturesOnly ? Stage::DONE : Stage::KILLERS;
            break;

        case Stage::KILLERS:
            result.m_stage = Stage::QUIETS_GEN;
            if (ordering != nullptr) {
                for (u32 i = 0; i < 2; ++i) {
                    const PackedMove killer = ordering->killers[i];
                    if (!killer || killer.isCapture() || killer.isPromotion())
                        continue;
                    if (killer == ordering->pvMove || killer == ordering->ttMove || (i == 1 && killer == ordering->killers[0]))
                        continue;
                    if (isLegal(killer))
                        result.push(ScoredMove::make(killer, priority::killerMove));
                }
            }
            break;

        case Stage::QUIETS_GEN:
            result.m_stage = Stage::QUIETS;
            generateScored<GenType::QUIETS>(result);
            break;

        case Stage::QUIETS:
        case Stage::UNORDERED:
            result.m_stage = Stage::DONE;
            break;

        case Stage::DONE:
            return { PackedMove::NullMove(), 0 };
        }
    }
}

template<Set us>
void MoveGenerator<us>::generateUnordered(MoveGenResult<us>& result) const {
    auto sink = [&result](PackedMove move, u8) {
        result.push(ScoredMove::make(move, 0));
    };
    generateMoves<GenType::ALL>(sink, ~0ull);
}

template<Set us>
template<GenType type>
void MoveGenerator<us>::generateScored(MoveGenResult<us>& result) const {
    const MaterialPositionMask& material = m_position.material();
    const std::array<u64, 6> checkSquares = computeCheckSquares<us>(material, material.combine().read());
    const MoveOrderingView* ordering = m_params.ordering;
    constexpr bool includeKillers = type == GenType::QUIETS;

    auto sink = [&](PackedMove move, u8 pieceId) {
        if (isOrderingMove(move, includeKillers))
            return;
        result.push(scoreMove<us>(move, pieceId, material, checkSquares, ordering));
    };
    const u32 begin = result.m_end;
    generateMoves<type>(sink, ~0ull);

    // insertion sort the batch by descending priority, stages are short and mostly ordered by generation.
    for (u32 i = begin + 1; i < result.m_end; ++i) {
        const ScoredMove moving = result.m_moves[i];
        u32 j = i;
        while (j > begin && result.m_moves[j - 1].priority() < moving.priority()) {
            result.m_moves[j] = result.m_moves[j - 1];
            --j;
        }
        result.m_moves[j] = moving;
    }
}

template<Set us>
bool MoveGenerator<us>::isOrderingMove(PackedMove move, bool includeKillers) const {
    const MoveOrderingView* ordering = m_params.ordering;
    if (ordering == nullptr)
        return false;
    if (move == ordering->pvMove || move == ordering->ttMove)
        return true;
    return includeKillers && (move == ordering->killers[0] || move == ordering->killers[1]);
}

template<Set us>
bool MoveGenerator<us>::isLegal(PackedMove move) const {
    if (move.isNull())
        return false;

    const u64 srcMask = 1ull << move.source();
    if ((m_position.material().combine<us>().read() & srcMask) == 0)
        return false;

    // generate only the moves of the piece on the source square and look for an exact match, flags included.
    bool found = false;
    auto sink = [&](PackedMove candidate, u8) {
        found |= candidate == move;
    };
    generateMoves<GenType::ALL>(sink, srcMask);
    return found;
}

template<Set us>
template<GenType type, typename Sink>
void MoveGenerator<us>::generateMoves(Sink& sink, u64 sources) const {
    constexpr Set op = opposing_set<us>();
    constexpr size_t usIndx = static_cast<size_t>(us);
    constexpr bool captures = type == GenType::CAPTURES || type == GenType::ALL;
    constexpr bool quiets = type == GenType::QUIETS || type == GenType::ALL;

    const MaterialPositionMask& material = m_position.material();
    const u64 usMat = material.combine<us>().read();
    const u64 opMat = material.combine<op>().read();
    const u64 occupancy = usMat | opMat;
    const u64 kingMask = material.king<us>().read();
    if (kingMask == 0)
        return;
    const u32 kingSqr = intrinsics::lsbIndex(kingMask);

    const u32 checkCount = m_pinThreats.isCheckedCount();
    const u64 checkMask = checkCount == 0 ? ~0ull : m_pinThreats.checks().read();
    const u64 pinned = m_pinThreats.pins().read() & usMat;

    // which destination squares a non king, non pawn piece may move to for this generation type.
    u64 targets = 0;
    if constexpr (captures) targets |= opMat;
    if constexpr (quiets) targets |= ~occupancy;

    // emits a move for every square in movesbb, captures flagged.
    auto emit = [&](u32 src, u64 movesbb, u8 pieceId) {
        while (movesbb) {
            const u32 dst = intrinsics::lsbIndex(movesbb);
            movesbb = intrinsics::resetLsb(movesbb);
            const u16 flags = (opMat & (1ull << dst)) ? static_cast<u16>(CAPTURES) : static_cast<u16>(QUIET_MOVES);
            sink(makeMove(src, dst, flags), pieceId);
        }
    };

    // emits queen, rook, bishop & knight promotions.
    auto emitPromotions = [&](u32 src, u32 dst, bool capture) {
        const u16 captureFlag = capture ? static_cast<u16>(CAPTURES) : 0;
        for (u8 promoteTo : { queenId, rookId, bishopId, knightId }) {
            const u16 flags = static_cast<u16>(PROMOTIONS | captureFlag | (promoteTo - 1));
            sink(makeMove(src, dst, flags), pawnId);
        }
    };

    // double check, only the king can move.
    if (checkCount < 2) {
        // pawns
        {
            const u64 promotionRank = pawn_constants::promotionRank[usIndx];
            const u64 singlePushRank = pawn_constants::baseRank[usIndx];
            u64 pawns = material.pawns<us>().read() & sources;

            while (pawns) {
                const u32 src = intrinsics::lsbIndex(pawns);
                pawns = intrinsics::resetLsb(pawns);
                const u64 srcMask = 1ull << src;

                u64 allowed = checkMask;
                if (pinned & srcMask)
                    allowed &= ray::getLine(kingSqr, src);

                const u64 single = pawnPush<us>(srcMask) & ~occupancy;
                const u64 pushes = (single | (pawnPush<us>(single & singlePushRank) & ~occupancy)) & allowed;
                const u64 attacksbb = pawnAttacks<us>(srcMask) & opMat & allowed;

                // promotions, quiet ones included, belong to the captures stage.
                if constexpr (captures) {
                    u64 promotions = (pushes | attacksbb) & promotionRank;
                    while (promotions) {
                        const u32 dst = intrinsics::lsbIndex(promotions);
                        promotions = intrinsics::resetLsb(promotions);
                        emitPromotions(src, dst, (opMat & (1ull << dst)) != 0);
                    }
                    emit(src, attacksbb & ~promotionRank, pawnId);
                }
                if constexpr (quiets) {
                    emit(src, pushes & ~promotionRank, pawnId);
                }
            }

            // en passant, rare enough that legality is verified by removing both pawns and testing the king.
            if constexpr (captures) {
                const EnPassantStateInfo enPassant = m_position.enPassant();
                if (enPassant) {
                    const u32 epSqr = static_cast<u32>(enPassant.readSquare());
                    const u64 epMask = 1ull << epSqr;
                    const u64 capturedMask = 1ull << static_cast<u32>(enPassant.readTarget());
                    u64 candidates = pawnAttacks<op>(epMask) & material.pawns<us>().read() & sources;

                    while (candidates) {
                        const u32 src = intrinsics::lsbIndex(candidates);
                        candidates = intrinsics::resetLsb(candidates);
                        const u64 occAfter = (occupancy ^ (1ull << src) ^ capturedMask) | epMask;
                        if (!isAttacked<us>(material, kingSqr, occAfter, capturedMask))
                            sink(makeMove(src, epSqr, static_cast<u16>(EN_PASSANT_CAPTURE)), pawnId);
                    }
                }
            }
        }

        // knights, bishops, rooks & queens
        auto generatePiece = [&]<u8 pieceId>() {
            u64 pieces = material.read<us>(pieceId).read() & sources;
            while (pieces) {
                const u32 src = intrinsics::lsbIndex(pieces);
                pieces = intrinsics::resetLsb(pieces);

                u64 movesbb;
                if constexpr (pieceId == knightId)
                    movesbb = attacks::getKnightAttacks(src);
                else if constexpr (pieceId == bishopId)
                    movesbb = attacks::getBishopAttacks(src, occupancy);
                else if constexpr (pieceId == rookId)
                    movesbb = attacks::getRookAttacks(src, occupancy);
                else
                    movesbb = attacks::getBishopAttacks(src, occupancy) | attacks::getRookAttacks(src, occupancy);

                movesbb &= targets & checkMask;
                if (pinned & (1ull << src))
                    movesbb &= ray::getLine(kingSqr, src);

                emit(src, movesbb, pieceId);
            }
        };
        generatePiece.template operator()<knightId>();
        generatePiece.template operator()<bishopId>();
        generatePiece.template operator()<rookId>();
        generatePiece.template operator()<queenId>();
    }

    // king
    if ((kingMask & sources) == 0)
        return;

    {
        // the king can't hide behind itself from a slider.
        const u64 occNoKing = occupancy & ~kingMask;
        u64 movesbb = kingAttacks[kingSqr] & targets;
        u64 legal = 0;
        while (movesbb) {
            const u32 dst = intrinsics::lsbIndex(movesbb);
            movesbb = intrinsics::resetLsb(movesbb);
            if (!isAttacked<us>(material, dst, occNoKing))
                legal |= 1ull << dst;
        }
        emit(kingSqr, legal, kingId);
    }

    if constexpr (quiets) {
        if (checkCount > 0)
            return;

        u8 castling = m_position.castling().read();
        if constexpr (us == Set::BLACK)
            castling = static_cast<u8>(castling >> 2);
        if ((castling & 3) == 0)
            return;

        constexpr u32 rank = us == Set::WHITE ? 0 : 56;
        // king side, f & g empty and not attacked.
        if (castling & 1) {
            const u64 path = (1ull << (rank + 5)) | (1ull << (rank + 6));
            if ((occupancy & path) == 0
                && !isAttacked<us>(material, rank + 5, occupancy)
                && !isAttacked<us>(material, rank + 6, occupancy))
                sink(makeMove(kingSqr, rank + 6, static_cast<u16>(KING_CASTLE)), kingId);
        }
        // queen side, b, c & d empty, c & d not attacked.
        if (castling & 2) {
            const u64 path = (1ull << (rank + 1)) | (1ull << (rank + 2)) | (1ull << (rank + 3));
            if ((occupancy & path) == 0
                && !isAttacked<us>(material, rank + 2, occupancy)
                && !isAttacked<us>(material, rank + 3, occupancy))
                sink(makeMove(kingSqr, rank + 2, static_cast<u16>(QUEEN_CASTLE)), kingId);
        }
    }
}

template class MoveGenerator<Set::WHITE>;
template class MoveGenerator<Set::BLACK>;

} // namespace tusk
