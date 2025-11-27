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
 * @file rays.hpp
 * @brief Provides functions to compute and retrieve ray attack masks for chess pieces 
 * 
 */

#pragma once
#include <array>

#include <bitboard/attacks/attacks.hpp>
#include "defines.hpp"

namespace ray {
namespace internals {
void initialize();

constexpr std::array<std::array<u64, 64>, 64> computeRays() {
    std::array<std::array<u64, 64>, 64> raysTable{};

    for (u8 from = 0; from < 64; ++from) {
        for (u8 to = 0; to < 64; ++to) {

            if (from == to) {
                raysTable[from][to] = 0;
                continue;
            }

            u64 fromMask = 1ull << from;
            u64 toMask = 1ull << to;

            u64 result = 0;

            u64 rookAttacks = attacks::internals::generateRookAttackMask<true>(from, 0);
            u64 bishopAttacks = attacks::internals::generateBishopAttackMask<true>(from, 0);

            if ((rookAttacks & toMask) > 0) {
                result = attacks::getRookAttacks(from, fromMask | toMask);
                result &= attacks::getRookAttacks(to, fromMask | toMask) | toMask; // adding destination square to the mask
            }
            else if ((bishopAttacks & toMask) > 0) {
                result = attacks::getBishopAttacks(from, fromMask | toMask);
                result &= attacks::getBishopAttacks(to, fromMask | toMask) | toMask; // adding destination square to the mask
            }

            raysTable[from][to] = result;
        }
    }

    return raysTable;
}

} // namespace internals

u64 getRay(u32 from, u32 to);

} // namespace ray