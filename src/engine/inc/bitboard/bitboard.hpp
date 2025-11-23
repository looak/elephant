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

#include <bitboard/bitboard_constants.hpp>
#include <bitboard/intrinsics.hpp>
#include <defines.hpp>
#include <material/chess_piece_defines.hpp>

template<typename T>
class BitboardSquare {
    template<typename U>
    friend class BitboardImpl;
public:
    constexpr void operator=(bool value)
    {
        if (value) {
            m_bb |= squareMaskTable[*m_sqr];
        }
        else {
            m_bb &= ~squareMaskTable[*m_sqr];
        }
    }

    constexpr operator bool() const { return (m_bb & squareMaskTable[static_cast<u8>(m_sqr)]) != 0; }

private:
    constexpr BitboardSquare(T& bb, Square sqr) :
        m_bb(bb),
        m_sqr(sqr)
    {
    }

    T& m_bb;
    Square m_sqr;
};

template<typename T>
class BitboardImpl {
public:
    constexpr BitboardImpl(T board) :
        m_board(board)
    {
    }

    constexpr BitboardImpl() :
        m_board(0)
    {
    }

    /**
     * @brief returns the internal 64bit integer    */
    [[nodiscard]] constexpr T read() const { return m_board; }

    /**
    * @brief returns the internal 64bit integer for mutating    */
    [[nodiscard]] T& edit() { return m_board; }

    /**
     * @brief returns true if the bitboard is empty    */
    [[nodiscard]] constexpr bool empty() const { return m_board == 0; }

    /**
     * @brief returns the number of set bits in the bitboard    */
    [[nodiscard]] constexpr i32 count() const { return intrinsics::popcnt(m_board); }

    /**
     * @brief resets board to 0    */
    void reset() { m_board = 0; }

    /**
     * @brief clears given mask from bitboard    */
    void clear(u64 mask) { m_board &= ~mask; }

    /**
     * @brief returns index of the least significant bit        */
    [[nodiscard]] constexpr u32 lsbIndex() const { return intrinsics::lsbIndex(m_board); }

    /**
     * @brief unsets least signficiant bit, board can't be empty    */
    [[nodiscard]] constexpr u64 resetLsb() const
    {
        ASSERT(!empty());
        return intrinsics::resetLsb(m_board);
    }

    /**
     * @brief returns the least significant bit and unsets it, board can't be empty     */
    [[nodiscard]] constexpr u32 popLsb()
    {
        ASSERT(!empty());
        u32 index = lsbIndex();
        m_board = resetLsb();
        return index;
    }

    /**
     * @brief combine given bitboards   */
    template<typename... Args>
    [[nodiscard]] BitboardImpl combine(BitboardImpl first, Args... args) const
    {
        return first | combine(args...);
    }

    [[nodiscard]] constexpr BitboardImpl shiftNorth(u8 shift) const { return BitboardImpl(m_board << shift); }

    [[nodiscard]] constexpr BitboardImpl shiftRight(u8 shift) const { return BitboardImpl(m_board >> shift); }
    [[nodiscard]] constexpr BitboardImpl shiftLeft(u8 shift) const { return BitboardImpl(m_board << shift); }

    /**
     * Section: operators */
    constexpr BitboardImpl& operator|=(const BitboardImpl& rhs)
    {
        m_board |= rhs.m_board;
        return *this;
    }

    constexpr BitboardImpl& operator&=(const BitboardImpl& rhs)
    {
        m_board &= rhs.m_board;
        return *this;
    }

    constexpr BitboardImpl& operator^=(const BitboardImpl& rhs)
    {
        m_board ^= rhs.m_board;
        return *this;
    }

    [[nodiscard]] constexpr BitboardImpl operator|(const BitboardImpl& rhs) const { return BitboardImpl(m_board | rhs.m_board); }
    [[nodiscard]] constexpr BitboardImpl operator&(const BitboardImpl& rhs) const { return BitboardImpl(m_board & rhs.m_board); }
    [[nodiscard]] constexpr BitboardImpl operator^(const BitboardImpl& rhs) const { return BitboardImpl(m_board ^ rhs.m_board); }

    constexpr BitboardImpl& operator|=(u64 rhs)
    {
        m_board |= rhs;
        return *this;
    }

    constexpr BitboardImpl& operator&=(u64 rhs)
    {
        m_board &= rhs;
        return *this;
    }

    constexpr BitboardImpl& operator^=(u64 rhs)
    {
        m_board ^= rhs;
        return *this;
    }

    [[nodiscard]] constexpr BitboardImpl operator|(u64 rhs) const { return BitboardImpl(m_board | rhs); }
    [[nodiscard]] constexpr BitboardImpl operator&(u64 rhs) const { return BitboardImpl(m_board & rhs); }
    [[nodiscard]] constexpr BitboardImpl operator^(u64 rhs) const { return BitboardImpl(m_board ^ rhs); }

