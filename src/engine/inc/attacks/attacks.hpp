#pragma once

#include <array>

#include "bitboard_constants.hpp"
#include "defines.hpp"
#include "intrinsics.hpp"
#include "magic_constants.hpp"

namespace attacks {
class tables {
public:
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

    constexpr std::array<u64, 64> generateRookAttackTable() {
        std::array<u64, 64> rookAttacks;
        for (u8 sqr = 0; sqr < 64; ++sqr)
            rookAttacks[sqr] = generateRookAttackMask(0, sqr);

        return rookAttacks;
    }

    constexpr std::array<u64, 64> generateBishopAttackTable() {
        std::array<u64, 64> bishopAttacks;
        for (u8 sqr = 0; sqr < 64; ++sqr)
            bishopAttacks[sqr] = generateBishopAttackMask(0, sqr);

        return bishopAttacks;
    }

    template<bool edge = false>
    constexpr u64 generateRookAttackMask(u64 occupancy, u8 sqr) {
        u64 attkMask = 0;
        const u64 sqrMask = 1ULL << sqr;

        attkMask |= shift<edge>(north, occupancy, board_constants::rank7Mask, sqrMask);
        attkMask |= shift<edge>(south, occupancy, board_constants::rank0Mask, sqrMask);
        attkMask |= shift<edge>(east, occupancy, board_constants::filehMask, sqrMask);
        attkMask |= shift<edge>(west, occupancy, board_constants::fileaMask, sqrMask);

        return attkMask;
    }

    template<bool edge = false>
    constexpr u64 generateBishopAttackMask(u64 occupancy, u8 sqr)
    {
        u64 attkMask = 0;
        const u64 sqrMask = 1ULL << sqr;

        attkMask |= shift<edge>(northeast, occupancy, board_constants::rank7Mask | board_constants::filehMask, sqrMask);
        attkMask |= shift<edge>(southeast, occupancy, board_constants::rank0Mask | board_constants::filehMask, sqrMask);
        attkMask |= shift<edge>(southwest, occupancy, board_constants::rank0Mask | board_constants::fileaMask, sqrMask);
        attkMask |= shift<edge>(northwest, occupancy, board_constants::rank7Mask | board_constants::fileaMask, sqrMask);

        return attkMask;
    }

    constexpr void generateRookTable(std::array<std::array<u64, 4096>, 64>& result) {
        for (u8 sqr = 0; sqr < 64; ++sqr) {

            u64 attkMask = tables::rookAttacks[sqr];
            u64 occupancyVariations = 1ULL << intrinsics::popcnt(attkMask);

            for (u64 i = 0; i < occupancyVariations; ++i) {
                u64 occupancy = intrinsics::pdep(i, attkMask);
                u64 magicIndex = (occupancy * magics::constants::rook[sqr]) >> (magics::constants::rook_shifts[sqr]);
                result[sqr][magicIndex] = generateRookAttackMask<true>(occupancy, sqr);
            }
        }
    }

    constexpr void generateBishopTable(std::array<std::array<u64, 1024>, 64>& result) {
        for (u8 sqr = 0; sqr < 64; ++sqr) {

            u64 attkMask = tables::bishopAttacks[sqr];
            u64 occupancyVariations = 1ULL << intrinsics::popcnt(attkMask);

            for (u64 i = 0; i < occupancyVariations; ++i) {
                u64 occupancy = intrinsics::pdep(i, attkMask);
                u64 magicIndex = (occupancy * magics::constants::bishop[sqr]) >> (magics::constants::bishop_shifts[sqr]);
                result[sqr][magicIndex] = generateBishopAttackMask<true>(occupancy, sqr);
            }
        }
    }

    static tables instance;

    tables()
        : rookAttacks(generateRookAttackTable())
        , bishopAttacks(generateBishopAttackTable())
    {
        generateRookTable(rookAttacksTable);
        generateBishopTable(bishopAttacksTable);
    }

public:
    static const tables& get() {
        return instance;
    }

    std::array<u64, 64> rookAttacks;
    std::array<u64, 64> bishopAttacks;
    std::array<std::array<u64, 4096>, 64> rookAttacksTable;
    std::array<std::array<u64, 1024>, 64> bishopAttacksTable;

}; // class tables

inline u64 getRookAttacks(u8 sqr, u64 occupancy) {
    u64 key = occupancy & tables::get().rookAttacks[sqr];
    u64 index = (key * magics::constants::rook[sqr]) >> (magics::constants::rook_shifts[sqr]);
    return tables::get().rookAttacksTable[sqr][index];
}

inline u64 getBishopAttacks(u8 sqr, u64 occupancy) {
    u64 key = occupancy & tables::get().bishopAttacks[sqr];
    u64 index = (key * magics::constants::bishop[sqr]) >> (magics::constants::bishop_shifts[sqr]);
    return tables::get().bishopAttacksTable[sqr][index];
}

} // namespace attacks