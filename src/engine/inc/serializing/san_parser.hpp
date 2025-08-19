// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021-2025  Alexander Loodin Ek

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

/**
 * @brief Standard algebraic notation for chess moves, converter from them to internal move representation, and vice versa.
 * https://en.wikipedia.org/wiki/Algebraic_notation_(chess)
 * @file san_parser.hpp
 * 
 * @author Alexander Loodin Ek  */
#pragma once
#include <string_view>
#include <move/move.hpp>
#include <position/position_accessors.hpp>

namespace san_parser {
    // Parses a move in standard algebraic notation and returns the internal move representation.
    PackedMove deserialize(PositionReader context, std::string_view san);

    

} // namespace san_parser