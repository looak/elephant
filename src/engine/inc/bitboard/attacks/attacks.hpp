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
 * @file attacks.hpp
 * @brief Defines functions and tables for calculating chess piece attack patterns and masks 
 * 
 */

#pragma once

#include <array>
#include <system/platform.hpp>
#include <bitboard/bitboard_constants.hpp>
#include <bitboard/intrinsics.hpp>
#include <bitboard/attacks/magic_constants.hpp>
#include <material/chess_piece_defines.hpp>

namespace attacks {
namespace tables {
const std::array<u64, 64>& getKnightAttacks();
const std::array<u64, 64>& getRookAttacks();
const std::array<u64, 64>& getBishopAttacks();
const std::array<std::array<u64, 4096>, 64>& getRookAttacksTable();
const std::array<std::array<u64, 1024>, 64>& getBishopAttacksTable();
} // namespace tables

namespace internals {

    template<bool edge = false>
    constexpr u64 shift(u8 direction, u64 occupancy, u64 bounds, u64 bb) {
        u64 result = 0;
        i32 shift = shifts::shifts[direction];
        const bool right = 0 > shift ? true : false;
        shift = 0 > shift ? -shift : shift;

        if (bb & bounds)
            return 0;

        occupancy |= bounds;

        do {
            if (right)
                bb >>= shift;
            else bb <<= shift;

            if (bb == 0)
                break;

            result |= bb;

        } while ((bb & occupancy) == 0);

        if constexpr (edge)
            return result;
        else
            return result & ~bounds;
    }

    constexpr u64 generateKnightAttackMask(u8 sqr) {
        u64 attkMask = 0;
        u8 moveCount = piece_constants::move_count[knightId];

        for (u8 moveIndx = 0; moveIndx < moveCount; ++moveIndx) {
            u8 curSqr = sqr;
            i8 dir = piece_constants::moves0x88[knightId][moveIndx];

            // build a 0x88 square out of current square.
            u8 sq0x88 = to0x88(curSqr);
            // do move
            sq0x88 += dir;
            if (sq0x88 & 0x88) // validate move, are we still on the board?
                continue;

            // convert our sqr0x88 back to a square index
            curSqr = fr0x88(sq0x88);

            // build a square mask from current square
            u64 sqrMask = squareMaskTable[curSqr];
            attkMask |= sqrMask;
        }

        return attkMask;
    }

    template<bool edge = false>
    constexpr u64 generateRookAttackMask(u8 sqr, u64 occupancy) {
        u64 attkMask = 0;
        const u64 sqrMask = 1ULL << sqr;

        attkMask |= shift<edge>(cardinal_constants::north, occupancy, board_constants::rank7Mask, sqrMask);
        attkMask |= shift<edge>(cardinal_constants::south, occupancy, board_constants::rank0Mask, sqrMask);
        attkMask |= shift<edge>(cardinal_constants::east, occupancy, board_constants::filehMask, sqrMask);
        attkMask |= shift<edge>(cardinal_constants::west, occupancy, board_constants::fileaMask, sqrMask);

        return attkMask;
    }

    template<bool edge = false>
    constexpr u64 generateBishopAttackMask(u8 sqr, u64 occupancy)
    {
        u64 attkMask = 0;
        const u64 sqrMask = 1ULL << sqr;

        attkMask |= shift<edge>(cardinal_constants::northeast, occupancy, board_constants::rank7Mask | board_constants::filehMask, sqrMask);
        attkMask |= shift<edge>(cardinal_constants::southeast, occupancy, board_constants::rank0Mask | board_constants::filehMask, sqrMask);
        attkMask |= shift<edge>(cardinal_constants::southwest, occupancy, board_constants::rank0Mask | board_constants::fileaMask, sqrMask);
        attkMask |= shift<edge>(cardinal_constants::northwest, occupancy, board_constants::rank7Mask | board_constants::fileaMask, sqrMask);

        return attkMask;
    }

    constexpr std::array<u64, 64> generateKnightAttackTable() {
        std::array<u64, 64> knightAttacks{};
        for (u8 sqr = 0; sqr < 64; ++sqr)
            knightAttacks[sqr] = generateKnightAttackMask(sqr);

        return knightAttacks;
    }

    constexpr std::array<u64, 64> generateRookAttackTable() {
        std::array<u64, 64> rookAttacks{};
        for (u8 sqr = 0; sqr < 64; ++sqr)
            rookAttacks[sqr] = generateRookAttackMask(sqr, 0);

        return rookAttacks;
    }

    constexpr std::array<u64, 64> generateBishopAttackTable() {
        std::array<u64, 64> bishopAttacks{};
        for (u8 sqr = 0; sqr < 64; ++sqr)
            bishopAttacks[sqr] = generateBishopAttackMask(sqr, 0);

        return bishopAttacks;
    }

    void generateRookTable(std::array<std::array<u64, 4096>, 64>& result);
    void generateBishopTable(std::array<std::array<u64, 1024>, 64>& result);
    void initialize();

} // namespace internals

inline u64 getKnightAttacks(u32 sqr) {
    return tables::getKnightAttacks()[sqr];
}

inline u64 getRookAttacks(u32 sqr, u64 occupancy) {
    u64 key = occupancy & tables::getRookAttacks()[sqr];
    u64 index = (key * magics::constants::rook[sqr]) >> (magics::constants::rook_shifts[sqr]);
    return tables::getRookAttacksTable()[sqr][static_cast<size_t>(index)];
}

inline u64 getBishopAttacks(u32 sqr, u64 occupancy) {
    u64 key = occupancy & tables::getBishopAttacks()[sqr];
    u64 index = (key * magics::constants::bishop[sqr]) >> (magics::constants::bishop_shifts[sqr]);
    return tables::getBishopAttacksTable()[sqr][static_cast<size_t>(index)];
}

} // namespace attacks