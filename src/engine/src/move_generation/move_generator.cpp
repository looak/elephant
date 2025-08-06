#include <move_generation/move_generator.hpp>
#include "game_context.h"
#include <material/material_topology.hpp>
#include <move/move.hpp>
#include "transposition_table.hpp"
#include "search.hpp"

#include <algorithm>

MoveGenerator::MoveGenerator(PositionReader pos, Set toMove, PieceType ptype, MoveTypes mtype) :
    m_toMove(toMove),
    m_position(pos),
    m_tt(nullptr),
    m_search(nullptr),
    m_ply(0),
    m_hashKey(0),
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
    m_tt(nullptr),
    m_search(nullptr),
    m_ply(0),
    m_hashKey(0),
    m_movesGenerated(false),
    m_moveCount(0),
    m_currentMoveIndx(0),
    m_movesBuffer()
{
    initializeMoveGenerator(PieceType::NONE, MoveTypes::ALL);
}

MoveGenerator::MoveGenerator(const GameContext& context, const TranspositionTable& tt, const Search& search, u32 ply) :
    m_toMove(context.readToPlay()),
    m_position(context.readChessboard().readPosition()),
    m_tt(&tt),
    m_search(&search),
    m_ply(ply),
    m_hashKey(context.readChessboard().readPosition().hash()),
    m_movesGenerated(false),
    m_moveCount(0),
    m_currentMoveIndx(0),
    m_movesBuffer()
{
    initializeMoveGenerator(PieceType::NONE, MoveTypes::ALL);
}

PrioratizedMove
MoveGenerator::generateNextMove() {
    if (m_currentMoveIndx < m_moveCount) {
        return m_movesBuffer[m_currentMoveIndx++];
    }

    if (m_movesGenerated)
        return { PackedMove::NullMove(), 0 };

    if (m_toMove == Set::WHITE) {
        return generateNextMove<Set::WHITE>();
    }
    else {
        return generateNextMove<Set::BLACK>();
    }
    return { PackedMove::NullMove(), 0 };
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
PrioratizedMove MoveGenerator::generateNextMove() {
    const size_t setIndx = static_cast<size_t>(set);
    if (m_moveMasks[setIndx].combine().empty()) {
        m_movesGenerated = true;
        return { PackedMove::NullMove(), 0 };
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

        sortMoves();

    }

    m_movesGenerated = true;
    if (m_currentMoveIndx < m_moveCount) {
        return m_movesBuffer[m_currentMoveIndx++];
    }

    return { PackedMove::NullMove(), 0 };
}

template<Set set>
void MoveGenerator::generateAllMoves() {
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
    sortMoves();
    m_movesGenerated = true;
}

void MoveGenerator::sortMoves() {
    if (m_tt != nullptr) {
        PackedMove pv = m_tt->probe(m_hashKey);
        if (pv != PackedMove::NullMove()) {
            auto itrMv = std::find_if(m_movesBuffer.begin(), m_movesBuffer.begin() + m_moveCount, [&](const PrioratizedMove& pm) {
                return pm.move == pv;
                });

            if (itrMv != m_movesBuffer.end()) {
                itrMv->priority += move_generator_constants::pvMovePriority;
            }
        }
    }

    if (m_search != nullptr) {
        for (u32 i = 0; i < m_moveCount; ++i) {
            auto& move = m_movesBuffer[i];
            if (m_search->isKillerMove(move.move, m_ply)) {
                move.priority += move_generator_constants::killerMovePriority;
                move.priority += m_search->getHistoryHeuristic(static_cast<u8>(m_toMove), move.move.source(), move.move.target());
            }
        }
    }

    PrioratizedMoveComparator comparator;
    std::sort(m_movesBuffer.begin(), m_movesBuffer.begin() + m_moveCount, comparator);
}

void MoveGenerator::forEachMove(std::function<void(const PrioratizedMove&)> callback) const {
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

    u16 promotionPriorityValue = move_generator_constants::promotionPriority << u8(move.isCapture());

    move.setPromoteTo(queenId);
    PrioratizedMove queenPromote(move, 0);
    queenPromote.setCheck(orthogonallyChecked || diagonallyChecked);
    queenPromote.priority = promotionPriorityValue;

    m_movesBuffer[m_moveCount] = queenPromote;
    m_moveCount++;

    move.setPromoteTo(rookId);
    PrioratizedMove rookPromote(move, 1);
    rookPromote.setCheck(orthogonallyChecked);
    rookPromote.priority = promotionPriorityValue;
    m_movesBuffer[m_moveCount] = rookPromote;
    m_moveCount++;

    move.setPromoteTo(bishopId);
    PrioratizedMove bishopPromote(move, 1);
    bishopPromote.setCheck(diagonallyChecked);
    bishopPromote.priority = promotionPriorityValue;
    m_movesBuffer[m_moveCount] = bishopPromote;
    m_moveCount++;

    // don't I need to check for check here?
    move.setPromoteTo(knightId);
    PrioratizedMove knigthPromote(move, 1);
    knigthPromote.priority = promotionPriorityValue;
    m_movesBuffer[m_moveCount] = knigthPromote;
    m_moveCount++;
}

template<Set set>
void MoveGenerator::internalGeneratePawnMoves()
{   
    const Bitboard movesbb = m_moveMasks[(size_t)set].material[pawnId];
    if (movesbb.empty())
        return;

    const KingPinThreats& pinThreats = m_pinThreats[static_cast<size_t>(set)];

    // cache pawns in local variable which we'll use to iterate over all pawns.
    Bitboard pawns = m_position.material().pawns<set>();

    while (pawns.empty() == false) {
        // build source square and remove pawn from pawns bitboard.
        const i32 srcSqr = pawns.popLsb();
        const Notation srcNotation(srcSqr);

        const u64 promotionMask = pawn_constants::promotionRank[(size_t)set];

        auto [isolatedPawnMoves, isolatedPawnAttacks] = internalIsolatePawn<set>(srcNotation, movesbb);
        while (isolatedPawnAttacks.empty() == false) {
            i32 dstSqr = isolatedPawnAttacks.popLsb();

            PrioratizedMove prioratizedMove;
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
                internalBuildPawnPromotionMoves(move, pinThreats, dstSqr);
            }
            else {
                Position checkedPos;
                checkedPos.edit().placePiece(ChessPiece(set, PieceType::PAWN), static_cast<Square>(dstSqr));
                Bitboard threat = checkedPos.read().material().topology<set>().computeThreatenedSquaresPawnBulk();
                if (threat & m_position.material().king<opposing_set<set>()>()) {
                    prioratizedMove.setCheck(true);
                    prioratizedMove.priority += move_generator_constants::checkPriority;
                }


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
                checkedPos.edit().placePiece(ChessPiece(set, PieceType::PAWN), static_cast<Square>(dstSqr));
                Bitboard threat = checkedPos.read().material().topology<set>().computeThreatenedSquaresPawnBulk();
                if (threat & m_position.material().king<opposing_set<set>()>()) {
                    prioratizedMove.setCheck(true);
                    prioratizedMove.priority += move_generator_constants::checkPriority;
                }

                m_movesBuffer[m_moveCount] = prioratizedMove;
                m_moveCount++;
            }
        }
    }
}

