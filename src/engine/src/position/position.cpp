#include "position/position.hpp"
#include <array>
#include "attacks/attacks.hpp"
#include "bitboard.hpp"
#include "chess_piece.h"
#include "log.h"
#include "notation.h"
#include <position/hash_zorbist.hpp>

std::string
CastlingStateInfo::toString() const
{
    std::string result;
    if (hasWhiteKingSide())
        result += "K";
    if (hasWhiteQueenSide())
        result += "Q";
    if (hasBlackKingSide())
        result += "k";
    if (hasBlackQueenSide())
        result += "q";

    if (result.empty())
        result = "-";

    return result;
}

Position::Position() :
    m_castlingState(),
    m_enpassantState()
{
    m_materialMask = {};
}

Position&
Position::operator=(const Position& other)
{
    m_materialMask = other.m_materialMask;
    m_castlingState = other.m_castlingState;
    m_enpassantState = other.m_enpassantState;
    return *this;
}

bool
Position::IsValidSquare(signed short currSqr)
{
    if (currSqr < 0)
        return false;
    if (currSqr > 63) {
        // LOG_ERROR() << "In case index is larger than 127 it will wrap around our board.";
        return false;
    }

    byte sq0x88 = currSqr + (currSqr & ~7);
    return (sq0x88 & 0x88) == 0;
}

bool
Position::IsValidSquare(Notation source)
{
    return Position::IsValidSquare(source.index());
}

void
Position::Clear()
{
    m_materialMask.clear();
    m_enpassantState = {};
    m_castlingState = {};
}

bool
Position::empty() const
{
    return m_materialMask.empty();
}

MutableMaterialProxy
Position::materialEditor(Set set, PieceType pType)
{
    return MutableMaterialProxy(&m_materialMask.m_set[static_cast<i8>(set)], &m_materialMask.m_material[toPieceId(pType)]);
}

u64
Position::Castling(byte set, byte castling, u64 threatenedMask) const
{
    u64 retVal = ~universe;
    byte rank = 0;
    if (set == 1)  // BLACK
    {
        rank = 7;
        // shift castling right
        // this should make black caslting 1 & 2
        castling = castling >> 2;
    }

    // early out in case we don't have any castling available to us.
    if (castling == 0)
        return retVal;

    u64 attacked = threatenedMask;
    u64 combMat = m_materialMask.combine().read();

    // check king side
    if (castling & 1) {
        // build castling square mask
        byte fsqr = (rank * 8) + 5;
        byte gsqr = fsqr + 1;
        u64 mask = ~universe;
        mask |= squareMaskTable[fsqr];
        mask |= squareMaskTable[gsqr];

        if (!(attacked & mask) && !(combMat & mask))
            retVal |= squareMaskTable[gsqr];
    }
    // check queen side
    if (castling & 2) {
        // build castling square mask
        byte bsqr = (rank * 8) + 1;
        byte csqr = bsqr + 1;
        byte dsqr = csqr + 1;
        u64 threatMask = ~universe;
        u64 blockedMask = ~universe;
        threatMask |= squareMaskTable[csqr];
        threatMask |= squareMaskTable[dsqr];

        blockedMask |= threatMask;
        blockedMask |= squareMaskTable[bsqr];

        if (!(attacked & threatMask) && !(combMat & blockedMask))
            retVal |= squareMaskTable[csqr];
    }
    return retVal;
}

template<Set us, bool captures>
Bitboard Position::calcAvailableMovesPawnBulk(const KingPinThreats& kingMask) const {
    const size_t usIndx = static_cast<size_t>(us);
    const Bitboard usMat = m_materialMask.combine<us>();
    const Bitboard opMat = m_materialMask.combine<opposing_set<us>()>();
    const Bitboard unoccupied(~(usMat | opMat));
    const Bitboard piecebb = m_materialMask.pawns<us>();

    Bitboard mvsbb;
    mvsbb = piecebb.shiftNorthRelative<us>();
    Bitboard doublePush = mvsbb & pawn_constants::baseRank[usIndx] & unoccupied;
    mvsbb |= doublePush.shiftNorthRelative<us>();

    mvsbb &= unoccupied;

    mvsbb |= (opMat | m_enpassantState.readBitboard()) & calcThreatenedSquaresPawnBulk<us>();

    if (kingMask.isChecked()) {
        Bitboard checksMask(kingMask.checks());
        auto otherMask = squareMaskTable[(u32)m_enpassantState.readTarget()];
        if (checksMask & otherMask) {
            checksMask |= m_enpassantState.readBitboard();
        }
        mvsbb &= checksMask;
    }

    if constexpr (captures == true)
        mvsbb &= opMat;

    return mvsbb;
}

