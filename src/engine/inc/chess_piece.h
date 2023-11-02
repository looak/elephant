// Elephant Gambit Chess Engine - a Chess AI
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
    static ChessPiece None() { return ChessPiece(); }

public:
    ChessPiece();
    ChessPiece(Set _set, PieceType _type);

    char toString() const;
    bool fromString(char piece);

    bool isPawn() const { return type() == 1; }
    bool isKnight() const { return type() == 2; }
    bool isBishop() const { return type() == 3; }
    bool isRook() const { return type() == 4; }
    bool isQueen() const { return type() == 5; }
    bool isKing() const { return type() == 6; }
    bool isWhite() const { return set() == 0; }

    bool operator==(const ChessPiece& rhs) const;
    bool operator!=(const ChessPiece& rhs) const;
    bool operator<(const ChessPiece& rhs) const;

    PieceType getType() const { return static_cast<PieceType>(m_internalState & 0x07); }
    Set getSet() const { return static_cast<Set>(m_internalState >> 7); }
    inline byte type() const { return m_internalState & 0x07; }
    inline byte set() const { return m_internalState >> 7; }
    byte index() const { return type() - 1; }
    byte raw() const { return m_internalState; }

    bool isValid() const { return type() > 0; }
    bool isSliding() const { return ChessPieceDef::Slides(index()); }

private:
    // [set][not used][not used][not used][not used][piece t][piece t][piece t]
    byte m_internalState;
};