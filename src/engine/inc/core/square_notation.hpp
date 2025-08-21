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
#include <string>
#include <defines.hpp>

// TODO: Make this a helper to enum class Square, so all it does is takes Squares and lets you extract rank/file or convert it
// to string. We don't want to allow building Squares out of Notations since Notations are unsafe.

struct SquareNotation {
    constexpr SquareNotation(Square _sqr) :
        m_file(0xF),
        m_rank(0xF),
        m_sqr(_sqr)
    {
        i32 indx = static_cast<i32>(m_sqr);
        m_file = mod_by_eight(indx);
        m_rank = indx / 8;
    }

    constexpr SquareNotation(byte file, byte rank) :
        m_file(file),
        m_rank(rank),
        m_sqr(static_cast<Square>((rank * 8) + file))
    {
        FATAL_ASSERT(m_file <= 7 && m_rank <= 7);
    }

    constexpr byte file() const { return m_file; }
    constexpr byte rank() const { return m_rank; }
    constexpr Square toSquare() const { return m_sqr; }
    constexpr inline byte index() const { return *m_sqr; }

    std::string toString() const {
        char buffer[4];
        snprintf(buffer, sizeof(buffer), "%c%c\n", 'a' + m_file, '1' + m_rank);
        return std::string(buffer);
    }

private:
    byte m_file;
    byte m_rank;
    Square m_sqr;
};

inline std::ostream& operator<<(std::ostream& os, const Square& s) { 
    os << SquareNotation(s).toString();
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const SquareNotation& n) {
    os << n.toString();
    return os;
}