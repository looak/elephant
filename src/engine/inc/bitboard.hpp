// Elephant Gambit Chess Engine - a Chess Engine & AI
// Copyright(C) 2023  Alexander Loodin Ek

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
#ifndef BITBOARD_HEADER
#define BITBOARD_HEADER

#include "bitboard_constants.h"
#include "chess_piece_defines.hpp"
#include "defines.h"

class BitboardSquare {
    friend class Bitboard;

public:
    constexpr void operator=(bool value)
    {
        if (value) {
            m_bb |= squareMaskTable[static_cast<u8>(m_sqr)];
        }
        else {
            m_bb &= ~squareMaskTable[static_cast<u8>(m_sqr)];
        }
    }

    constexpr operator bool() const { return (m_bb & squareMaskTable[static_cast<u8>(m_sqr)]) != 0; }

private:
    constexpr BitboardSquare(u64& bb, Square sqr) :
        m_bb(bb),
        m_sqr(sqr)
    {
    }

    u64& m_bb;
    Square m_sqr;
};

class Bitboard {
public:
    constexpr Bitboard(u64 board) :
        m_board(board)
    {
    }

    constexpr Bitboard(const Bitboard& rhs) :
        m_board(rhs.m_board)
    {
    }

    constexpr Bitboard() :
        m_board(0)
    {
    }

    /**
     * @brief returns the internal 64bit integer    */
    [[nodiscard]] constexpr u64 read() const { return m_board; }

#pragma region operators
    constexpr Bitboard& operator=(const Bitboard& rhs)
    {
        m_board = rhs.m_board;
        return *this;
    }
    constexpr Bitboard& operator|=(const Bitboard& rhs)
    {
        m_board |= rhs.m_board;
        return *this;
    }
    constexpr Bitboard& operator&=(const Bitboard& rhs)
    {
        m_board &= rhs.m_board;
        return *this;
    }
    constexpr Bitboard& operator^=(const Bitboard& rhs)
    {
        m_board ^= rhs.m_board;
        return *this;
    }
    [[nodiscard]] constexpr Bitboard operator|(const Bitboard& rhs) const { return Bitboard(m_board | rhs.m_board); }
    [[nodiscard]] constexpr Bitboard operator&(const Bitboard& rhs) const { return Bitboard(m_board & rhs.m_board); }
    [[nodiscard]] constexpr Bitboard operator^(const Bitboard& rhs) const { return Bitboard(m_board ^ rhs.m_board); }
    [[nodiscard]] constexpr Bitboard operator~() const { return Bitboard(~m_board); };

    [[nodiscard]] constexpr bool operator==(const Bitboard& rhs) const { return m_board == rhs.m_board; }
    [[nodiscard]] constexpr bool operator!=(const Bitboard& rhs) const { return !(*this == rhs); }

    /**
     * @brief returns true if the square is set in the bitboard    */
    constexpr bool operator[](Square sqr) const { return (m_board & squareMaskTable[static_cast<u8>(sqr)]) != 0; }

    constexpr BitboardSquare operator[](Square sqr) { return BitboardSquare(m_board, sqr); }
#pragma endregion  // operators

#pragma region relative shifts
    // template<Set s>
    // [[nodiscard]] constexpr u64 shiftNorthRelative(u64 bb);

    // template<Set s>
    // [[nodiscard]] constexpr u64 shiftEastRelative(u64 bb);

    // template<Set s>
    // [[nodiscard]] constexpr u64 shiftSouthRelative(u64 bb);

    // template<Set s>
    // [[nodiscard]] constexpr u64 shiftWestRelative(u64 bb);

    // template<Set s>
    // [[nodiscard]] constexpr u64 shiftNorthEastRelative(u64 bb);

    // template<Set s>
    // [[nodiscard]] constexpr u64 shiftSouthEastRelative(u64 bb);

    // template<Set s>
    // [[nodiscard]] constexpr u64 shiftSouthWestRelative(u64 bb);

    // template<Set s>
    // [[nodiscard]] constexpr u64 shiftNorthWestRelative(u64 bb);

