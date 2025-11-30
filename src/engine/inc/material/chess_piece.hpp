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
#include <system/platform.hpp>
#include <material/chess_piece_defines.hpp>

struct ChessPiece {
public:
    static Set FlipSet(Set source);
    static byte FlipSet(byte source);
    static constexpr ChessPiece None() { return ChessPiece(); }
private:

public:
    constexpr ChessPiece() : m_internalState(0x00) {}
    constexpr ChessPiece(Set _set, PieceType _type)
        : m_internalState(0x00)
    {
        m_internalState |= (byte)_set << 7;
        m_internalState |= (byte)_type;
    }   

    char toString() const;
    bool fromString(char piece);   

    bool isPawn() const { return getType() == PieceType::PAWN; }
    bool isKnight() const { return getType() == PieceType::KNIGHT; }
    bool isBishop() const { return getType() == PieceType::BISHOP; }
    bool isRook() const { return getType() == PieceType::ROOK; }
    bool isQueen() const { return getType() == PieceType::QUEEN; }
    bool isKing() const { return getType() == PieceType::KING; }
    bool isWhite() const { return getSet() == Set::WHITE; }
    bool isBlack() const { return getSet() == Set::BLACK; }
    bool isSliding() const { return piece_constants::slides[index()]; }

    bool operator==(const ChessPiece& rhs) const;
    bool operator!=(const ChessPiece& rhs) const;
    bool operator<(const ChessPiece& rhs) const;

    PieceType getType() const { return static_cast<PieceType>(m_internalState & 0x07); }
    Set getSet() const { return static_cast<Set>(m_internalState >> 7); }

    /**
     * @brief Returns the PieceType as a number. Does not correlate to the type index.     */
    inline byte typeId() const { return m_internalState & 0x07; }
    inline byte set() const { return m_internalState >> 7; }
    byte index() const { return typeId() - 1; }
    byte raw() const { return m_internalState; }

    bool isValid() const { return typeId() > 0; }    

private:
    // [set][not used][not used][not used][not used][piece t][piece t][piece t]
    byte m_internalState;
};

namespace piece_constants {
constexpr ChessPiece white_pawn(Set::WHITE, PieceType::PAWN);
constexpr ChessPiece black_pawn(Set::BLACK, PieceType::PAWN);
constexpr ChessPiece white_knight(Set::WHITE, PieceType::KNIGHT);
constexpr ChessPiece black_knight(Set::BLACK, PieceType::KNIGHT);
constexpr ChessPiece white_bishop(Set::WHITE, PieceType::BISHOP);
constexpr ChessPiece black_bishop(Set::BLACK, PieceType::BISHOP);
constexpr ChessPiece white_rook(Set::WHITE, PieceType::ROOK);
constexpr ChessPiece black_rook(Set::BLACK, PieceType::ROOK);
constexpr ChessPiece white_queen(Set::WHITE, PieceType::QUEEN);
constexpr ChessPiece black_queen(Set::BLACK, PieceType::QUEEN);
constexpr ChessPiece white_king(Set::WHITE, PieceType::KING);
constexpr ChessPiece black_king(Set::BLACK, PieceType::KING);

constexpr ChessPiece pieces[2][6] = {
    { white_pawn, white_knight, white_bishop, white_rook, white_queen, white_king },
    { black_pawn, black_knight, black_bishop, black_rook, black_queen, black_king }
};

constexpr ChessPiece null() { return ChessPiece::None();}

} // namespace piece_constants

#define BLACKKING piece_constants::black_king
#define WHITEKING piece_constants::white_king
#define BLACKPAWN piece_constants::black_pawn
#define WHITEPAWN piece_constants::white_pawn
#define BLACKBISHOP piece_constants::black_bishop
#define WHITEBISHOP piece_constants::white_bishop
#define BLACKROOK piece_constants::black_rook
#define WHITEROOK piece_constants::white_rook
#define BLACKQUEEN piece_constants::black_queen
#define WHITEQUEEN piece_constants::white_queen
#define BLACKKNIGHT piece_constants::black_knight
#define WHITEKNIGHT piece_constants::white_knight
