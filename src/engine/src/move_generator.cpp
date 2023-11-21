#include "move_generator.hpp"
#include "game_context.h"
#include "move.h"

MoveGenerator::MoveGenerator(const GameContext& context) :
    m_context(context),
    m_moves(),
    m_movesGenerated(false)
{
    initializeMoveGenerator();
}

PackedMove
MoveGenerator::generateNextMove()
{
    if (m_moves.empty() == false) {
        auto move = m_moves.top();
        m_moves.pop();
        m_returnedMoves.push_back(move.move);
        return move.move;
    }

    if (m_movesGenerated)
        return PackedMove::NullMove();

    m_movesGenerated = true;
    if (m_context.readToPlay() == Set::WHITE) {
        return generateNextMove<Set::WHITE>();
    }
    else {
        return generateNextMove<Set::BLACK>();
    }

    return PackedMove::NullMove();
}

template<Set set>
PackedMove
MoveGenerator::generateNextMove()
{
    const size_t setIndx = static_cast<size_t>(set);
    if (m_moveMasks[setIndx].combine().empty())
        return PackedMove::NullMove();

    if (m_returnedMoves.empty()) {
        generateMoves<set, pawnId>(m_kingMask[setIndx]);
        generateMoves<set, knightId>(m_kingMask[setIndx]);
        generateMoves<set, bishopId>(m_kingMask[setIndx]);
        generateMoves<set, rookId>(m_kingMask[setIndx]);
        generateMoves<set, queenId>(m_kingMask[setIndx]);
        generateMoves<set, kingId>(m_kingMask[setIndx]);
    }

    FATAL_ASSERT(!m_moves.empty()) << "This should never be able to happen since our bitboards have moves in them.";

    auto move = m_moves.top();
    m_moves.pop();
    m_returnedMoves.push_back(move.move);
    return move.move;
}

template<Set set>
void
MoveGenerator::internalGeneratePawnMoves(const KingMask& kingMask)
{
    const auto& pos = m_context.readChessboard().readPosition();

    const Bitboard movesbb = m_moveMasks[(size_t)set].material[pawnId];
    if (movesbb.empty())
        return;

    // cache pawns in local variable which we'll use to iterate over all pawns.
    Bitboard pawns = pos.readMaterial<set>().material[pawnId];

    while (pawns.empty() == false) {
        // build source square and remove pawn from pawns bitboard.
        const i32 srcSqr = pawns.popLsb();
        const Notation srcNotation(srcSqr);

        const u64 promotionMask = pawn_constants::promotionRank[(size_t)set];

        auto [isolatedPawnMoves, isolatedPawnAttacks] = pos.isolatePiece<set, pawnId>(srcNotation, movesbb, kingMask);
        while (isolatedPawnAttacks.empty() == false) {
            i32 dstSqr = isolatedPawnAttacks.popLsb();

            PackedMove move;
            move.setSource(srcSqr);
            move.setTarget(dstSqr);

            // if we're capturing enpassant set the enpassant flag.
            if (pos.readEnPassant().readSquare() == static_cast<Square>(dstSqr))
                move.setEnPassant(true);  // sets both capture & enpassant
            else
                move.setCapture(true);

            // if we're promoting set the promotion flag and create 4 moves.
            if (promotionMask & squareMaskTable[dstSqr]) {
                move.setPromoteTo(queenId);
                PrioratizedMove prioratizedMove(move, 1);
                m_moves.push(prioratizedMove);

                move.setPromoteTo(rookId);
                PrioratizedMove prioratizedMove2(move, 1);
                m_moves.push(prioratizedMove2);

                move.setPromoteTo(bishopId);
                PrioratizedMove prioratizedMove3(move, 1);
                m_moves.push(prioratizedMove3);

                move.setPromoteTo(knightId);
                PrioratizedMove prioratizedMove4(move, 1);
                m_moves.push(prioratizedMove4);
            }
            else {
                PrioratizedMove prioratizedMove(move, 1);
                m_moves.push(prioratizedMove);
            }
        }
        while (isolatedPawnMoves.empty() == false) {
            i32 dstSqr = isolatedPawnMoves.popLsb();

            PackedMove move;
            move.setSource(srcSqr);
            move.setTarget(dstSqr);

            // if we're promoting set the promotion flag and create 4 moves.
            if (promotionMask & squareMaskTable[dstSqr]) {
                move.setPromoteTo(queenId);
                PrioratizedMove prioratizedMove(move, 1);
                m_moves.push(prioratizedMove);

                move.setPromoteTo(rookId);
                PrioratizedMove prioratizedMove2(move, 1);
                m_moves.push(prioratizedMove2);

                move.setPromoteTo(bishopId);
                PrioratizedMove prioratizedMove3(move, 1);
                m_moves.push(prioratizedMove3);

                move.setPromoteTo(knightId);
                PrioratizedMove prioratizedMove4(move, 1);
                m_moves.push(prioratizedMove4);
            }
            else {
                PrioratizedMove prioratizedMove(move, 1);
                m_moves.push(prioratizedMove);
            }
        }
    }
}
template void MoveGenerator::internalGeneratePawnMoves<Set::WHITE>(const KingMask& kingMask);
template void MoveGenerator::internalGeneratePawnMoves<Set::BLACK>(const KingMask& kingMask);

