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
    if (m_moveMasks[(size_t)set].combine() == 0)
        return PackedMove::NullMove();

    if (m_returnedMoves.empty()) {
        generateMoves<set, pawnId>();
        generateMoves<set, knightId>();
        generateMoves<set, bishopId>();
        generateMoves<set, rookId>();
        generateMoves<set, queenId>();
        generateMoves<set, kingId>();
    }

    auto move = m_moves.top();
    m_moves.pop();
    m_returnedMoves.push_back(move.move);
    return move.move;
}

template<Set set>
void
MoveGenerator::internalGeneratePawnMoves()
{
    const auto& bb = m_context.readChessboard().readBitboard();

    const u64 movesbb = m_moveMasks[(size_t)set].material[pawnId];
    if (movesbb == 0)
        return;

    // cache pawns in local variable which we'll use to iterate over all pawns.
    u64 pawns = bb.readMaterial<set>().material[pawnId];

    while (pawns != 0) {
        // build source square and remove pawn from pawns bitboard.
        const i32 srcSqr = intrinsics::lsbIndex(pawns);
        const Notation srcNotation(srcSqr);
        pawns = intrinsics::resetLsb(pawns);

        u64 isolatedPawnMoves = bb.isolatePiece<set>(pawnId, srcNotation, movesbb);
        while (isolatedPawnMoves != 0) {
            i32 dstSqr = intrinsics::lsbIndex(isolatedPawnMoves);
            isolatedPawnMoves = intrinsics::resetLsb(isolatedPawnMoves);

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

    u64 movesbb = m_moveMasks[(size_t)set].material[kingId];
#if defined EG_DEBUGGING || defined EG_TESTING
    // during testing and debugging king can be missing
    if (movesbb == 0)
        return;
#endif

    i32 srcSqr = intrinsics::lsbIndex(bb.readMaterial<set>().material[kingId]);

    while (movesbb != 0) {
        i32 dstSqr = intrinsics::lsbIndex(movesbb);
        movesbb = intrinsics::resetLsb(movesbb);

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
    target.material[pawnId] = bb.calcAvailableMovesPawnsBulk<set>();
    target.material[knightId] = bb.calcAvailableMovesKnightBulk<set>();
    target.material[bishopId] = bb.calcAvailableMovesBishopBulk<set>();
    target.material[rookId] = bb.calcAvailableMovesRookBulk<set>();
    target.material[queenId] = bb.calcAvailableMovesQueenBulk<set>();
    target.material[kingId] = bb.calcAvailableMovesKingBulk<set>();
}

template void MoveGenerator::initializeMoveMasks<Set::WHITE>(MaterialMask& target);
template void MoveGenerator::initializeMoveMasks<Set::BLACK>(MaterialMask& target);