﻿// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021  Alexander Loodin Ek

// This program is free software : you can redistribute it and /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.If not, see < http://www.gnu.org/licenses/>.
#pragma once
#include "chess_piece_defines.hpp"
#include "defines.h"

struct ChessPiece {
public:
    static Set FlipSet(Set source);
    static byte FlipSet(byte source);
    static ChessPiece None() { return s_empty; }
private:
    static ChessPiece s_empty;

public:
    ChessPiece();
    constexpr ChessPiece(Set _set, PieceType _type)
        : m_internalState(0x00)
    {
        m_internalState |= (byte)_set << 7;
        m_internalState |= (byte)_type;
    }

    ChessPiece(byte setId, byte typeId);

    char toString() const;
    bool fromString(char piece);

    // todo cleanup typeid, typeindx and typeenum

    bool isPawn() const { return typeId() == 1; }
    bool isKnight() const { return typeId() == 2; }
    bool isBishop() const { return typeId() == 3; }
    bool isRook() const { return typeId() == 4; }
    bool isQueen() const { return typeId() == 5; }
    bool isKing() const { return typeId() == 6; }
    bool isWhite() const { return set() == 0; }
    bool isBlack() const { return set() == 1; }

    bool operator==(const ChessPiece& rhs) const;
    bool operator!=(const ChessPiece& rhs) const;
    bool operator<(const ChessPiece& rhs) const;

    PieceType getType() const { return static_cast<PieceType>(m_internalState & 0x07); }
    Set getSet() const { return static_cast<Set>(m_internalState >> 7); }
    inline byte typeId() const { return m_internalState & 0x07; }
    inline byte set() const { return m_internalState >> 7; }
    byte index() const { return typeId() - 1; }
    byte raw() const { return m_internalState; }

    bool isValid() const { return typeId() > 0; }
    bool isSliding() const { return ChessPieceDef::Slides(index()); }

private:
    // [set][not used][not used][not used][not used][piece t][piece t][piece t]
    byte m_internalState;
};

constexpr ChessPiece WhitePawn(Set::WHITE, PieceType::PAWN);
constexpr ChessPiece BlackPawn(Set::BLACK, PieceType::PAWN);
constexpr ChessPiece WhiteKnight(Set::WHITE, PieceType::KNIGHT);
constexpr ChessPiece BlackKnight(Set::BLACK, PieceType::KNIGHT);
constexpr ChessPiece WhiteBishop(Set::WHITE, PieceType::BISHOP);
constexpr ChessPiece BlackBishop(Set::BLACK, PieceType::BISHOP);
constexpr ChessPiece WhiteRook(Set::WHITE, PieceType::ROOK);
constexpr ChessPiece BlackRook(Set::BLACK, PieceType::ROOK);
constexpr ChessPiece WhiteQueen(Set::WHITE, PieceType::QUEEN);
constexpr ChessPiece BlackQueen(Set::BLACK, PieceType::QUEEN);
constexpr ChessPiece WhiteKing(Set::WHITE, PieceType::KING);
constexpr ChessPiece BlackKing(Set::BLACK, PieceType::KING);

constexpr ChessPiece s_pieces[2][6] = {
    {WhitePawn, WhiteKnight, WhiteBishop, WhiteRook, WhiteQueen, WhiteKing},
    {BlackPawn, BlackKnight, BlackBishop, BlackRook, BlackQueen, BlackKing}
};