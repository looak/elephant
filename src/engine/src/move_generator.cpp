#include "move_generator.hpp"
#include "game_context.h"
#include "move.h"

MoveGenerator::MoveGenerator(const Position& pos, Set toMove, PieceType ptype, MoveTypes mtype) :
    m_toMove(toMove),
    m_position(pos),
    m_moves(),
    m_unsortedMoves(),
    m_returnedMoves(),
    m_movesGenerated(false)
{
    initializeMoveGenerator(ptype, mtype);
}

MoveGenerator::MoveGenerator(const GameContext& context) :
    m_toMove(context.readToPlay()),
    m_position(context.readChessboard().readPosition()),
    m_moves(),
    m_unsortedMoves(),
    m_returnedMoves(),
    m_movesGenerated(false)
{
    initializeMoveGenerator(PieceType::NONE, MoveTypes::ALL);
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

    if (m_toMove == Set::WHITE) {
        return generateNextMove<Set::WHITE>();
    }
    else {
        return generateNextMove<Set::BLACK>();
    }
    return PackedMove::NullMove();
}

void
MoveGenerator::generate()
{
    if (m_toMove == Set::WHITE)
        generateAllMoves<Set::WHITE>();
    else
        generateAllMoves<Set::BLACK>();
}

template<Set set>
PackedMove
MoveGenerator::generateNextMove()
{
    const size_t setIndx = static_cast<size_t>(set);
    if (m_moveMasks[setIndx].combine().empty())
        return PackedMove::NullMove();

    if (m_movesGenerated == false) {
        if (m_pinThreats->isCheckedCount() > 1) {
            generateMoves<set, kingId>(m_pinThreats[setIndx]);
        }
        else {
            generateMoves<set, pawnId>(m_pinThreats[setIndx]);
            generateMoves<set, knightId>(m_pinThreats[setIndx]);
            generateMoves<set, bishopId>(m_pinThreats[setIndx]);
            generateMoves<set, rookId>(m_pinThreats[setIndx]);
            generateMoves<set, queenId>(m_pinThreats[setIndx]);
            generateMoves<set, kingId>(m_pinThreats[setIndx]);
        }
    }

    FATAL_ASSERT(!m_moves.empty()) << "This should never be able to happen since our bitboards have moves in them.";

    m_movesGenerated = true;
    auto move = m_moves.top();
    m_moves.pop();
    m_returnedMoves.push_back(move.move);
    return move.move;
}

template<Set set>
void
MoveGenerator::generateAllMoves()
{
    const size_t setIndx = static_cast<size_t>(set);
    if (m_moveMasks[setIndx].combine().empty())
        return;

    if (m_pinThreats->isCheckedCount() > 1) {
        generateMoves<set, kingId>(m_pinThreats[setIndx]);
    }
    else {
        generateMoves<set, pawnId>(m_pinThreats[setIndx]);
        generateMoves<set, knightId>(m_pinThreats[setIndx]);
        generateMoves<set, bishopId>(m_pinThreats[setIndx]);
        generateMoves<set, rookId>(m_pinThreats[setIndx]);
        generateMoves<set, queenId>(m_pinThreats[setIndx]);
        generateMoves<set, kingId>(m_pinThreats[setIndx]);
    }

    m_movesGenerated = true;
}

void
MoveGenerator::forEachMove(std::function<void(const PrioratizedMove&)> callback) const
{
    if (m_movesGenerated == false)
        LOG_ERROR() << "Moves have not been generated yet.";

    for (auto move : m_unsortedMoves) {
        callback(move);
    }
}

