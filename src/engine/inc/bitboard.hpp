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
#include "intrinsics.hpp"

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

    /**
     * @brief returns true if the bitboard is empty    */
    [[nodiscard]] constexpr bool empty() const { return m_board == 0; }

    /**
     * @brief returns the number of set bits in the bitboard    */
    [[nodiscard]] constexpr i32 count() const { return intrinsics::popcnt(m_board); }

    /**
     * @brief returns index of the least significant bit        */
    [[nodiscard]] constexpr u32 lsbIndex() const { return intrinsics::lsbIndex(m_board); }

    /**
     * @brief unsets least signficiant bit, board can't be empty    */
    [[nodiscard]] constexpr u64 resetLsb() const
    {
        FATAL_ASSERT(!empty());
        return intrinsics::resetLsb(m_board);
    }

    /**
     * @brief returns the least significant bit and unsets it, board can't be empty     */
    [[nodiscard]] constexpr u32 popLsb()
    {
        FATAL_ASSERT(!empty());
        u32 index = lsbIndex();
        m_board = resetLsb();
        return index;
    }

    /**
     * @brief resets board to 0    */
    void reset() { m_board = 0; }

    [[nodiscard]] constexpr Bitboard shiftRight(u8 shift) const { return Bitboard(m_board >> shift); }
    [[nodiscard]] constexpr Bitboard shiftLeft(u8 shift) const { return Bitboard(m_board << shift); }

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

    constexpr Bitboard& operator|=(u64 rhs)
    {
        m_board |= rhs;
        return *this;
    }

    constexpr Bitboard& operator&=(u64 rhs)
    {
        m_board &= rhs;
        return *this;
    }

    constexpr Bitboard& operator^=(u64 rhs)
    {
        m_board ^= rhs;
        return *this;
    }

    [[nodiscard]] constexpr Bitboard operator|(u64 rhs) const { return Bitboard(m_board | rhs); }
    [[nodiscard]] constexpr Bitboard operator&(u64 rhs) const { return Bitboard(m_board & rhs); }
    [[nodiscard]] constexpr Bitboard operator^(u64 rhs) const { return Bitboard(m_board ^ rhs); }

    [[nodiscard]] constexpr Bitboard operator<<(u64 rhs) const { return shiftLeft(rhs); }
    [[nodiscard]] constexpr Bitboard operator>>(u64 rhs) const { return shiftRight(rhs); }

    [[nodiscard]] constexpr Bitboard operator~() const { return Bitboard(~m_board); };
    [[nodiscard]] explicit constexpr operator bool() const { return !empty(); }

    [[nodiscard]] constexpr bool operator==(const Bitboard& rhs) const { return m_board == rhs.m_board; }
    [[nodiscard]] constexpr bool operator!=(const Bitboard& rhs) const { return !(*this == rhs); }
    [[nodiscard]] constexpr bool operator==(const u64 rhs) const { return m_board == rhs; }
    [[nodiscard]] constexpr bool operator!=(const u64 rhs) const { return !(*this == rhs); }

    /**
     * @brief returns true if the square is set in the bitboard    */
    constexpr bool operator[](Square sqr) const { return (m_board & squareMaskTable[static_cast<u8>(sqr)]) != 0; }
    /**
     * @brief returns a BitboardSquare object that can be used to set the square    */
    constexpr BitboardSquare operator[](Square sqr) { return BitboardSquare(m_board, sqr); }
#pragma endregion  // operators

#pragma region shifts
    [[nodiscard]] constexpr Bitboard shiftNorth() const { return Bitboard(m_board << shifts::vertical); }
    [[nodiscard]] constexpr Bitboard shiftEast() const { return Bitboard(m_board << shifts::horizontal); }
    [[nodiscard]] constexpr Bitboard shiftSouth() const { return Bitboard(m_board >> shifts::vertical); }
    [[nodiscard]] constexpr Bitboard shiftWest() const { return Bitboard(m_board >> shifts::horizontal); }
    [[nodiscard]] constexpr Bitboard shiftNorthEast() const { return Bitboard(m_board << shifts::forward_diagonal); }
    [[nodiscard]] constexpr Bitboard shiftSouthEast() const { return Bitboard(m_board >> shifts::backward_diagonal); }
    [[nodiscard]] constexpr Bitboard shiftSouthWest() const { return Bitboard(m_board >> shifts::forward_diagonal); }
    [[nodiscard]] constexpr Bitboard shiftNorthWest() const { return Bitboard(m_board << shifts::backward_diagonal); }

    template<Set us, u8 direction>
    [[nodiscard]] constexpr Bitboard shiftRelative() const;

    template<Set us>
    [[nodiscard]] constexpr Bitboard shiftNorthRelative() const;
    template<Set us>
    [[nodiscard]] constexpr Bitboard shiftEastRelative() const;
    template<Set us>
    [[nodiscard]] constexpr Bitboard shiftSouthRelative() const;
    template<Set us>
    [[nodiscard]] constexpr Bitboard shiftWestRelative() const;
    template<Set us>
    [[nodiscard]] constexpr Bitboard shiftNorthEastRelative() const;
    template<Set us>
    [[nodiscard]] constexpr Bitboard shiftSouthEastRelative() const;
    template<Set us>
    [[nodiscard]] constexpr Bitboard shiftSouthWestRelative() const;
    template<Set us>
    [[nodiscard]] constexpr Bitboard shiftNorthWestRelative() const;
