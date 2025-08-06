#include "position/position.hpp"
#include <array>
#include "attacks/attacks.hpp"
#include "bitboard.hpp"
#include <material/chess_piece.hpp>
#include "log.h"
#include "notation.h"
#include <position/hash_zorbist.hpp>

Position::Position() :
    m_castlingState(),
    m_enpassantState()
{
    m_materialMask = {};
}

Position::Position(const Position& other) :
    m_materialMask(other.m_materialMask),
    m_castlingState(other.m_castlingState),
    m_enpassantState(other.m_enpassantState)
{}

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