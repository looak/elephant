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
 * @file printer.hpp
 * @brief Provides functions for printing chessboard and position information to output streams 
 * 
 */

#pragma once
#include <position/position_accessors.hpp>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cstdint>

class Chessboard;

namespace io {
namespace printer {

void board(std::ostream& output, const Chessboard& board);
void position(std::ostream& output, PositionReader reader);
void bitboard(std::ostream& output, const Bitboard& bitboard);

void bitboardOperationResult(std::ostream& output, const Bitboard& result, const Bitboard& lhs, const Bitboard& rhs, const std::string& operation);

inline std::string formatReadableNumber(u64 number) {    
    std::stringstream ss;
    if (number >= 1'000'000) { // Millions
        double val = number / 1'000'000.0;
        val = std::floor(val * 100.0) / 100.0;
        ss << std::fixed << std::setprecision(2) << val << " million";
    } else { // Below 1 million
        std::string s = std::to_string(number);
        int n = s.length();
        int first_group_len = n % 3;
        
        if (first_group_len == 0 && n > 0) {
            first_group_len = 3;
        }
        
        ss << s.substr(0, first_group_len);
        
        for (int i = first_group_len; i < n; i += 3) {
            ss << " " << s.substr(i, 3);
        }
    }
    
    return ss.str();
}

}  // namespace printer
}  // namespace io