    // template<Set s, u8 direction>
    // [[nodiscard]] constexpr u64 shiftRelative(u64 bb);
#pragma endregion  // relative shifts

#pragma region fill
    constexpr void inclusiveFillWest(i16 file);
    // constexpr void inclusiveFillEast(i16 file);
    // constexpr void inclusiveFillSouth(i16 rank);
    // constexpr void inclusiveFillNorth(i16 rank);
    // constexpr void inclusiveFillNorthEast(i16 file, i16 rank);
    // constexpr void inclusiveFillSouthEast(i16 file, i16 rank);
    // constexpr void inclusiveFillSouthWest(i16 file, i16 rank);
    // constexpr void inclusiveFillNorthWest(i16 file, i16 rank);
#pragma endregion  // fill

private:
#pragma region private operators
    [[nodiscard]] constexpr Bitboard& operator|=(u64 rhs)
    {
        m_board |= rhs;
        return *this;
    }
    [[nodiscard]] constexpr Bitboard& operator&=(u64 rhs)
    {
        m_board &= rhs;
        return *this;
    }
    [[nodiscard]] constexpr Bitboard& operator^=(u64 rhs)
    {
        m_board ^= rhs;
        return *this;
    }
    [[nodiscard]] constexpr Bitboard operator|(u64 rhs) const { return Bitboard(m_board | rhs); }
    [[nodiscard]] constexpr Bitboard operator&(u64 rhs) const { return Bitboard(m_board & rhs); }
    [[nodiscard]] constexpr Bitboard operator^(u64 rhs) const { return Bitboard(m_board ^ rhs); }
#pragma endregion  // private operators
    u64 m_board;
};

template<Set s>
[[nodiscard]] constexpr u64
shiftNorthRelative(u64 bb)
{
    if constexpr (s == Set::WHITE) {
        return bb << shifts::vertical;
    }
    else {
        return bb >> shifts::vertical;
    }
}
template<Set s>
[[nodiscard]] constexpr u64
shiftEastRelative(u64 bb)
{
    if constexpr (s == Set::WHITE) {
        return bb << shifts::horizontal;
    }
    else {
        return bb >> shifts::horizontal;
    }
}

template<Set s>
[[nodiscard]] constexpr u64
shiftSouthRelative(u64 bb)
{
    if constexpr (s == Set::WHITE) {
        return bb >> shifts::vertical;
    }
    else {
        return bb << shifts::vertical;
    }
}

template<Set s>
[[nodiscard]] constexpr u64
shiftWestRelative(u64 bb)
{
    if constexpr (s == Set::WHITE) {
        return bb >> shifts::horizontal;
    }
    else {
        return bb << shifts::horizontal;
    }
}

template<Set s>
[[nodiscard]] constexpr u64
shiftNorthEastRelative(u64 bb)
{
    if constexpr (s == Set::WHITE) {
        return bb << shifts::forward_diagonal;
    }
    else {
        return bb >> shifts::forward_diagonal;
    }
}

template<Set s>
[[nodiscard]] constexpr u64
shiftSouthEastRelative(u64 bb)
{
    if constexpr (s == Set::WHITE) {
        return bb >> shifts::backward_diagonal;
    }
    else {
        return bb << shifts::backward_diagonal;
    }
}

template<Set s>
[[nodiscard]] constexpr u64
shiftSouthWestRelative(u64 bb)
{
    if constexpr (s == Set::WHITE) {
        return bb >> shifts::forward_diagonal;
    }
    else {
        return bb << shifts::forward_diagonal;
    }
}

template<Set s>
[[nodiscard]] constexpr u64
shiftNorthWestRelative(u64 bb)
{
    if constexpr (s == Set::WHITE) {
        return bb << shifts::backward_diagonal;
    }
    else {
        return bb >> shifts::backward_diagonal;
    }
}

template<Set s, u8 direction>
[[nodiscard]] constexpr u64
shiftRelative(u64 bb)
{
    if constexpr (direction == north) {
        return shiftNorthRelative<s>(bb);
    }
    else if constexpr (direction == east) {
        return shiftEastRelative<s>(bb);
    }
    else if constexpr (direction == south) {
        return shiftSouthRelative<s>(bb);
    }
    else if constexpr (direction == west) {
        return shiftWestRelative<s>(bb);
    }
    else if constexpr (direction == northeast) {
        return shiftNorthEastRelative<s>(bb);
    }
    else if constexpr (direction == southeast) {
        return shiftSouthEastRelative<s>(bb);
    }
    else if constexpr (direction == southwest) {
        return shiftSouthWestRelative<s>(bb);
    }
    else if constexpr (direction == northwest) {
        return shiftNorthWestRelative<s>(bb);
    }

    FATAL_ASSERT(false) << "Invalid direction";
}

#endif  // BITBOARD_HEADER