template void MoveGenerator::internalGeneratePawnMoves<Set::WHITE>();
template void MoveGenerator::internalGeneratePawnMoves<Set::BLACK>();

template<Set set>
void MoveGenerator::internalGenerateMoves(u8 pieceId, const KingPinThreats& pinThreats)
{   
    const Bitboard movesbb = m_moveMasks[(size_t)set].material[pieceId];
    if (movesbb.empty())
        return;

    Bitboard pieces = m_position.material().read<set>(pieceId);

    while (pieces.empty() == false) {
        // build source square and remove knight from cached material bitboard.
        const i32 srcSqr = pieces.popLsb();
        const Notation srcNotation(srcSqr);

        auto [isolatedMoves, isolatedCaptures] = bb.isolatePiece<set>(pieceId, srcNotation, movesbb, pinThreats);
        genPackedMovesFromBitboard(static_cast<u8>(set), pieceId, isolatedCaptures, srcSqr, /*are captures*/ true, pinThreats);
        genPackedMovesFromBitboard(static_cast<u8>(set), pieceId, isolatedMoves, srcSqr, /*are captures*/ false, pinThreats);
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
    const Bitboard opMaterial = m_position.material().combine<opposing_set<set>()>();
    const u8 setId = static_cast<u8>(set);

    Bitboard movesbb = m_moveMasks[setId].material[kingId];
#if defined EG_DEBUGGING || defined EG_TESTING
    // during testing and debugging king can be missing
    if (movesbb.empty())
        return;
#endif

    u32 srcSqr = m_position.material().king<set>().lsbIndex();
    u8 castlingRaw = m_position.castling().read() >> (setId * 2);

    while (movesbb.empty() == false) {
        i32 dstSqr = movesbb.popLsb();

        PrioratizedMove prioratizedMove;
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

template void MoveGenerator::internalGenerateKingMoves<Set::WHITE>();
template void MoveGenerator::internalGenerateKingMoves<Set::BLACK>();

void
MoveGenerator::initializeMoveGenerator(PieceType ptype, MoveTypes mtype) {
    bool captures = mtype == MoveTypes::CAPTURES_ONLY;
    const auto& bb = m_position;
    if (bb.empty())
        return;

    m_pinThreats[0] = computeKingPinThreats<Set::WHITE>();
    m_pinThreats[1] = computeKingPinThreats<Set::BLACK>();

    if (captures) {
        initializeMoveMasks<Set::WHITE, true>(m_moveMasks[0], ptype);
        initializeMoveMasks<Set::BLACK, true>(m_moveMasks[1], ptype);
    }
    else {
        initializeMoveMasks<Set::WHITE, false>(m_moveMasks[0], ptype);
        initializeMoveMasks<Set::BLACK, false>(m_moveMasks[1], ptype);
    }
}

template<Set set, bool captures>
void MoveGenerator::initializeMoveMasks(MaterialMask& target, PieceType ptype) {
    const auto& bb = m_position;
    if (bb.empty())
        return;
    const size_t setIndx = static_cast<size_t>(set);

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
MoveGenerator::genPackedMovesFromBitboard(u8 setId, u8 pieceId, Bitboard movesbb, i32 srcSqr, bool capture, const KingPinThreats& pinThreats)
{
    while (movesbb.empty() == false) {
        i32 dstSqr = movesbb.popLsb();

        PrioratizedMove prioratizedMove;
        PackedMove& move = prioratizedMove.move;
        move.setSource(static_cast<Square>(srcSqr));
        move.setTarget(static_cast<Square>(dstSqr));
        move.setCapture(capture);
        prioratizedMove.priority = 0;

        if (capture) {
            const u8 opId = opposing_set(setId);
            u8 recaptureBonus = ((m_moveMasks[opId].combine() & squareMaskTable[dstSqr]).empty() == false) ? 2 : 1;
            prioratizedMove.priority = move_generator_constants::capturePriority * u8(capture) * recaptureBonus;
        }

        // figure out if we're checking the king.
        if (pieceId == rookId || pieceId == queenId) {
            if (pinThreats.readOpponentOpenAngles()[0] & squareMaskTable[dstSqr]) {
                prioratizedMove.setCheck(true);
                prioratizedMove.priority += move_generator_constants::checkPriority;
            }
        }
        else if (pieceId == bishopId || pieceId == queenId) {
            if (pinThreats.readOpponentOpenAngles()[1] & squareMaskTable[dstSqr]) {
                prioratizedMove.setCheck(true);
                prioratizedMove.priority += move_generator_constants::checkPriority;
            }
        }
        m_movesBuffer[m_moveCount] = prioratizedMove;
        m_moveCount++;
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

template<Set us>
std::tuple<Bitboard, Bitboard>
MoveGenerator::isolatePiece(u8 pieceId, Notation source, Bitboard movesbb, const KingPinThreats& kingMask) const
{
    switch (pieceId) {
    case pawnId:
        return internalIsolatePawn<us>(source, movesbb, kingMask);
    case bishopId:
        return internalIsolateBishop<us>(source, movesbb, kingMask);
    case rookId:
        return internalIsolateRook<us>(source, movesbb, kingMask);
    case knightId:
        return internalIsolateKnightMoves<us>(source, movesbb, kingMask);
    case queenId: {
        auto [diags, diagCaptures] = internalIsolateBishop<us>(source, movesbb, kingMask, queenId);
        auto [orthos, orthoCaptures] = internalIsolateRook<us>(source, movesbb, kingMask, queenId);
        return { diags | orthos, diagCaptures | orthoCaptures };
    }
    default:
        FATAL_ASSERT(false) << "Not implemented";
    }

    return { 0, 0 };
}

template std::tuple<Bitboard, Bitboard> MoveGenerator::isolatePiece<Set::WHITE>(u8, Notation, Bitboard, const KingPinThreats&) const;
template std::tuple<Bitboard, Bitboard> MoveGenerator::isolatePiece<Set::BLACK>(u8, Notation, Bitboard, const KingPinThreats&) const;

template<Set us>
std::tuple<Bitboard, Bitboard> MoveGenerator::isolatePawn(Square source, Bitboard movesbb) const
{
    const size_t usIndx = static_cast<size_t>(us);
    const KingPinThreats& pinThreats = m_pinThreats[usIndx];

    Bitboard opMatCombined = m_position.material().combine<opposing_set<us>()>() | m_position.enPassant().readBitboard();
    Bitboard srcMask = Bitboard(squareMaskTable[*source]);

    const Bitboard pinned = pinThreats.pinned(srcMask);

    // special case for when there is a enpassant available.
    if (m_position.enPassant()) {
        // Bitboard enPassantTarget(squareMaskTable[(int)m_enpassantState.readTarget()]);
        Bitboard potentialPin(pinThreats.readEnPassantMask() & srcMask);
        if (potentialPin) {
            opMatCombined ^= m_position.enPassant().readBitboard();
        }
    }

    Bitboard threatns;
    if ((srcMask & board_constants::boundsRelativeMasks[usIndx][west]).empty())
        threatns |= srcMask.shiftNorthWestRelative<us>();
    if ((srcMask & board_constants::boundsRelativeMasks[usIndx][east]).empty())
        threatns |= srcMask.shiftNorthEastRelative<us>();

    Bitboard isolatedbb = srcMask.shiftNorthRelative<us>();
    Bitboard unoccupied = ~(m_position.material().combine<us>() | opMatCombined);
    Bitboard doublePush = isolatedbb & pawn_constants::baseRank[usIndx] & unoccupied;
    isolatedbb |= doublePush.shiftNorthRelative<us>();
    isolatedbb &= unoccupied;
    if (srcMask & pinned) {
        isolatedbb &= pinned;
        threatns &= pinned;
    }
    return { movesbb & isolatedbb, movesbb & opMatCombined & threatns };
}

template std::tuple<Bitboard, Bitboard> MoveGenerator::isolatePawn<Set::WHITE>(Square, Bitboard) const;
template std::tuple<Bitboard, Bitboard> MoveGenerator::isolatePawn<Set::BLACK>(Square, Bitboard) const;

template<Set us>
std::tuple<Bitboard, Bitboard>
MoveGenerator::internalIsolateKnightMoves(Notation source, Bitboard movesbb, const KingPinThreats& kingMask) const
{
    Bitboard opMatCombined = m_position.material().combine<opposing_set<us>()>();

    // figure out if piece is pinned
    u64 srcMask = squareMaskTable[source.index()];
    const Bitboard pinned = kingMask.pinned(srcMask);
    if (pinned.empty() == false) {
        movesbb &= pinned;
    }

    movesbb &= attacks::getKnightAttacks(source.index());
    movesbb &= ~m_position.material().combine<us>();

    return { movesbb & ~opMatCombined, movesbb & opMatCombined };
}

template<Set us>
std::tuple<Bitboard, Bitboard>
MoveGenerator::internalIsolateBishop(Notation source, Bitboard movesbb, const KingPinThreats& kingMask, i8) const
{
    const Bitboard opMatCombined = m_position.material().combine<opposing_set<us>()>();
    const Bitboard allMaterial = m_position.material().combine();
    const Bitboard usMaterial = opMatCombined ^ allMaterial;

    // figure out if piece is pinned
    u64 srcMask = squareMaskTable[source.index()];
    const Bitboard pinned = kingMask.pinned(srcMask);
    if (pinned.empty() == false) {
        movesbb &= pinned;
    }

    movesbb &= attacks::getBishopAttacks(source.index(), allMaterial.read());
    movesbb &= ~usMaterial;

    return { movesbb & ~opMatCombined, movesbb & opMatCombined };
}

template std::tuple<Bitboard, Bitboard> MoveGenerator::internalIsolateBishop<Set::WHITE>(Notation, Bitboard, const KingPinThreats&,
    i8) const;
template std::tuple<Bitboard, Bitboard> MoveGenerator::internalIsolateBishop<Set::BLACK>(Notation, Bitboard, const KingPinThreats&,
    i8) const;

template<Set us>
std::tuple<Bitboard, Bitboard>
MoveGenerator::internalIsolateRook(Notation source, Bitboard movesbb, const KingPinThreats& kingMask, i8) const {
    const Bitboard opMatCombined = m_position.material().combine<opposing_set<us>()>();
    const Bitboard allMaterial = m_position.material().combine();
    const Bitboard usMaterial = opMatCombined ^ allMaterial;

    u64 srcMask = squareMaskTable[source.index()];
    Bitboard pinned = kingMask.pinned(srcMask);
    if (pinned.empty() == false) {
        movesbb &= pinned;
    }

    movesbb &= attacks::getRookAttacks(source.index(), allMaterial.read());
    movesbb &= ~usMaterial;

    return { movesbb & ~opMatCombined, movesbb & opMatCombined };
}

template std::tuple<Bitboard, Bitboard> MoveGenerator::internalIsolateRook<Set::WHITE>(Notation, Bitboard, const KingPinThreats&,
    i8) const;
template std::tuple<Bitboard, Bitboard> MoveGenerator::internalIsolateRook<Set::BLACK>(Notation, Bitboard, const KingPinThreats&,
    i8) const;