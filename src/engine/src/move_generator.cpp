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
    if (m_moveMasks[(size_t)set].combine().empty())
        return PackedMove::NullMove();

    if (m_returnedMoves.empty()) {
        generateMoves<set, pawnId>();
        generateMoves<set, knightId>();
        generateMoves<set, bishopId>();
        generateMoves<set, rookId>();
        generateMoves<set, queenId>();
        generateMoves<set, kingId>();
    }

    FATAL_ASSERT(!m_moves.empty()) << "This should never be able to happen since our bitboards have moves in them.";

    auto move = m_moves.top();
    m_moves.pop();
    m_returnedMoves.push_back(move.move);
    return move.move;
}

template<Set set>
void
MoveGenerator::internalGeneratePawnMoves()
{
    const auto& pos = m_context.readChessboard().readBitboard();

    const Bitboard movesbb = m_moveMasks[(size_t)set].material[pawnId];
    if (movesbb.empty())
        return;

    // cache pawns in local variable which we'll use to iterate over all pawns.
    Bitboard pawns = pos.readMaterial<set>().material[pawnId];

    while (pawns.empty() == false) {
        // build source square and remove pawn from pawns bitboard.
        const i32 srcSqr = pawns.popLsb();
        const Notation srcNotation(srcSqr);

        auto [isolatedPawnMoves, isolatedPawnAttacks] = pos.isolatePiece<set, pawnId>(srcNotation, movesbb);
        while (isolatedPawnAttacks.empty() == false) {
            i32 dstSqr = isolatedPawnAttacks.popLsb();

            PackedMove move;
            move.setSource(srcSqr);
            move.setTarget(dstSqr);
            move.setCapture(true);

            PrioratizedMove prioratizedMove(move, 1);
            m_moves.push(prioratizedMove);
        }
        while (isolatedPawnMoves.empty() == false) {
            i32 dstSqr = isolatedPawnMoves.popLsb();

            PackedMove move;
            move.setSource(srcSqr);
            move.setTarget(dstSqr);

            PrioratizedMove prioratizedMove(move, 1);
            m_moves.push(prioratizedMove);
        }
    }
}
template void MoveGenerator::internalGeneratePawnMoves<Set::WHITE>();
template void MoveGenerator::internalGeneratePawnMoves<Set::BLACK>();

template<Set set>
void
MoveGenerator::internalGenerateKnightMoves()
{
    const auto& bb = m_context.readChessboard().readBitboard();

    const Bitboard movesbb = m_moveMasks[(size_t)set].material[knightId];
    if (movesbb.empty())
        return;

    Bitboard knights = bb.readMaterial<set>().material[knightId];

    while (knights.empty() == false) {
        // build source square and remove knight from cached material bitboard.
        const i32 srcSqr = knights.popLsb();
        const Notation srcNotation(srcSqr);

        auto [isolatedKnightMoves, isolatedKnightAttks] = bb.isolatePiece<set>(knightId, srcNotation, movesbb);
        genPackedMovesFromBitboard(isolatedKnightAttks, srcSqr, /*are captures*/ true, m_moves);
        genPackedMovesFromBitboard(isolatedKnightMoves, srcSqr, /*are captures*/ false, m_moves);
    }
}

template void MoveGenerator::internalGenerateKnightMoves<Set::WHITE>();
template void MoveGenerator::internalGenerateKnightMoves<Set::BLACK>();

template<Set set>
void
MoveGenerator::internalGenerateBishopMoves()
{
}

template void MoveGenerator::internalGenerateBishopMoves<Set::WHITE>();
template void MoveGenerator::internalGenerateBishopMoves<Set::BLACK>();

template<Set set>
void
MoveGenerator::internalGenerateRookMoves()
{
}

template void MoveGenerator::internalGenerateRookMoves<Set::WHITE>();
template void MoveGenerator::internalGenerateRookMoves<Set::BLACK>();

template<Set set>
void
MoveGenerator::internalGenerateQueenMoves()
{
}

template void MoveGenerator::internalGenerateQueenMoves<Set::WHITE>();
template void MoveGenerator::internalGenerateQueenMoves<Set::BLACK>();

template<Set set>
void
MoveGenerator::internalGenerateKingMoves()
{
    const auto& bb = m_context.readChessboard().readBitboard();

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

template void MoveGenerator::internalGenerateKingMoves<Set::WHITE>();
template void MoveGenerator::internalGenerateKingMoves<Set::BLACK>();

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
    const auto& bb = m_context.readChessboard().readBitboard();
    target.material[pawnId] = bb.calcAvailableMovesPawnBulk<set>();
    target.material[knightId] = bb.calcAvailableMovesKnightBulk<set>();
    target.material[bishopId] = bb.calcAvailableMovesBishopBulk<set>();
    target.material[rookId] = bb.calcAvailableMovesRookBulk<set>();
    target.material[queenId] = bb.calcAvailableMovesQueenBulk<set>();
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
        move.setSource(srcSqr);
        move.setTarget(dstSqr);
        move.setCapture(capture);

        PrioratizedMove prioratizedMove(move, 1);
        queue.push(prioratizedMove);
    }
}