template Bitboard Position::calcAvailableMovesPawnBulk<Set::WHITE, true>(const KingPinThreats& kingMask) const;
template Bitboard Position::calcAvailableMovesPawnBulk<Set::WHITE, false>(const KingPinThreats& kingMask) const;
template Bitboard Position::calcAvailableMovesPawnBulk<Set::BLACK, true>(const KingPinThreats& kingMask) const;
template Bitboard Position::calcAvailableMovesPawnBulk<Set::BLACK, false>(const KingPinThreats& kingMask) const;

template<Set us, bool captures, Set op>
Bitboard
Position::calcAvailableMovesKing(byte castlingRights) const
{
    bool constexpr includeMaterial = false;
    bool constexpr pierceKing = true;
    Bitboard threatened = calcThreatenedSquares<op, includeMaterial, pierceKing>();
    Bitboard moves = calcThreatenedSquaresKing<us>();
    // remove any squares blocked by our own pieces.
    moves &= ~m_materialMask.combine<us>();
    moves &= ~threatened;
    if ((threatened & m_materialMask.king<us>()).empty())
        moves |= Castling((byte)us, castlingRights, threatened.read());
    if constexpr (captures == true)
        moves &= m_materialMask.combine<op>();
    return moves;
}

template Bitboard Position::calcAvailableMovesKing<Set::WHITE, true, Set::BLACK>(byte) const;
template Bitboard Position::calcAvailableMovesKing<Set::WHITE, false, Set::BLACK>(byte) const;
template Bitboard Position::calcAvailableMovesKing<Set::BLACK, true, Set::WHITE>(byte) const;
template Bitboard Position::calcAvailableMovesKing<Set::BLACK, false, Set::WHITE>(byte) const;


template<Set us, bool captures, u8 pieceId>
Bitboard
Position::calcAvailableMovesBishopBulk(const KingPinThreats& kingMask) const
{
    const Bitboard materialbb = readMaterial().combine<us>();
    Bitboard moves = calcThreatenedSquaresBishopBulk<us, pieceId>();

    if (kingMask.isChecked())
        moves &= kingMask.checks();
    else
        moves ^= (materialbb & moves);

    if constexpr (captures == true)
        moves &= readMaterial().combine<opposing_set<us>()>();

    return moves;
}

template Bitboard Position::calcAvailableMovesBishopBulk<Set::WHITE, true, bishopId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesBishopBulk<Set::BLACK, true, bishopId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesBishopBulk<Set::WHITE, true, queenId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesBishopBulk<Set::BLACK, true, queenId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesBishopBulk<Set::WHITE, false, bishopId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesBishopBulk<Set::BLACK, false, bishopId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesBishopBulk<Set::WHITE, false, queenId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesBishopBulk<Set::BLACK, false, queenId>(const KingPinThreats&) const;

template<Set us, bool captures, u8 pieceId>
Bitboard
Position::calcAvailableMovesRookBulk(const KingPinThreats& kingMask) const
{
    const Bitboard materialbb = readMaterial().combine<us>();

    Bitboard moves = calcThreatenedSquaresRookBulk<us, pieceId>();

    if (kingMask.isChecked())
        moves &= kingMask.checks();
    else
        moves ^= (materialbb & moves);

    if constexpr (captures == true)
        moves &= readMaterial().combine<opposing_set<us>()>();

    return moves;
}

template Bitboard Position::calcAvailableMovesRookBulk<Set::WHITE, true, rookId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesRookBulk<Set::BLACK, true, rookId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesRookBulk<Set::WHITE, true, queenId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesRookBulk<Set::BLACK, true, queenId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesRookBulk<Set::WHITE, false, rookId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesRookBulk<Set::BLACK, false, rookId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesRookBulk<Set::WHITE, false, queenId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesRookBulk<Set::BLACK, false, queenId>(const KingPinThreats&) const;

template<Set us, bool captures>
Bitboard
Position::calcAvailableMovesQueenBulk(const KingPinThreats& kingMask) const
{
    Bitboard moves = 0;
    moves |= calcAvailableMovesBishopBulk<us, captures, queenId>(kingMask);
    moves |= calcAvailableMovesRookBulk<us, captures, queenId>(kingMask);
    return moves;
}

template Bitboard Position::calcAvailableMovesQueenBulk<Set::WHITE, true>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesQueenBulk<Set::WHITE, false>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesQueenBulk<Set::BLACK, true>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesQueenBulk<Set::BLACK, false>(const KingPinThreats&) const;

template<Set us, bool captures>
Bitboard
Position::calcAvailableMovesKnightBulk(const KingPinThreats& kingMask) const
{
    auto moves = calcThreatenedSquaresKnightBulk<us>();
    const Bitboard ourMaterial = readMaterial().combine<us>();

    moves &= ~ourMaterial;

    if (kingMask.isChecked())
        return moves & kingMask.checks();

    if constexpr (captures == true)
        return moves & readMaterial().combine<opposing_set<us>()>();

    return moves;
}