template<Set set>
void
MoveGenerator::internalGeneratePawnMoves(const KingPinThreats& pinThreats)
{
    const auto& pos = m_position;

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

        auto [isolatedPawnMoves, isolatedPawnAttacks] = pos.isolatePiece<set, pawnId>(srcNotation, movesbb, pinThreats);
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
                if ((pinThreats.readOpponentOpenAngles()[0] | pinThreats.readOpponentOpenAngles()[1]) & squareMaskTable[dstSqr])
                    prioratizedMove.setCheck(true);
                m_moves.push(prioratizedMove);
                m_unsortedMoves.push_back(prioratizedMove);

                move.setPromoteTo(rookId);
                PrioratizedMove prioratizedMove2(move, 1);
                if (pinThreats.readOpponentOpenAngles()[0] & squareMaskTable[dstSqr])
                    prioratizedMove2.setCheck(true);
                m_moves.push(prioratizedMove2);
                m_unsortedMoves.push_back(prioratizedMove2);

                move.setPromoteTo(bishopId);
                PrioratizedMove prioratizedMove3(move, 1);
                if (pinThreats.readOpponentOpenAngles()[1] & squareMaskTable[dstSqr])
                    prioratizedMove3.setCheck(true);
                m_moves.push(prioratizedMove3);
                m_unsortedMoves.push_back(prioratizedMove3);

                move.setPromoteTo(knightId);
                PrioratizedMove prioratizedMove4(move, 1);
                m_moves.push(prioratizedMove4);
                m_unsortedMoves.push_back(prioratizedMove4);
            }
            else {
                PrioratizedMove prioratizedMove(move, 1);
                Position checkedPos;
                checkedPos.PlacePiece(ChessPiece(set, PieceType::PAWN), Notation(dstSqr));
                auto threat = checkedPos.calcThreatenedSquaresPawnBulk<set>();
                if (threat & pos.readMaterial<opposing_set<set>()>().kings())
                    prioratizedMove.setCheck(true);
                m_moves.push(prioratizedMove);
                m_unsortedMoves.push_back(prioratizedMove);
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
                if ((pinThreats.readOpponentOpenAngles()[0] | pinThreats.readOpponentOpenAngles()[1]) & squareMaskTable[dstSqr])
                    prioratizedMove.setCheck(true);
                m_moves.push(prioratizedMove);
                m_unsortedMoves.push_back(prioratizedMove);

                move.setPromoteTo(rookId);
                PrioratizedMove prioratizedMove2(move, 1);
                if (pinThreats.readOpponentOpenAngles()[0] & squareMaskTable[dstSqr])
                    prioratizedMove2.setCheck(true);
                m_moves.push(prioratizedMove2);
                m_unsortedMoves.push_back(prioratizedMove2);

                move.setPromoteTo(bishopId);
                PrioratizedMove prioratizedMove3(move, 1);
                if (pinThreats.readOpponentOpenAngles()[1] & squareMaskTable[dstSqr])
                    prioratizedMove3.setCheck(true);
                m_moves.push(prioratizedMove3);
                m_unsortedMoves.push_back(prioratizedMove3);

                move.setPromoteTo(knightId);
                PrioratizedMove prioratizedMove4(move, 1);
                m_moves.push(prioratizedMove4);
                m_unsortedMoves.push_back(prioratizedMove4);
            }
            else {
                PrioratizedMove prioratizedMove(move, 1);
                Position checkedPos;
                checkedPos.PlacePiece(ChessPiece(set, PieceType::PAWN), Notation(dstSqr));
                auto threat = checkedPos.calcThreatenedSquaresPawnBulk<set>();
                if (threat & pos.readMaterial<opposing_set<set>()>().kings())
                    prioratizedMove.setCheck(true);
                m_moves.push(prioratizedMove);
                m_unsortedMoves.push_back(prioratizedMove);
            }
        }
    }
}
template void MoveGenerator::internalGeneratePawnMoves<Set::WHITE>(const KingPinThreats& pinThreats);
template void MoveGenerator::internalGeneratePawnMoves<Set::BLACK>(const KingPinThreats& pinThreats);

