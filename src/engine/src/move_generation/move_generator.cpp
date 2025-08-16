#include <move_generation/move_generator.hpp>
#include <move_generation/move_bulk_generator.hpp>

#include <attacks/attacks.hpp>

#include "game_context.h"
#include <material/material_topology.hpp>
#include <move/move.hpp>
#include "transposition_table.hpp"
#include "search.hpp"

#include <algorithm>

template<Set us>
MoveGenerator<us>::MoveGenerator(PositionReader position, const MoveGenParams& params) :
    m_position(position),   
    m_movesGenerated(false),
    m_currentMoveIndx(0),
    m_moveCount(0),
    m_pinThreats(toSquare(position.material().king<us>().lsbIndex()), position)
{
    m_movesBuffer.fill({});
}

template MoveGenerator<Set::WHITE>::MoveGenerator(PositionReader, const MoveGenParams&);
template MoveGenerator<Set::BLACK>::MoveGenerator(PositionReader, const MoveGenParams&);

template<Set us>
PrioritizedMove MoveGenerator<us>::generateNextMove() {
    if (m_currentMoveIndx < m_moveCount) {
        return m_movesBuffer[m_currentMoveIndx++];
    }

    if (m_movesGenerated)
        return { PackedMove::NullMove(), 0 };

    // generateNextMove<us>();

    return { PackedMove::NullMove(), 0 };
}

template PrioritizedMove MoveGenerator<Set::WHITE>::generateNextMove();
template PrioritizedMove MoveGenerator<Set::BLACK>::generateNextMove();

template<Set us>
void MoveGenerator<us>::internalGenerateMoves() {
    const size_t usIndx = static_cast<size_t>(us);
    if (m_position.material().combine<us>().empty()) {
        // TODO: This should never happen, there will always be a king?
        m_movesGenerated = true;        
    }

    BulkMoveGenerator bulkMoveGen(m_position);

    if (m_movesGenerated == false) {
        // special case for when king is in a double check, no need to generate other moves than king.
        if (m_pinThreats.isCheckedCount() > 1) {
            internalGenerateKingMoves(bulkMoveGen);
        }
        else {
            internalGeneratePawnMoves(bulkMoveGen);
            internalGenerateMovesGeneric<knightId>(bulkMoveGen);
            internalGenerateMovesGeneric<bishopId>(bulkMoveGen);
            internalGenerateMovesGeneric<rookId>(bulkMoveGen);
            internalGenerateMovesGeneric<queenId>(bulkMoveGen);
            internalGenerateKingMoves(bulkMoveGen);
        }

        // sortMoves();
    }
}

template void MoveGenerator<Set::WHITE>::internalGenerateMoves();
template void MoveGenerator<Set::BLACK>::internalGenerateMoves();


// template<Set us>
// void MoveGenerator<us>::sortMoves() {
    // if (m_tt != nullptr) {
    //     PackedMove pv = m_tt->probe(m_hashKey);
    //     if (pv != PackedMove::NullMove()) {
    //         auto itrMv = std::find_if(m_movesBuffer.begin(), m_movesBuffer.begin() + m_moveCount, [&](const PrioritizedMove& pm) {
    //             return pm.move == pv;
    //             });

    //         if (itrMv != m_movesBuffer.end()) {
    //             itrMv->priority += move_generator_constants::pvMovePriority;
    //         }
    //     }
    // }

    // if (m_search != nullptr) {
    //     for (u32 i = 0; i < m_moveCount; ++i) {
    //         auto& move = m_movesBuffer[i];
    //         if (m_search->isKillerMove(move.move, m_ply)) {
    //             move.priority += move_generator_constants::killerMovePriority;
    //             move.priority += m_search->getHistoryHeuristic(static_cast<u8>(m_toMove), move.move.source(), move.move.target());
    //         }
    //     }
    // }

    // PrioratizedMoveComparator comparator;
    // std::sort(m_movesBuffer.begin(), m_movesBuffer.begin() + m_moveCount, comparator);
//}

// void MoveGenerator<us>::forEachMove(std::function<void(const PrioritizedMove&)> callback) const {
//     if (m_movesGenerated == false)
//         LOG_ERROR() << "Moves have not been generated yet.";

//     for (uint16_t i = 0; i < m_moveCount; i++) {
//         callback(m_movesBuffer[i]);
//     }
// }

