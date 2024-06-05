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
#include "defines.hpp"

struct Notation {
    static Notation BuildPosition(byte file, byte rank);
    static bool Validate(const Notation& notation);
    static std::string toString(const Notation& notation);
    static char fileToChar(const Notation& notation);
    static char rankToChar(const Notation& notation);
    static Notation Invalid();

    // Should Notation validate rank & file?
    // Should Notation validate format of rank & file?
    Notation() :
        file(0xF),
        rank(0xF)
    {
    }

    explicit constexpr Notation(byte index) :
        file(0xF),
        rank(0xF)
    {
        if (index > 127)
            LOG_ERROR() << "In case index is larger than 127 it will wrap around our board.";
        file = mod_by_eight(index);
        rank = index / 8;
    }

    constexpr Notation(Square sqr) :
        file(0xF),
        rank(0xF)
    {
        i32 indx = static_cast<i32>(sqr);
        file = mod_by_eight(indx);
        rank = indx / 8;
    }

    // when we read algebraic notations we might only have file or rank in some cases.
    // in those cases the other value will be 9 to identify this as a value which needs to be
    // looked up.
    Notation(byte _file, byte _rank) :
        file(_file),
        rank(_rank)
    {
    }

    Notation(Notation&& other) = default;
    Notation(const Notation& other) = default;

    byte file : 4;
    byte rank : 4;

    constexpr inline byte index() const
    {
#ifdef EG_DEBUGGING
        if (isValid())
            return (rank * 8) + file;
        else
            return 0xFF;
#else
        return (rank * 8) + file;
#endif
    }

    constexpr Square toSquare() const { return static_cast<Square>(index()); }

    bool operator==(const Notation& rhs) const;
    bool operator!=(const Notation& rhs) const;
    bool operator<(const Notation& rhs) const;
    Notation& operator=(Notation&& other);
    Notation& operator=(const Notation& other);
    bool isValid() const { return Validate(*this); }
    std::string toString() const;
};

constexpr Notation InvalidNotation(127);