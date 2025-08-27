#include "position/position.hpp"
#include <array>
#include <bitboard/attacks/attacks.hpp>
#include <bitboard/bitboard.hpp>
#include <material/chess_piece.hpp>
#include <debug/log.hpp>

#include <position/hash_zobrist.hpp>

Position::Position() :
    m_castlingState(),
    m_enpassantState()
{
    m_materialMask = {};
}

Position::Position(const Position& other) :
    m_materialMask(other.m_materialMask),
    m_castlingState(other.m_castlingState),
    m_enpassantState(other.m_enpassantState),
    m_hash(other.m_hash)
{}

Position&
Position::operator=(const Position& other)
{
    m_materialMask = other.m_materialMask;
    m_castlingState = other.m_castlingState;
    m_enpassantState = other.m_enpassantState;
    m_hash = other.m_hash;
    return *this;
}

bool Position::operator==(const Position& other) const
{
    return m_materialMask == other.m_materialMask &&
           m_castlingState == other.m_castlingState &&
           m_enpassantState == other.m_enpassantState &&
           m_hash == other.m_hash;
}