template<Set us>
void MoveGenerator<us>::internalBuildPawnPromotionMoves(PackedMove move, i32 dstSqr)
{    
    bool orthogonallyChecked = !(m_pinThreats.readOpponentOpenAngles()[0] & squareMaskTable[dstSqr]).empty();
    bool diagonallyChecked = !(m_pinThreats.readOpponentOpenAngles()[1] & squareMaskTable[dstSqr]).empty();

    u16 promotionPriorityValue = move_generator_constants::promotionPriority << u8(move.isCapture());

    move.setPromoteTo(queenId);
    PrioritizedMove queenPromote(move, 0);
    queenPromote.setCheck(orthogonallyChecked || diagonallyChecked);
    queenPromote.priority = promotionPriorityValue;

    m_movesBuffer[m_moveCount] = queenPromote;
    m_moveCount++;

    move.setPromoteTo(rookId);
    PrioritizedMove rookPromote(move, 1);
    rookPromote.setCheck(orthogonallyChecked);
    rookPromote.priority = promotionPriorityValue;
    m_movesBuffer[m_moveCount] = rookPromote;
    m_moveCount++;

    move.setPromoteTo(bishopId);
    PrioritizedMove bishopPromote(move, 1);
    bishopPromote.setCheck(diagonallyChecked);
    bishopPromote.priority = promotionPriorityValue;
    m_movesBuffer[m_moveCount] = bishopPromote;
    m_moveCount++;

    // don't I need to check for check here?
    move.setPromoteTo(knightId);
    PrioritizedMove knigthPromote(move, 1);
    knigthPromote.priority = promotionPriorityValue;
    m_movesBuffer[m_moveCount] = knigthPromote;
    m_moveCount++;
}

template void MoveGenerator<Set::WHITE>::internalBuildPawnPromotionMoves(PackedMove, i32);
template void MoveGenerator<Set::BLACK>::internalBuildPawnPromotionMoves(PackedMove, i32);

template<Set us>
void MoveGenerator<us>::internalGeneratePawnMoves(BulkMoveGenerator bulkMoveGen)
{
    u8 usIndx = static_cast<u8>(us);
    const Bitboard movesbb = bulkMoveGen.computeBulkPawnMoves<us>();
    if (movesbb.empty())
        return;

        
    PieceIsolator<us, pawnId> isolator(m_position, movesbb, m_pinThreats);

    // cache pawns in local variable which we'll use to iterate over all pawns.
    Bitboard pawns = m_position.material().pawns<us>();

    while (pawns.empty() == false) {
        // build source square and remove pawn from pawns bitboard.
        const Square srcSqr = toSquare(pawns.popLsb());
        const u64 promotionMask = pawn_constants::promotionRank[usIndx];

        auto isolated = isolator.isolate(srcSqr);
        while (isolated.captures.empty() == false) {
            i32 dstSqr = isolated.captures.popLsb();

            PrioritizedMove prioratizedMove;
            PackedMove& move = prioratizedMove.move;
            move.setSource(srcSqr);
            move.setTarget(dstSqr);

            prioratizedMove.priority = move_generator_constants::capturePriority;

            // if we're capturing enpassant set the enpassant flag.
            if (m_position.enPassant().readSquare() == static_cast<Square>(dstSqr))
                move.setEnPassant(true);  // sets both capture & enpassant
            else
                move.setCapture(true);

            // if we're promoting set the promotion flag and create 4 moves.
            if (promotionMask & squareMaskTable[dstSqr]) {
                internalBuildPawnPromotionMoves(move, dstSqr);
            }
            else {
                Position checkedPos;
                checkedPos.edit().placePiece(ChessPiece(us, PieceType::PAWN), static_cast<Square>(dstSqr));
                Bitboard threat = checkedPos.read().material().topology<us>().computeThreatenedSquaresPawnBulk();
                if (threat & m_position.material().king<opposing_set<us>()>()) {
                    prioratizedMove.setCheck(true);
                    prioratizedMove.priority += move_generator_constants::checkPriority;
                }

                m_movesBuffer[m_moveCount] = prioratizedMove;
                m_moveCount++;
            }
        }
        while (isolated.quiets.empty() == false) {
            i32 dstSqr = isolated.quiets.popLsb();

            PrioritizedMove prioratizedMove;
            PackedMove& move = prioratizedMove.move;
            move.setSource(srcSqr);
            move.setTarget(dstSqr);

            // if we're promoting set the promotion flag and create 4 moves.
            if (promotionMask & squareMaskTable[dstSqr]) {
                internalBuildPawnPromotionMoves(move, dstSqr);
            }
            else {
                Position checkedPos;
                checkedPos.edit().placePiece(ChessPiece(us, PieceType::PAWN), static_cast<Square>(dstSqr));
                Bitboard threat = checkedPos.read().material().topology<us>().computeThreatenedSquaresPawnBulk();
                if (threat & m_position.material().king<opposing_set<us>()>()) {
                    prioratizedMove.setCheck(true);
                    prioratizedMove.priority += move_generator_constants::checkPriority;
                }

                m_movesBuffer[m_moveCount] = prioratizedMove;
                m_moveCount++;
            }
        }
    }
}