    [[nodiscard]] constexpr BitboardImpl operator<<(u8 rhs) const { return shiftLeft(rhs); }
    [[nodiscard]] constexpr BitboardImpl operator>>(u8 rhs) const { return shiftRight(rhs); }

    [[nodiscard]] constexpr BitboardImpl operator~() const { return BitboardImpl(~m_board); };
    [[nodiscard]] explicit constexpr operator bool() const { return !empty(); }

    [[nodiscard]] constexpr bool operator==(const BitboardImpl& rhs) const { return m_board == rhs.m_board; }
    [[nodiscard]] constexpr bool operator!=(const BitboardImpl& rhs) const { return !(*this == rhs); }
    [[nodiscard]] constexpr bool operator==(const u64 rhs) const { return m_board == rhs; }
    [[nodiscard]] constexpr bool operator!=(const u64 rhs) const { return !(*this == rhs); }

    /**
     * @brief returns true if the square is set in the bitboard    */
    constexpr bool operator[](Square sqr) const { return (m_board & squareMaskTable[static_cast<u8>(sqr)]) != 0; }
    /**
     * @brief returns a BitboardSquare object that can be used to mutate the square    */
    constexpr BitboardSquare<T> operator[](Square sqr) { return BitboardSquare<T>(m_board, sqr); }

    /**
     * Section: shifts */
    [[nodiscard]] constexpr BitboardImpl shiftNorth() const { return BitboardImpl(m_board << shifts::vertical); }
    [[nodiscard]] constexpr BitboardImpl shiftEast() const { return BitboardImpl(m_board << shifts::horizontal); }
    [[nodiscard]] constexpr BitboardImpl shiftSouth() const { return BitboardImpl(m_board >> shifts::vertical); }
    [[nodiscard]] constexpr BitboardImpl shiftWest() const { return BitboardImpl(m_board >> shifts::horizontal); }
    [[nodiscard]] constexpr BitboardImpl shiftNorthEast() const { return BitboardImpl(m_board << shifts::forward_diagonal); }
    [[nodiscard]] constexpr BitboardImpl shiftSouthEast() const { return BitboardImpl(m_board >> shifts::backward_diagonal); }
    [[nodiscard]] constexpr BitboardImpl shiftSouthWest() const { return BitboardImpl(m_board >> shifts::forward_diagonal); }
    [[nodiscard]] constexpr BitboardImpl shiftNorthWest() const { return BitboardImpl(m_board << shifts::backward_diagonal); }
    [[nodiscard]] constexpr BitboardImpl shift(u8 direcction) const;

    template<Set us, u8 direction>
    [[nodiscard]] constexpr BitboardImpl shiftRelative() const;

    template<Set us>
    [[nodiscard]] constexpr BitboardImpl shiftNorthRelative() const;
    template<Set us>
    [[nodiscard]] constexpr BitboardImpl shiftEastRelative() const;
    template<Set us>
    [[nodiscard]] constexpr BitboardImpl shiftSouthRelative() const;
    template<Set us>
    [[nodiscard]] constexpr BitboardImpl shiftWestRelative() const;
    template<Set us>
    [[nodiscard]] constexpr BitboardImpl shiftNorthEastRelative() const;
    template<Set us>
    [[nodiscard]] constexpr BitboardImpl shiftSouthEastRelative() const;
    template<Set us>
    [[nodiscard]] constexpr BitboardImpl shiftSouthWestRelative() const;
    template<Set us>
    [[nodiscard]] constexpr BitboardImpl shiftNorthWestRelative() const;

