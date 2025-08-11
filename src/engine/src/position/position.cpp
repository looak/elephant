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