template<Set set>
void
MoveGenerator::internalGenerateMoves(u8 pieceId, const KingPinThreats& pinThreats)
{
    const auto& bb = m_position;

    const Bitboard movesbb = m_moveMasks[(size_t)set].material[pieceId];
    if (movesbb.empty())
        return;

    Bitboard pieces = bb.readMaterial<set>().material[pieceId];

    while (pieces.empty() == false) {
        // build source square and remove knight from cached material bitboard.
        const i32 srcSqr = pieces.popLsb();
        const Notation srcNotation(srcSqr);

        auto [isolatedMoves, isolatedCaptures] = bb.isolatePiece<set>(pieceId, srcNotation, movesbb, pinThreats);
        genPackedMovesFromBitboard(pieceId, isolatedCaptures, srcSqr, /*are captures*/ true, pinThreats);
        genPackedMovesFromBitboard(pieceId, isolatedMoves, srcSqr, /*are captures*/ false, pinThreats);
    }
}

template void MoveGenerator::internalGenerateMoves<Set::WHITE>(u8, const KingPinThreats&);
template void MoveGenerator::internalGenerateMoves<Set::BLACK>(u8, const KingPinThreats&);

template<Set set>
void
MoveGenerator::internalGenerateKnightMoves(const KingPinThreats& pinThreats)
{
    internalGenerateMoves<set>(knightId, pinThreats);
}

template void MoveGenerator::internalGenerateKnightMoves<Set::WHITE>(const KingPinThreats& pinThreats);
template void MoveGenerator::internalGenerateKnightMoves<Set::BLACK>(const KingPinThreats& pinThreats);

template<Set set>
void
MoveGenerator::internalGenerateBishopMoves(const KingPinThreats& pinThreats)
{
    internalGenerateMoves<set>(bishopId, pinThreats);
}

template void MoveGenerator::internalGenerateBishopMoves<Set::WHITE>(const KingPinThreats& pinThreats);
template void MoveGenerator::internalGenerateBishopMoves<Set::BLACK>(const KingPinThreats& pinThreats);

template<Set set>
void
MoveGenerator::internalGenerateRookMoves(const KingPinThreats& pinThreats)
{
    internalGenerateMoves<set>(rookId, pinThreats);
}

template void MoveGenerator::internalGenerateRookMoves<Set::WHITE>(const KingPinThreats& pinThreats);
template void MoveGenerator::internalGenerateRookMoves<Set::BLACK>(const KingPinThreats& pinThreats);

template<Set set>
void
MoveGenerator::internalGenerateQueenMoves(const KingPinThreats& pinThreats)
{
    internalGenerateMoves<set>(queenId, pinThreats);
}

template void MoveGenerator::internalGenerateQueenMoves<Set::WHITE>(const KingPinThreats& pinThreats);
template void MoveGenerator::internalGenerateQueenMoves<Set::BLACK>(const KingPinThreats& pinThreats);

template<Set set>
void
MoveGenerator::internalGenerateKingMoves(const KingPinThreats& pinThreats)
{
    const auto& bb = m_position;
    const Bitboard opMaterial = bb.readMaterial<opposing_set<set>()>().combine();

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
        u64 dstSqrMsk = squareMaskTable[dstSqr];

        if (opMaterial & dstSqrMsk)
            move.setCapture(true);

        if (dstSqrMsk & king_constants::queenSideCastleMask) {
            move.setCastleQueenSide(true);
        }
        else if (dstSqrMsk & king_constants::kingSideCastleMask) {
            move.setCastleKingSide(true);
        }

        PrioratizedMove prioratizedMove(move, 1);
        m_moves.push(prioratizedMove);
        m_unsortedMoves.push_back(prioratizedMove);
    }
}

template void MoveGenerator::internalGenerateKingMoves<Set::WHITE>(const KingPinThreats& pinThreats);
template void MoveGenerator::internalGenerateKingMoves<Set::BLACK>(const KingPinThreats& pinThreats);