template void MoveGenerator<Set::WHITE>::internalGeneratePawnMoves(BulkMoveGenerator);
template void MoveGenerator<Set::BLACK>::internalGeneratePawnMoves(BulkMoveGenerator);

template<Set us>
void MoveGenerator<us>::internalGenerateKingMoves(BulkMoveGenerator bulkMoveGen)
{
    const Bitboard opMaterial = m_position.material().combine<opposing_set<us>()>();
    const u8 setId = static_cast<u8>(us);

    Bitboard movesbb = bulkMoveGen.computeKingMoves<us>();
#if defined EG_DEBUGGING || defined EG_TESTING
    // during testing and debugging king can be missing
    if (movesbb.empty())
        return;
#endif

    u32 srcSqr = m_position.material().king<us>().lsbIndex();
    u8 castlingRaw = m_position.castling().read() >> (setId * 2);

    while (movesbb.empty() == false) {
        i32 dstSqr = movesbb.popLsb();

        PrioritizedMove prioratizedMove;
        PackedMove& move = prioratizedMove.move;
        move.setSource(srcSqr);
        move.setTarget(dstSqr);
        u64 dstSqrMsk = squareMaskTable[dstSqr];

        if (opMaterial & dstSqrMsk) {
            move.setCapture(true);
            prioratizedMove.priority = move_generator_constants::capturePriority;
        }

        if (castlingRaw & 2) {
            u64 queenSideCastleSqrMask = king_constants::queenSideCastleMask & board_constants::baseRankRelative[setId];
            if (dstSqrMsk & queenSideCastleSqrMask) {
                move.setCastleQueenSide(true);
            }
        }
        if (castlingRaw & 1) {
            u64 kingSideCastleSqrMask = king_constants::kingSideCastleMask & board_constants::baseRankRelative[setId];
            if (dstSqrMsk & kingSideCastleSqrMask) {
                move.setCastleKingSide(true);
            }
        }

        m_movesBuffer[m_moveCount] = prioratizedMove;
        m_moveCount++;
    }
}

template void MoveGenerator<Set::WHITE>::internalGenerateKingMoves(BulkMoveGenerator);
template void MoveGenerator<Set::BLACK>::internalGenerateKingMoves(BulkMoveGenerator);

template<Set us>
void MoveGenerator<us>::buildPackedMoveFromBitboard(u8 pieceId, Bitboard movesbb, Square srcSqr, bool capture)
{
    while (movesbb.empty() == false) {
        Square dstSqr = toSquare(movesbb.popLsb());

        PrioritizedMove prioratizedMove;
        PackedMove& move = prioratizedMove.move;
        move.setSource(srcSqr);
        move.setTarget(dstSqr);
        move.setCapture(capture);
        prioratizedMove.priority = 0;

        if (capture) {
            u8 recaptureBonus = ((m_position.material().combine<opposing_set<us>()>() & squareMaskTable[*dstSqr]).empty() == false) ? 2 : 1;
            prioratizedMove.priority = move_generator_constants::capturePriority * recaptureBonus;
        }

        // figure out if we're checking the king.
        if (pieceId == rookId || pieceId == queenId) {
            if (m_pinThreats.readOpponentOpenAngles()[0] & squareMaskTable[*dstSqr]) {
                prioratizedMove.setCheck(true);
                prioratizedMove.priority += move_generator_constants::checkPriority;
            }
        }
        else if (pieceId == bishopId || pieceId == queenId) {
            if (m_pinThreats.readOpponentOpenAngles()[1] & squareMaskTable[*dstSqr]) {
                prioratizedMove.setCheck(true);
                prioratizedMove.priority += move_generator_constants::checkPriority;
            }
        }
        m_movesBuffer[m_moveCount] = prioratizedMove;
        m_moveCount++;
    }
}

template void MoveGenerator<Set::WHITE>::buildPackedMoveFromBitboard(u8, Bitboard, Square, bool);
template void MoveGenerator<Set::BLACK>::buildPackedMoveFromBitboard(u8, Bitboard, Square, bool);