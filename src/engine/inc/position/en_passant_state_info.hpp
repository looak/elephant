#pragma once
#include <defines.hpp>
#include <bitboard/bitboard.hpp>
#include <core/square_notation.hpp>

struct EnPassantStateInfo {
public:
    EnPassantStateInfo() = default;
    operator bool() const { return m_innerState != 0; }
    void clear() { m_innerState = 0; }

    void writeSquare(Square sq)
    {
        m_innerState = static_cast<byte>(sq);
        m_innerState = m_innerState << 2;
        m_innerState += 1;
        // m_innerState += ((byte)set << 1);
    }

    Square readSquare() const
    {
        if (*this)
            return static_cast<Square>(m_innerState >> 2);
        else
            return Square::NullSQ;
    }

    Square readTarget() const
    {
        byte sq = m_innerState >> 2;
        // Set set = static_cast<Set>(m_innerState & 2);
        if (sq < 31) {
            return static_cast<Square>(sq + 8);
        }
        return static_cast<Square>(sq - 8);
    }
    Bitboard readBitboard() const
    {
        if (*this == true) {
            Square sq = readSquare();
            return squareMaskTable[(size_t)sq];
        }
        return 0;
    }

    std::string toString() const
    {
        if (*this == true) {
            return SquareNotation(readSquare()).toString();
        }
        return "-";
    }

    // read & write will mainly be used by make / unmake to track state.
    byte read() const { return m_innerState; }
    void write(byte state) { m_innerState = state; }

private:
    // [sqr] [sqr] [sqr] [sqr] [sqr] [sqr] [set] [hasEnPassant]
    byte m_innerState;
};