void
MoveGenerator::initializeMoveGenerator(PieceType ptype, MoveTypes mtype)
{
    if (m_toMove == Set::WHITE)
        initializeMoveMasks<Set::WHITE>(m_moveMasks[0], ptype, mtype);
    else
        initializeMoveMasks<Set::BLACK>(m_moveMasks[1], ptype, mtype);
}

template<Set set>
void
MoveGenerator::initializeMoveMasks(MaterialMask& target, PieceType ptype, MoveTypes mtype)
{
    const auto& bb = m_position;
    if (bb.empty())
        return;
    const size_t setIndx = static_cast<size_t>(set);
    m_pinThreats[setIndx] = bb.calcKingMask<set>();

    bool captures = mtype == MoveTypes::CAPTURES_ONLY;

    if (ptype == PieceType::NONE) {
        target.material[pawnId] = bb.calcAvailableMovesPawnBulk<set>(m_pinThreats[setIndx], captures);
        target.material[knightId] = bb.calcAvailableMovesKnightBulk<set>(m_pinThreats[setIndx], captures);
        target.material[bishopId] = bb.calcAvailableMovesBishopBulk<set>(m_pinThreats[setIndx], captures);
        target.material[rookId] = bb.calcAvailableMovesRookBulk<set>(m_pinThreats[setIndx], captures);
        target.material[queenId] = bb.calcAvailableMovesQueenBulk<set>(m_pinThreats[setIndx], captures);
        target.material[kingId] = bb.calcAvailableMovesKing<set>(bb.readCastling().read(), captures);
    }
    else {
        switch (ptype) {
            case PieceType::PAWN:
                target.material[pawnId] = bb.calcAvailableMovesPawnBulk<set>(m_pinThreats[setIndx], captures);
                break;
            case PieceType::KNIGHT:
                target.material[knightId] = bb.calcAvailableMovesKnightBulk<set>(m_pinThreats[setIndx], captures);
                break;
            case PieceType::BISHOP:
                target.material[bishopId] = bb.calcAvailableMovesBishopBulk<set>(m_pinThreats[setIndx], captures);
                break;
            case PieceType::ROOK:
                target.material[rookId] = bb.calcAvailableMovesRookBulk<set>(m_pinThreats[setIndx], captures);
                break;
            case PieceType::QUEEN:
                target.material[queenId] = bb.calcAvailableMovesQueenBulk<set>(m_pinThreats[setIndx], captures);
                break;
            case PieceType::KING:
                target.material[kingId] = bb.calcAvailableMovesKing<set>(bb.readCastling().read(), captures);
                break;
            default:
                FATAL_ASSERT(false) << "Invalid piece type";
                break;
        }
    }
}

template void MoveGenerator::initializeMoveMasks<Set::WHITE>(MaterialMask& target, PieceType ptype, MoveTypes mtype);
template void MoveGenerator::initializeMoveMasks<Set::BLACK>(MaterialMask& target, PieceType ptype, MoveTypes mtype);

void
MoveGenerator::genPackedMovesFromBitboard(u8 pieceId, Bitboard movesbb, i32 srcSqr, bool capture,
                                          const KingPinThreats& pinThreats)
{
    while (movesbb.empty() == false) {
        i32 dstSqr = movesbb.popLsb();

        PackedMove move;
        move.setSource(static_cast<Square>(srcSqr));
        move.setTarget(static_cast<Square>(dstSqr));
        move.setCapture(capture);

        PrioratizedMove prioratizedMove(move, 1);

        // figure out if we're checking the king.
        if (pieceId == rookId || pieceId == queenId) {
            if (pinThreats.readOpponentOpenAngles()[0] & squareMaskTable[dstSqr])
                prioratizedMove.setCheck(true);
        }
        else if (pieceId == bishopId || pieceId == queenId) {
            if (pinThreats.readOpponentOpenAngles()[1] & squareMaskTable[dstSqr])
                prioratizedMove.setCheck(true);
        }

        m_moves.push(prioratizedMove);
        m_unsortedMoves.push_back(prioratizedMove);
    }
}