template<Set set>
void
MoveGenerator::internalGenerateKnightMoves(const KingMask& kingMask)
{
    const auto& bb = m_context.readChessboard().readPosition();

    const Bitboard movesbb = m_moveMasks[(size_t)set].material[knightId];
    if (movesbb.empty())
        return;

    Bitboard knights = bb.readMaterial<set>().material[knightId];

    while (knights.empty() == false) {
        // build source square and remove knight from cached material bitboard.
        const i32 srcSqr = knights.popLsb();
        const Notation srcNotation(srcSqr);

        auto [isolatedKnightMoves, isolatedKnightAttks] = bb.isolatePiece<set>(knightId, srcNotation, movesbb, kingMask);
        genPackedMovesFromBitboard(isolatedKnightAttks, srcSqr, /*are captures*/ true, m_moves);
        genPackedMovesFromBitboard(isolatedKnightMoves, srcSqr, /*are captures*/ false, m_moves);
    }
}

template void MoveGenerator::internalGenerateKnightMoves<Set::WHITE>(const KingMask& kingMask);
template void MoveGenerator::internalGenerateKnightMoves<Set::BLACK>(const KingMask& kingMask);

template<Set set>
void
MoveGenerator::internalGenerateBishopMoves(const KingMask& kingMask)
{
}

template void MoveGenerator::internalGenerateBishopMoves<Set::WHITE>(const KingMask& kingMask);
template void MoveGenerator::internalGenerateBishopMoves<Set::BLACK>(const KingMask& kingMask);

template<Set set>
void
MoveGenerator::internalGenerateRookMoves(const KingMask& kingMask)
{
}

template void MoveGenerator::internalGenerateRookMoves<Set::WHITE>(const KingMask& kingMask);
template void MoveGenerator::internalGenerateRookMoves<Set::BLACK>(const KingMask& kingMask);

template<Set set>
void
MoveGenerator::internalGenerateQueenMoves(const KingMask& kingMask)
{
}

template void MoveGenerator::internalGenerateQueenMoves<Set::WHITE>(const KingMask& kingMask);
template void MoveGenerator::internalGenerateQueenMoves<Set::BLACK>(const KingMask& kingMask);

template<Set set>
void
MoveGenerator::internalGenerateKingMoves(const KingMask& kingMask)
{
    const auto& bb = m_context.readChessboard().readPosition();

    Bitboard movesbb = m_moveMasks[(size_t)set].material[kingId];
#if defined EG_DEBUGGING || defined EG_TESTING
    // during testing and debugging king can be missing
    if (movesbb.empty())
        return;
#endif

    u32 srcSqr = bb.readMaterial<set>().material[kingId].lsbIndex();

    while (movesbb.empty() == false) {
        i32 dstSqr = movesbb.popLsb();

        PackedMove move;
        move.setSource(srcSqr);
        move.setTarget(dstSqr);

        PrioratizedMove prioratizedMove(move, 1);
        m_moves.push(prioratizedMove);
    }
}

template void MoveGenerator::internalGenerateKingMoves<Set::WHITE>(const KingMask& kingMask);
template void MoveGenerator::internalGenerateKingMoves<Set::BLACK>(const KingMask& kingMask);

void
MoveGenerator::initializeMoveGenerator()
{
    initializeMoveMasks<Set::WHITE>(m_moveMasks[0]);
    initializeMoveMasks<Set::BLACK>(m_moveMasks[1]);
}

template<Set set>
void
MoveGenerator::initializeMoveMasks(MaterialMask& target)
{
    const auto& bb = m_context.readChessboard().readPosition();
    const size_t setIndx = static_cast<size_t>(set);
    m_kingMask[setIndx] = bb.calcKingMask<set>();
    target.material[pawnId] = bb.calcAvailableMovesPawnBulk<set>(m_kingMask[setIndx]);
    target.material[knightId] = bb.calcAvailableMovesKnightBulk<set>(m_kingMask[setIndx]);
    target.material[bishopId] = bb.calcAvailableMovesBishopBulk<set>(m_kingMask[setIndx]);
    target.material[rookId] = bb.calcAvailableMovesRookBulk<set>(m_kingMask[setIndx]);
    target.material[queenId] = bb.calcAvailableMovesQueenBulk<set>(m_kingMask[setIndx]);
    target.material[kingId] = bb.calcAvailableMovesKing<set>(0);
}

template void MoveGenerator::initializeMoveMasks<Set::WHITE>(MaterialMask& target);
template void MoveGenerator::initializeMoveMasks<Set::BLACK>(MaterialMask& target);

void
MoveGenerator::genPackedMovesFromBitboard(Bitboard movesbb, i32 srcSqr, bool capture, PriorityMoveQueue& queue)
{
    while (movesbb.empty() == false) {
        i32 dstSqr = movesbb.popLsb();

        PackedMove move;
        move.setSource(static_cast<Square>(srcSqr));
        move.setTarget(static_cast<Square>(dstSqr));
        move.setCapture(capture);

        PrioratizedMove prioratizedMove(move, 1);
        queue.push(prioratizedMove);
    }
}