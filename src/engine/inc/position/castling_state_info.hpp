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
 * @file castling_state_info.hpp
 * @brief Defines castling state management for chess pieces in a structured format. Also contains a proxy class which helps
 * updating position hash.
 * 
 */

#pragma once
#include <string>
#include <system/platform.hpp>
#include <position/hash_zobrist.hpp>

// 0x01 == K, 0x02 == Q, 0x04 == k, 0x08 == q
enum CastlingState : uint8_t {
    NONE = 0x00,
    WHITE_KINGSIDE = 0x01,
    WHITE_QUEENSIDE = 0x02,
    WHITE_ALL = WHITE_KINGSIDE | WHITE_QUEENSIDE,
    BLACK_KINGSIDE = 0x04,
    BLACK_QUEENSIDE = 0x08,
    BLACK_ALL = BLACK_KINGSIDE | BLACK_QUEENSIDE,
    ALL = WHITE_ALL | BLACK_ALL
};

struct CastlingStateInfo {
public:
    CastlingStateInfo() = default;
    CastlingStateInfo(const CastlingStateInfo& other) : m_innerState(other.m_innerState) {}

    bool hasAll() const { return m_innerState == ALL; }
    bool hasAny() const { return m_innerState != NONE; }
    bool hasNone() const { return m_innerState == NONE; }
    bool hasWhite() const { return m_innerState & WHITE_ALL; }
    bool hasBlack() const { return m_innerState & BLACK_ALL; }
    bool hasWhiteKingSide() const { return m_innerState & WHITE_KINGSIDE; }
    bool hasWhiteQueenSide() const { return m_innerState & WHITE_QUEENSIDE; }
    bool hasBlackKingSide() const { return m_innerState & BLACK_KINGSIDE; }
    bool hasBlackQueenSide() const { return m_innerState & BLACK_QUEENSIDE; }

    void clear() { m_innerState = NONE; }
    void revokeAll() { clear(); }
    void revokeAllWhite() { m_innerState &= ~WHITE_ALL; }
    void revokeAllBlack() { m_innerState &= ~BLACK_ALL; }
    void revokeWhiteKingSide() { m_innerState &= ~WHITE_KINGSIDE; }
    void revokeWhiteQueenSide() { m_innerState &= ~WHITE_QUEENSIDE; }
    void revokeBlackKingSide() { m_innerState &= ~BLACK_KINGSIDE; }
    void revokeBlackQueenSide() { m_innerState &= ~BLACK_QUEENSIDE; }

    void grantAll() { m_innerState = ALL; }
    void grantAllWhite() { m_innerState |= WHITE_ALL; }
    void grantAllBlack() { m_innerState |= BLACK_ALL; }
    void grantWhiteKingSide() { m_innerState |= WHITE_KINGSIDE; }
    void grantWhiteQueenSide() { m_innerState |= WHITE_QUEENSIDE; }
    void grantBlackKingSide() { m_innerState |= BLACK_KINGSIDE; }
    void grantBlackQueenSide() { m_innerState |= BLACK_QUEENSIDE; }

    byte read() const { return m_innerState; }
    void write(byte state) { m_innerState = state; }

    CastlingState asFlag() const { return static_cast<CastlingState>(m_innerState); }

    std::string toString() const
    {
        std::string result;
        if (hasWhiteKingSide())
            result += "K";
        if (hasWhiteQueenSide())
            result += "Q";
        if (hasBlackKingSide())
            result += "k";
        if (hasBlackQueenSide())
            result += "q";

        if (result.empty())
            result = "-";

        return result;
    }

    bool operator==(const CastlingStateInfo& rhs) const { return m_innerState == rhs.m_innerState; }
    CastlingStateInfo& operator=(const CastlingStateInfo& other) {
        if (this != &other) {
            m_innerState = other.m_innerState;
        }
        return *this;
    }

private:
    byte m_innerState = 0;
};

class CastlingStateProxy {
public:
    CastlingStateProxy(CastlingStateInfo& state, u64& hash) : m_state(state), m_hash(hash) {}
    ~CastlingStateProxy() = default;


    bool hasAll() const { return m_state.hasAll(); }
    bool hasAny() const { return m_state.hasAny(); }
    bool hasNone() const { return m_state.hasNone(); }
    bool hasWhite() const { return m_state.hasWhite(); }
    bool hasBlack() const { return m_state.hasBlack(); }
    bool hasWhiteKingSide() const { return m_state.hasWhiteKingSide(); }
    bool hasWhiteQueenSide() const { return m_state.hasWhiteQueenSide(); }
    bool hasBlackKingSide() const { return m_state.hasBlackKingSide(); }
    bool hasBlackQueenSide() const { return m_state.hasBlackQueenSide(); }

    void clear() {
        ScopedHasher hasher(m_hash, m_state);
        m_state.clear();
    }

    void revokeAll() { clear(); }

    void revokeAllWhite() {
        ScopedHasher hasher(m_hash, m_state);
        m_state.revokeAllWhite();
    }

    void revokeAllBlack() {
        ScopedHasher hasher(m_hash, m_state);
        m_state.revokeAllBlack();
    }

    void revokeWhiteKingSide() {
        ScopedHasher hasher(m_hash, m_state);
        m_state.revokeWhiteKingSide();
    }

    void revokeWhiteQueenSide() {
        ScopedHasher hasher(m_hash, m_state);
        m_state.revokeWhiteQueenSide();
    }

    void revokeBlackKingSide() {
        ScopedHasher hasher(m_hash, m_state);
        m_state.revokeBlackKingSide();
    }

    void revokeBlackQueenSide() {
        ScopedHasher hasher(m_hash, m_state);
        m_state.revokeBlackQueenSide();
    }

    void grantAll() {
        ScopedHasher hasher(m_hash, m_state);
        m_state.grantAll();
    }

    void grantAllWhite() {
        ScopedHasher hasher(m_hash, m_state);
        m_state.grantAllWhite();
    }

    void grantAllBlack() {
        ScopedHasher hasher(m_hash, m_state);
        m_state.grantAllBlack();
    }

    void grantWhiteKingSide() {
        ScopedHasher hasher(m_hash, m_state);
        m_state.grantWhiteKingSide();
    }

    void grantWhiteQueenSide() {
        ScopedHasher hasher(m_hash, m_state);
        m_state.grantWhiteQueenSide();
    }

    void grantBlackKingSide() {
        ScopedHasher hasher(m_hash, m_state);
        m_state.grantBlackKingSide();
    }

    void grantBlackQueenSide() {
        ScopedHasher hasher(m_hash, m_state);
        m_state.grantBlackQueenSide();
    }

    u8 read() const { return m_state.read(); }
    void write(u8 state) {
        ScopedHasher hasher(m_hash, m_state);
        m_state.write(state);        
    }

private:
    struct ScopedHasher {
        ScopedHasher(u64& hashRef, CastlingStateInfo& stateRef) : m_hashRef(hashRef), m_stateRef(stateRef) {
            // clear hash
            m_hashRef = zobrist::updateCastlingHash(m_hashRef, m_stateRef.read());
        }
        ~ScopedHasher(){
            m_hashRef = zobrist::updateCastlingHash(m_hashRef, m_stateRef.read());
        }

    private:
        u64& m_hashRef;
        CastlingStateInfo& m_stateRef;
    };

    CastlingStateInfo& m_state;
    u64& m_hash;

};