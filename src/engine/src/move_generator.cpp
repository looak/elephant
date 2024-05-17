#include "move_generator.hpp"
#include "game_context.h"
#include "move.h"

MoveGenerator::MoveGenerator(const Position& pos, Set toMove, PieceType ptype, MoveTypes mtype) :
    m_toMove(toMove),
    m_position(pos),
    m_movesGenerated(false),
    m_moveCount(0),
    m_currentMoveIndx(0),
    m_movesBuffer()
{
    initializeMoveGenerator(ptype, mtype);
}

MoveGenerator::MoveGenerator(const GameContext& context) :
    m_toMove(context.readToPlay()),
    m_position(context.readChessboard().readPosition()),
    m_movesGenerated(false),
    m_moveCount(0),
    m_currentMoveIndx(0),
    m_movesBuffer()
{
    initializeMoveGenerator(PieceType::NONE, MoveTypes::ALL);
}

PackedMove
MoveGenerator::generateNextMove()
{
    if (m_currentMoveIndx < m_moveCount) {
        return m_movesBuffer[m_currentMoveIndx++].move;
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
    if (m_moveMasks[setIndx].combine().empty()) {
        m_movesGenerated = true;
        return PackedMove::NullMove();
    }

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

    m_movesGenerated = true;
    if (m_currentMoveIndx < m_moveCount) {
        return m_movesBuffer[m_currentMoveIndx++].move;
    }

    return PackedMove::NullMove();
}

template<Set set>
void
MoveGenerator::generateAllMoves()
{
    const size_t setIndx = static_cast<size_t>(set);
    if (m_moveMasks[setIndx].combine().empty()) {
        m_movesGenerated = true;
        return;
    }

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

    for (uint16_t i = 0; i < m_moveCount; i++) {
        callback(m_movesBuffer[i]);
    }
}

void MoveGenerator::internalBuildPawnPromotionMoves(PackedMove move, const KingPinThreats& pinThreats, i32 dstSqr)
{
    bool orthogonallyChecked = !(pinThreats.readOpponentOpenAngles()[0] & squareMaskTable[dstSqr]).empty();
    bool diagonallyChecked = !(pinThreats.readOpponentOpenAngles()[1] & squareMaskTable[dstSqr]).empty();

    move.setPromoteTo(queenId);
    PrioratizedMove queenPromote(move, 0);
    queenPromote.setCheck(orthogonallyChecked || diagonallyChecked);
    m_movesBuffer[m_moveCount] = queenPromote;
    m_moveCount++;

    move.setPromoteTo(rookId);
    PrioratizedMove rookPromote(move, 1);
    rookPromote.setCheck(orthogonallyChecked);
    m_movesBuffer[m_moveCount] = rookPromote;
    m_moveCount++;

    move.setPromoteTo(bishopId);
    PrioratizedMove bishopPromote(move, 1);
    bishopPromote.setCheck(diagonallyChecked);
    m_movesBuffer[m_moveCount] = bishopPromote;
    m_moveCount++;

    // don't I need to check for check here?
    move.setPromoteTo(knightId);
    PrioratizedMove knigthPromote(move, 1);
    m_movesBuffer[m_moveCount] = knigthPromote;
    m_moveCount++;
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
    Bitboard pawns = pos.readMaterial().pawns<set>();

    while (pawns.empty() == false) {
        // build source square and remove pawn from pawns bitboard.
        const i32 srcSqr = pawns.popLsb();
        const Notation srcNotation(srcSqr);

        const u64 promotionMask = pawn_constants::promotionRank[(size_t)set];

        auto [isolatedPawnMoves, isolatedPawnAttacks] = pos.isolatePiece<set, pawnId>(srcNotation, movesbb, pinThreats);
        while (isolatedPawnAttacks.empty() == false) {
            i32 dstSqr = isolatedPawnAttacks.popLsb();

            PrioratizedMove prioratizedMove;
            PackedMove& move = prioratizedMove.move;
            move.setSource(srcSqr);
            move.setTarget(dstSqr);

            // if we're capturing enpassant set the enpassant flag.
            if (pos.readEnPassant().readSquare() == static_cast<Square>(dstSqr))
                move.setEnPassant(true);  // sets both capture & enpassant
            else
                move.setCapture(true);

            // if we're promoting set the promotion flag and create 4 moves.
            if (promotionMask & squareMaskTable[dstSqr]) {
                internalBuildPawnPromotionMoves(move, pinThreats, dstSqr);
            }
            else {
                Position checkedPos;
                checkedPos.PlacePiece(ChessPiece(set, PieceType::PAWN), Notation(dstSqr));
                auto threat = checkedPos.calcThreatenedSquaresPawnBulk<set>();
                if (threat & pos.readMaterial().kings<opposing_set<set>()>())
                    prioratizedMove.setCheck(true);

                m_movesBuffer[m_moveCount] = prioratizedMove;
                m_moveCount++;
            }
        }
        while (isolatedPawnMoves.empty() == false) {
            i32 dstSqr = isolatedPawnMoves.popLsb();

            PrioratizedMove prioratizedMove;
            PackedMove& move = prioratizedMove.move;
            move.setSource(srcSqr);
            move.setTarget(dstSqr);

            // if we're promoting set the promotion flag and create 4 moves.
            if (promotionMask & squareMaskTable[dstSqr]) {
                internalBuildPawnPromotionMoves(move, pinThreats, dstSqr);
            }
            else {
                Position checkedPos;
                checkedPos.PlacePiece(ChessPiece(set, PieceType::PAWN), Notation(dstSqr));
                auto threat = checkedPos.calcThreatenedSquaresPawnBulk<set>();
                if (threat & pos.readMaterial().kings<opposing_set<set>()>())
                    prioratizedMove.setCheck(true);

                m_movesBuffer[m_moveCount] = prioratizedMove;
                m_moveCount++;
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

    Bitboard pieces = bb.readMaterial().read<set>(pieceId);

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
MoveGenerator::internalGenerateKingMoves()
{
    const auto& bb = m_position;
    const Bitboard opMaterial = bb.readMaterial().combine<opposing_set<set>()>();
    const u8 setId = static_cast<u8>(set);

    Bitboard movesbb = m_moveMasks[setId].material[kingId];
#if defined EG_DEBUGGING || defined EG_TESTING
    // during testing and debugging king can be missing
    if (movesbb.empty())
        return;
#endif

    u32 srcSqr = bb.readMaterial().kings<set>().lsbIndex();
    u8 castlingRaw = bb.readCastling().read() >> (setId * 2);

    while (movesbb.empty() == false) {
        i32 dstSqr = movesbb.popLsb();

        PrioratizedMove prioratizedMove;
        PackedMove& move = prioratizedMove.move;
        move.setSource(srcSqr);
        move.setTarget(dstSqr);
        u64 dstSqrMsk = squareMaskTable[dstSqr];

        if (opMaterial & dstSqrMsk)
            move.setCapture(true);

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

template void MoveGenerator::internalGenerateKingMoves<Set::WHITE>();
template void MoveGenerator::internalGenerateKingMoves<Set::BLACK>();

void
MoveGenerator::initializeMoveGenerator(PieceType ptype, MoveTypes mtype)
{
    bool captures = mtype == MoveTypes::CAPTURES_ONLY;

    if (m_toMove == Set::WHITE)
    {
        if (captures)
            initializeMoveMasks<Set::WHITE, true>(m_moveMasks[0], ptype);
        else
            initializeMoveMasks<Set::WHITE, false>(m_moveMasks[0], ptype);
    }
    else
        if (captures)
            initializeMoveMasks<Set::BLACK, true>(m_moveMasks[1], ptype);
        else
            initializeMoveMasks<Set::BLACK, false>(m_moveMasks[1], ptype);
}

template<Set set, bool captures>
void
MoveGenerator::initializeMoveMasks(MaterialMask& target, PieceType ptype)
{
    const auto& bb = m_position;
    if (bb.empty())
        return;
    const size_t setIndx = static_cast<size_t>(set);
    m_pinThreats[setIndx] = bb.calcKingMask<set>();

    if (ptype == PieceType::NONE) {
        target.material[pawnId] = bb.calcAvailableMovesPawnBulk<set, captures>(m_pinThreats[setIndx]);
        target.material[knightId] = bb.calcAvailableMovesKnightBulk<set, captures>(m_pinThreats[setIndx]);
        target.material[bishopId] = bb.calcAvailableMovesBishopBulk<set, captures>(m_pinThreats[setIndx]);
        target.material[rookId] = bb.calcAvailableMovesRookBulk<set, captures>(m_pinThreats[setIndx]);
        target.material[queenId] = bb.calcAvailableMovesQueenBulk<set, captures>(m_pinThreats[setIndx]);
        target.material[kingId] = bb.calcAvailableMovesKing<set, captures>(bb.readCastling().read());
    }
    else {
        switch (ptype) {
        case PieceType::PAWN:
            target.material[pawnId] = bb.calcAvailableMovesPawnBulk<set, captures>(m_pinThreats[setIndx]);
            break;
        case PieceType::KNIGHT:
            target.material[knightId] = bb.calcAvailableMovesKnightBulk<set, captures>(m_pinThreats[setIndx]);
            break;
        case PieceType::BISHOP:
            target.material[bishopId] = bb.calcAvailableMovesBishopBulk<set, captures>(m_pinThreats[setIndx]);
            break;
        case PieceType::ROOK:
            target.material[rookId] = bb.calcAvailableMovesRookBulk<set, captures>(m_pinThreats[setIndx]);
            break;
        case PieceType::QUEEN:
            target.material[queenId] = bb.calcAvailableMovesQueenBulk<set, captures>(m_pinThreats[setIndx]);
            break;
        case PieceType::KING:
            target.material[kingId] = bb.calcAvailableMovesKing<set, captures>(bb.readCastling().read());
            break;
        default:
            FATAL_ASSERT(false) << "Invalid piece type";
            break;
        }
    }
}

template void MoveGenerator::initializeMoveMasks<Set::WHITE, true>(MaterialMask& target, PieceType ptype);
template void MoveGenerator::initializeMoveMasks<Set::BLACK, true>(MaterialMask& target, PieceType ptype);
template void MoveGenerator::initializeMoveMasks<Set::WHITE, false>(MaterialMask& target, PieceType ptype);
template void MoveGenerator::initializeMoveMasks<Set::BLACK, false>(MaterialMask& target, PieceType ptype);

void
MoveGenerator::genPackedMovesFromBitboard(u8 pieceId, Bitboard movesbb, i32 srcSqr, bool capture, const KingPinThreats& pinThreats)
{
    while (movesbb.empty() == false) {
        i32 dstSqr = movesbb.popLsb();

        PrioratizedMove prioratizedMove;
        PackedMove& move = prioratizedMove.move;
        move.setSource(static_cast<Square>(srcSqr));
        move.setTarget(static_cast<Square>(dstSqr));
        move.setCapture(capture);

        // figure out if we're checking the king.
        if (pieceId == rookId || pieceId == queenId) {
            if (pinThreats.readOpponentOpenAngles()[0] & squareMaskTable[dstSqr])
                prioratizedMove.setCheck(true);
        }
        else if (pieceId == bishopId || pieceId == queenId) {
            if (pinThreats.readOpponentOpenAngles()[1] & squareMaskTable[dstSqr])
                prioratizedMove.setCheck(true);
        }
        m_movesBuffer[m_moveCount] = prioratizedMove;
        m_moveCount++;

        // m_moves.push(prioratizedMove);
        // m_unsortedMoves.push_back(prioratizedMove);
    }
}

bool
MoveGenerator::isChecked() const
{
    if (m_toMove == Set::WHITE)
        return m_pinThreats[0].isCheckedCount() > 0;
    else
        return m_pinThreats[1].isCheckedCount() > 0;
}