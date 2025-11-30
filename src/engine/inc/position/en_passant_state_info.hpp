/******************************************************************************
 * Elephant Gambit Chess Engine - a Chess AI
 * Copyright(C) 2025  Alexander Loodin Ek
 * 
 * This program is free software : you can redistribute it and /or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.If not, see < http://www.gnu.org/licenses/>. 
 *****************************************************************************/

/**
 * @file en_passant_state_info.hpp
 * @brief Handles the en passant state information for a chess game 
 * 
 */

#pragma once
#include <system/platform.hpp>
#include <bitboard/bitboard.hpp>
#include <core/square_notation.hpp>
#include <position/hash_zobrist.hpp>

struct EnPassantStateInfo {
public:
    operator bool() const { return m_innerState != 0; }
    void clear() { m_innerState = 0; }

    void writeSquare(Square sq)
    {
        m_innerState = *sq;
        m_innerState = static_cast<byte>(m_innerState << 2);
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

class EnPassantStateProxy {
public:
    EnPassantStateProxy(EnPassantStateInfo& state, u64& hash) : m_state(state), m_hash(hash) {}

    operator bool() const { return m_state.operator bool(); } 
    
    void writeSquare(Square sq)
    {
        if (m_state)
            m_hash = zobrist::updateEnPassantHash(m_hash, m_state.readSquare());
        
        m_state.writeSquare(sq);
        m_hash = zobrist::updateEnPassantHash(m_hash, m_state.readSquare());
    }
    
    Square readSquare() const { return m_state.readSquare(); }
    Square readTarget() const { return m_state.readTarget(); }
    Bitboard readBitboard() const { return m_state.readBitboard(); }
    std::string toString() const { return m_state.toString(); }
    
    // read & write will mainly be used by make / unmake to track state.
    byte read() const { return m_state.read(); }
    void write(byte state) { 
        if (m_state)
            m_hash = zobrist::updateEnPassantHash(m_hash, m_state.readSquare());
        m_state.write(state);
        m_hash = zobrist::updateEnPassantHash(m_hash, m_state.readSquare());
    }

    void clear() { 
        if (m_state) // expect this to be true, but might not be.
            m_hash = zobrist::updateEnPassantHash(m_hash, m_state.readSquare());
        m_state.clear(); 
    }

private:
    EnPassantStateInfo& m_state;
    u64& m_hash;
};