    /**
     * Section: fill */
    [[nodiscard]] constexpr BitboardImpl inclusiveFill(const u64* begin, const u64* end) const
    {
        BitboardImpl result(0);
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

    [[nodiscard]] constexpr BitboardImpl inclusiveFillWest(i8 file) const
    {
        return inclusiveFill(&board_constants::fileMasks[0], &board_constants::fileMasks[file]);
    }

    [[nodiscard]] constexpr BitboardImpl inclusiveFillEast(i8 file) const
    {
        return inclusiveFill(&board_constants::fileMasks[file], &board_constants::fileMasks[7]);
    }

    [[nodiscard]] constexpr BitboardImpl inclusiveFillSouth(i8 rank)
    {
        return inclusiveFill(&board_constants::rankMasks[0], &board_constants::rankMasks[rank]);
    }

    [[nodiscard]] constexpr BitboardImpl inclusiveFillNorth(i8 rank)
    {
        return inclusiveFill(&board_constants::rankMasks[rank], &board_constants::rankMasks[7]);
    }

    [[nodiscard]] constexpr BitboardImpl inclusiveFillNorthEast(i8 file, i8 rank)
    {
        i8 index = file + rank;
        return inclusiveFill(&board_constants::backwardDiagonalMasks[index], &board_constants::backwardDiagonalMasks[14]);
    }

    [[nodiscard]] constexpr BitboardImpl inclusiveFillSouthEast(i8 file, i8 rank)
    {
        i8 index = 7 + file - rank;
        return inclusiveFill(&board_constants::forwardDiagonalMasks[index], &board_constants::forwardDiagonalMasks[14]);
    }

    [[nodiscard]] constexpr BitboardImpl inclusiveFillSouthWest(i8 file, i8 rank)
    {
        i8 index = file + rank;
        return inclusiveFill(&board_constants::backwardDiagonalMasks[0], &board_constants::backwardDiagonalMasks[index]);
    }

    [[nodiscard]] constexpr BitboardImpl inclusiveFillNorthWest(i8 file, i8 rank)
    {
        i8 index = 7 + file - rank;
        return inclusiveFill(&board_constants::forwardDiagonalMasks[0], &board_constants::forwardDiagonalMasks[index]);
    }

private:
    // internal combine for end of variadic template recursion
    [[nodiscard]] BitboardImpl combine() const { return BitboardImpl(0); }

    T m_board;
};

template<typename T>
[[nodiscard]] constexpr BitboardImpl<T>
BitboardImpl<T>::shift(u8 direction) const
{
    if (direction == north) {
        return shiftNorth();
    }
    else if (direction == east) {
        return shiftEast();
    }
    else if (direction == south) {
        return shiftSouth();
    }
    else if (direction == west) {
        return shiftWest();
    }
    else if (direction == northeast) {
        return shiftNorthEast();
    }
    else if (direction == southeast) {
        return shiftSouthEast();
    }
    else if (direction == southwest) {
        return shiftSouthWest();
    }
    else if (direction == northwest) {
        return shiftNorthWest();
    }

    ASSERT_MSG(false, "Invalid direction");
}

template<typename T>
template<Set us, u8 direction>
[[nodiscard]] constexpr BitboardImpl<T>
BitboardImpl<T>::shiftRelative() const
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

    ASSERT_MSG(false, "Invalid direction");
}


template<typename T>
template<Set us>
[[nodiscard]] constexpr BitboardImpl<T>
BitboardImpl<T>::shiftNorthRelative() const
{
    if constexpr (us == Set::WHITE) {
        return shiftNorth();
    }
    else {
        return shiftSouth();
    }
}
template<typename T>
template<Set us>
[[nodiscard]] constexpr BitboardImpl<T>
BitboardImpl<T>::shiftEastRelative() const
{
    if constexpr (us == Set::WHITE) {
        return shiftEast();
    }
    else {
        return shiftWest();
    }
}

template<typename T>
template<Set us>
[[nodiscard]] constexpr BitboardImpl<T>
BitboardImpl<T>::shiftSouthRelative() const
{
    if constexpr (us == Set::WHITE) {
        return shiftSouth();
    }
    else {
        return shiftNorth();
    }
}

template<typename T>
template<Set us>
[[nodiscard]] constexpr BitboardImpl<T>
BitboardImpl<T>::shiftWestRelative() const
{
    if constexpr (us == Set::WHITE) {
        return shiftWest();
    }
    else {
        return shiftEast();
    }
}

template<typename T>
template<Set us>
[[nodiscard]] constexpr BitboardImpl<T>
BitboardImpl<T>::shiftNorthEastRelative() const
{
    if constexpr (us == Set::WHITE) {
        return shiftNorthEast();
    }
    else {
        return shiftSouthWest();
    }
}

template<typename T>
template<Set us>
[[nodiscard]] constexpr BitboardImpl<T>
BitboardImpl<T>::shiftSouthEastRelative() const
{
    if constexpr (us == Set::WHITE) {
        return shiftSouthEast();
    }
    else {
        return shiftNorthWest();
    }
}

template<typename T>
template<Set us>
[[nodiscard]] constexpr BitboardImpl<T>
BitboardImpl<T>::shiftSouthWestRelative() const
{
    if constexpr (us == Set::WHITE) {
        return shiftSouthWest();
    }
    else {
        return shiftNorthEast();
    }
}

template<typename T>
template<Set us>
[[nodiscard]] constexpr BitboardImpl<T>
BitboardImpl<T>::shiftNorthWestRelative() const
{
    if constexpr (us == Set::WHITE) {
        return shiftNorthWest();
    }
    else {
        return shiftSouthEast();
    }
}

typedef BitboardImpl<u64> Bitboard;

[[nodiscard]] constexpr u64
operator&(const u64& lhs, const Bitboard& rhs)
{
    return lhs & rhs.read();
}

#endif  // BITBOARD_HEADER