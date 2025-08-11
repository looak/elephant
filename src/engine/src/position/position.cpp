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