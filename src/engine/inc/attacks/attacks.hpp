#pragma once

#include <array>

#include "bitboard_constants.hpp"
#include "chess_piece_defines.hpp"
#include "defines.hpp"
#include "intrinsics.hpp"
#include "magic_constants.hpp"

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
    constexpr u64 shift(i8 direction, u64 occupancy, u64 bounds, u64 bb) {
        u64 result = 0;
        i8 shift = shifts::shifts[direction];
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
        u8 moveCount = ChessPieceDef::MoveCount(knightId);

        for (byte moveIndx = 0; moveIndx < moveCount; ++moveIndx) {
            byte curSqr = sqr;
            signed short dir = ChessPieceDef::Moves0x88(knightId, moveIndx);

            // build a 0x88 square out of current square.
            signed char sq0x88 = to0x88(curSqr);
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

        attkMask |= shift<edge>(north, occupancy, board_constants::rank7Mask, sqrMask);
        attkMask |= shift<edge>(south, occupancy, board_constants::rank0Mask, sqrMask);
        attkMask |= shift<edge>(east, occupancy, board_constants::filehMask, sqrMask);
        attkMask |= shift<edge>(west, occupancy, board_constants::fileaMask, sqrMask);

        return attkMask;
    }

    template<bool edge = false>
    constexpr u64 generateBishopAttackMask(u8 sqr, u64 occupancy)
    {
        u64 attkMask = 0;
        const u64 sqrMask = 1ULL << sqr;

        attkMask |= shift<edge>(northeast, occupancy, board_constants::rank7Mask | board_constants::filehMask, sqrMask);
        attkMask |= shift<edge>(southeast, occupancy, board_constants::rank0Mask | board_constants::filehMask, sqrMask);
        attkMask |= shift<edge>(southwest, occupancy, board_constants::rank0Mask | board_constants::fileaMask, sqrMask);
        attkMask |= shift<edge>(northwest, occupancy, board_constants::rank7Mask | board_constants::fileaMask, sqrMask);

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

}; // namespace internals

inline u64 getKnightAttacks(u8 sqr) {
    return tables::getKnightAttacks()[sqr];
}

inline u64 getRookAttacks(u8 sqr, u64 occupancy) {
    u64 key = occupancy & tables::getRookAttacks()[sqr];
    u64 index = (key * magics::constants::rook[sqr]) >> (magics::constants::rook_shifts[sqr]);
    return tables::getRookAttacksTable()[sqr][index];
}

inline u64 getBishopAttacks(u8 sqr, u64 occupancy) {
    u64 key = occupancy & tables::getBishopAttacks()[sqr];
    u64 index = (key * magics::constants::bishop[sqr]) >> (magics::constants::bishop_shifts[sqr]);
    return tables::getBishopAttacksTable()[sqr][index];
}

} // namespace attacks