#pragma once
#include <string>
#include <defines.hpp>

// 0x01 == K, 0x02 == Q, 0x04 == k, 0x08 == q
enum CastlingState {
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
    void unsetWhite() { m_innerState &= ~WHITE_ALL; }
    void unsetBlack() { m_innerState &= ~BLACK_ALL; }
    void unsetWhiteKingSide() { m_innerState &= ~WHITE_KINGSIDE; }
    void unsetWhiteQueenSide() { m_innerState &= ~WHITE_QUEENSIDE; }
    void unsetBlackKingSide() { m_innerState &= ~BLACK_KINGSIDE; }
    void unsetBlackQueenSide() { m_innerState &= ~BLACK_QUEENSIDE; }

    void setAll() { m_innerState = ALL; }
    void setWhite() { m_innerState |= WHITE_ALL; }
    void setBlack() { m_innerState |= BLACK_ALL; }
    void setWhiteKingSide() { m_innerState |= WHITE_KINGSIDE; }
    void setWhiteQueenSide() { m_innerState |= WHITE_QUEENSIDE; }
    void setBlackKingSide() { m_innerState |= BLACK_KINGSIDE; }
    void setBlackQueenSide() { m_innerState |= BLACK_QUEENSIDE; }

    byte read() const { return m_innerState; }
    void write(byte state) { m_innerState = state; }

    std::string toString() const;

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

std::string CastlingStateInfo::toString() const
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