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
#include <ostream>
#include <string>

#include <core/square.hpp>

// A square's algebraic name, "e4". Squares are built & taken apart with the helpers in square.hpp (to_square, to_file,
// to_rank); this is only the text form, kept out of square.hpp so it doesn't bring <string> & <ostream> everywhere.
inline std::string to_string(Square sqr) {
    return { static_cast<char>('a' + to_file(sqr)), static_cast<char>('1' + to_rank(sqr)) };
}

inline std::ostream& operator<<(std::ostream& os, Square sqr) {
    return os << to_string(sqr);
}