#pragma endregion  // shifts

#pragma region fill

    [[nodiscard]] constexpr Bitboard inclusiveFill(const u64* begin, const u64* end) const
    {
        Bitboard result(0);
        if (*begin == *end) {
            return result | *begin;
        }
        do {
            result |= *begin;
            begin++;
        } while (*begin != *end);
        result |= *begin;
        return result;
    }

    [[nodiscard]] constexpr Bitboard inclusiveFillWest(i8 file) const
    {
        return inclusiveFill(&board_constants::fileMasks[0], &board_constants::fileMasks[file]);
    }

    [[nodiscard]] constexpr Bitboard inclusiveFillEast(i8 file) const
    {
        return inclusiveFill(&board_constants::fileMasks[file], &board_constants::fileMasks[7]);
    }

    [[nodiscard]] constexpr Bitboard inclusiveFillSouth(i8 rank)
    {
        return inclusiveFill(&board_constants::rankMasks[0], &board_constants::rankMasks[rank]);
    }

    [[nodiscard]] constexpr Bitboard inclusiveFillNorth(i8 rank)
    {
        return inclusiveFill(&board_constants::rankMasks[rank], &board_constants::rankMasks[7]);
    }

    [[nodiscard]] constexpr Bitboard inclusiveFillNorthEast(i8 file, i8 rank)
    {
        i8 index = file + rank;
        return inclusiveFill(&board_constants::backwardDiagonalMasks[index], &board_constants::backwardDiagonalMasks[14]);
    }

    [[nodiscard]] constexpr Bitboard inclusiveFillSouthEast(i8 file, i8 rank)
    {
        i8 index = 7 + file - rank;
        return inclusiveFill(&board_constants::forwardDiagonalMasks[index], &board_constants::forwardDiagonalMasks[14]);
    }

    [[nodiscard]] constexpr Bitboard inclusiveFillSouthWest(i8 file, i8 rank)
    {
        i8 index = file + rank;
        return inclusiveFill(&board_constants::backwardDiagonalMasks[0], &board_constants::backwardDiagonalMasks[index]);
    }

    [[nodiscard]] constexpr Bitboard inclusiveFillNorthWest(i8 file, i8 rank)
    {
        i8 index = 7 + file - rank;
        return inclusiveFill(&board_constants::forwardDiagonalMasks[0], &board_constants::forwardDiagonalMasks[index]);
    }
#pragma endregion  // fill

private:
    u64 m_board;
};

template<Set us, u8 direction>
[[nodiscard]] constexpr Bitboard
Bitboard::shiftRelative() const
{
    if constexpr (direction == north) {
        return shiftNorthRelative<us>();
    }
    else if constexpr (direction == east) {
        return shiftEastRelative<us>();
    }
    else if constexpr (direction == south) {
        return shiftSouthRelative<us>();
    }
    else if constexpr (direction == west) {
        return shiftWestRelative<us>();
    }
    else if constexpr (direction == northeast) {
        return shiftNorthEastRelative<us>();
    }
    else if constexpr (direction == southeast) {
        return shiftSouthEastRelative<us>();
    }
    else if constexpr (direction == southwest) {
        return shiftSouthWestRelative<us>();
    }
    else if constexpr (direction == northwest) {
        return shiftNorthWestRelative<us>();
    }

    FATAL_ASSERT(false) << "Invalid direction";
}

template<Set us>
constexpr Bitboard
Bitboard::shiftNorthRelative() const
{
    if constexpr (us == Set::WHITE) {
        return shiftNorth();
    }
    else {
        return shiftSouth();
    }
}
template<Set us>
constexpr Bitboard
Bitboard::shiftEastRelative() const
{
    if constexpr (us == Set::WHITE) {
        return shiftEast();
    }
    else {
        return shiftWest();
    }
}

template<Set us>
constexpr Bitboard
Bitboard::shiftSouthRelative() const
{
    if constexpr (us == Set::WHITE) {
        return shiftSouth();
    }
    else {
        return shiftNorth();
    }
}

template<Set us>
constexpr Bitboard
Bitboard::shiftWestRelative() const
{
    if constexpr (us == Set::WHITE) {
        return shiftWest();
    }
    else {
        return shiftEast();
    }
}

template<Set us>
[[nodiscard]] constexpr Bitboard
Bitboard::shiftNorthEastRelative() const
{
    if constexpr (us == Set::WHITE) {
        return shiftNorthEast();
    }
    else {
        return shiftSouthWest();
    }
}

template<Set us>
[[nodiscard]] constexpr Bitboard
Bitboard::shiftSouthEastRelative() const
{
    if constexpr (us == Set::WHITE) {
        return shiftSouthEast();
    }
    else {
        return shiftNorthWest();
    }
}

template<Set us>
[[nodiscard]] constexpr Bitboard
Bitboard::shiftSouthWestRelative() const
{
    if constexpr (us == Set::WHITE) {
        return shiftSouthWest();
    }
    else {
        return shiftNorthEast();
    }
}

template<Set us>
[[nodiscard]] constexpr Bitboard
Bitboard::shiftNorthWestRelative() const
{
    if constexpr (us == Set::WHITE) {
        return shiftNorthWest();
    }
    else {
        return shiftSouthEast();
    }
}

#endif  // BITBOARD_HEADER