template Bitboard Position::calcAvailableMovesKnightBulk<Set::WHITE, true>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesKnightBulk<Set::BLACK, true>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesKnightBulk<Set::WHITE, false>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesKnightBulk<Set::BLACK, false>(const KingPinThreats&) const;


template<Set us>
std::tuple<Bitboard, Bitboard>
Position::isolatePiece(u8 pieceId, Notation source, Bitboard movesbb, const KingPinThreats& kingMask) const
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

template std::tuple<Bitboard, Bitboard> Position::isolatePiece<Set::WHITE>(u8, Notation, Bitboard, const KingPinThreats&) const;
template std::tuple<Bitboard, Bitboard> Position::isolatePiece<Set::BLACK>(u8, Notation, Bitboard, const KingPinThreats&) const;

template<Set us>
std::tuple<Bitboard, Bitboard>
Position::internalIsolatePawn(Notation source, Bitboard movesbb, const KingPinThreats& pinThreats) const
{
    const size_t usIndx = static_cast<size_t>(us);

    Bitboard opMatCombined = readMaterial().combine<opposing_set<us>()>() | m_enpassantState.readBitboard();
    Bitboard srcMask = Bitboard(squareMaskTable[source.index()]);

    const Bitboard pinned = pinThreats.pinned(srcMask);

    // special case for when there is a enpassant available.
    if (m_enpassantState) {
        // Bitboard enPassantTarget(squareMaskTable[(int)m_enpassantState.readTarget()]);
        Bitboard potentialPin(pinThreats.readEnPassantMask() & srcMask);
        if (potentialPin) {
            opMatCombined ^= m_enpassantState.readBitboard();
        }
    }

    Bitboard threatns;
    if ((srcMask & board_constants::boundsRelativeMasks[usIndx][west]).empty())
        threatns |= srcMask.shiftNorthWestRelative<us>();
    if ((srcMask & board_constants::boundsRelativeMasks[usIndx][east]).empty())
        threatns |= srcMask.shiftNorthEastRelative<us>();

    Bitboard isolatedbb = srcMask.shiftNorthRelative<us>();
    Bitboard unoccupied = ~(readMaterial().combine<us>() | opMatCombined);
    Bitboard doublePush = isolatedbb & pawn_constants::baseRank[usIndx] & unoccupied;
    isolatedbb |= doublePush.shiftNorthRelative<us>();
    isolatedbb &= unoccupied;
    if (srcMask & pinned) {
        isolatedbb &= pinned;
        threatns &= pinned;
    }
    return { movesbb & isolatedbb, movesbb & opMatCombined & threatns };
}

template std::tuple<Bitboard, Bitboard> Position::internalIsolatePawn<Set::WHITE>(Notation, Bitboard,
    const KingPinThreats&) const;
template std::tuple<Bitboard, Bitboard> Position::internalIsolatePawn<Set::BLACK>(Notation, Bitboard,
    const KingPinThreats&) const;

template<Set us>
std::tuple<Bitboard, Bitboard>
Position::internalIsolateKnightMoves(Notation source, Bitboard movesbb, const KingPinThreats& kingMask) const
{
    Bitboard opMatCombined = readMaterial().combine<opposing_set<us>()>();

    // figure out if piece is pinned
    u64 srcMask = squareMaskTable[source.index()];
    const Bitboard pinned = kingMask.pinned(srcMask);
    if (pinned.empty() == false) {
        movesbb &= pinned;
    }

    movesbb &= attacks::getKnightAttacks(source.index());
    movesbb &= ~readMaterial().combine<us>();

    return { movesbb & ~opMatCombined, movesbb & opMatCombined };
}

template<Set us>
std::tuple<Bitboard, Bitboard>
Position::internalIsolateBishop(Notation source, Bitboard movesbb, const KingPinThreats& kingMask, i8) const
{
    const Bitboard opMatCombined = readMaterial().combine<opposing_set<us>()>();
    const Bitboard allMaterial = readMaterial().combine();
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

template std::tuple<Bitboard, Bitboard> Position::internalIsolateBishop<Set::WHITE>(Notation, Bitboard, const KingPinThreats&,
    i8) const;
template std::tuple<Bitboard, Bitboard> Position::internalIsolateBishop<Set::BLACK>(Notation, Bitboard, const KingPinThreats&,
    i8) const;

template<Set us>
std::tuple<Bitboard, Bitboard>
Position::internalIsolateRook(Notation source, Bitboard movesbb, const KingPinThreats& kingMask, i8) const {
    const Bitboard opMatCombined = readMaterial().combine<opposing_set<us>()>();
    const Bitboard allMaterial = readMaterial().combine();
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

template std::tuple<Bitboard, Bitboard> Position::internalIsolateRook<Set::WHITE>(Notation, Bitboard, const KingPinThreats&,
    i8) const;
template std::tuple<Bitboard, Bitboard> Position::internalIsolateRook<Set::BLACK>(Notation, Bitboard, const KingPinThreats&,
    i8) const;