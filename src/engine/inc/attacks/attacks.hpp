#pragma once

#include <array>

#include "bitboard_constants.hpp"
#include "defines.h"
#include "intrinsics.hpp"
#include "magic_constants.hpp"

namespace attacks {
class tables {
private:
    // can't be consteval because of the while loop - might be supported in the future.
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

        return result & ~bounds;
    }

    constexpr std::array<u64, 64> generateRookAttackTables() {
        std::array<u64, 64> rookAttacks;
        for (u8 sqr = 0; sqr < 64; ++sqr)
            rookAttacks[sqr] = generateRookAttackMask(0, sqr);

        return rookAttacks;
    }

    constexpr u64 generateRookAttackMask(u64 occupancy, u8 sqr) {
        u64 attkMask = 0;
        const u64 sqrMask = 1ULL << sqr;

        attkMask |= shift(north, occupancy, board_constants::rank7Mask, sqrMask);
        attkMask |= shift(south, occupancy, board_constants::rank0Mask, sqrMask);
        attkMask |= shift(east, occupancy, board_constants::filehMask, sqrMask);
        attkMask |= shift(west, occupancy, board_constants::fileaMask, sqrMask);

        return attkMask;
    }

    // constexpr u64 generateBishopAttackMask(u64 occupancy, u8 sqr)
    // {
    //     u64 attkMask = 0;
    //     const u64 sqrMask = 1ULL << sqr;

    //     attkMask |= shift(northeast, occupancy | board_constants::rank7Mask | board_constants::filehMask, sqrMask);
    //     attkMask |= shift(southeast, occupancy | board_constants::rank0Mask | board_constants::filehMask, sqrMask);
    //     attkMask |= shift(southwest, occupancy | board_constants::rank0Mask | board_constants::fileaMask, sqrMask);
    //     attkMask |= shift(northwest, occupancy | board_constants::rank7Mask | board_constants::fileaMask, sqrMask);

    //     return attkMask;
    // }

    // constexpr std::array<u64, 64> generateBishopAttackTables() {
    //     std::array<u64, 64> bishopAttacks;

    //     // sliding attacks
    //     for (u8 sqr = 0; sqr < 64; ++sqr)
    //         bishopAttacks[sqr] = generateBishopAttackMask(0, sqr);

    //     return bishopAttacks;
    // }

    // //
    constexpr std::array<std::array<u64, 4096>, 64> generateRookTable() {
        std::array<std::array<u64, 4096>, 64> result;
        for (u8 sqr = 0; sqr < 64; ++sqr) {

            u64 attkMask = tables::rookAttacks[sqr];
            u64 occupancyVariations = 1ULL << intrinsics::popcnt(attkMask);

            for (u64 i = 0; i < occupancyVariations; ++i) {
                u64 occupancy = intrinsics::pdep(i, attkMask);
                u64 magicIndex = (occupancy * magics::constants::rook[sqr]) >> (magics::constants::rook_shifts[sqr]);
                result[sqr][magicIndex] = generateRookAttackMask(occupancy, sqr);
            }
        }
        return result;
    }

    // constexpr std::array<std::array<u64, 1024>, 64> generateBishopAttacksTable() {
    //     std::array<std::array<u64, 1024>, 64> bishopAttacksTable;
    //     for (u8 sqr = 0; sqr < 64; ++sqr) {

    //         u64 attkMask = tables::bishopAttacks[sqr];
    //         u64 occupancyVariations = 1ULL << intrinsics::popcnt(attkMask);

    //         for (u64 i = 0; i < occupancyVariations; ++i) {
    //             u64 occupancy = intrinsics::pdep(i, attkMask);
    //             u64 magicIndex = (occupancy * magics::constants::bishop[sqr]) >> (magics::constants::bishop_shifts[sqr]);
    //             bishopAttacksTable[sqr][magicIndex] = generateBishopAttackMask(occupancy, sqr);
    //         }
    //     }
    //     return bishopAttacksTable;
    // }

    static tables* instance;

    tables()
        : rookAttacks(generateRookAttackTables())
        , rookAttacksTable(generateRookTable())
    {
        //rookAttacks = _consteval::generateRookAttackTables();
        // bishopAttacks = generateBishopAttackTables();
        //bishopAttacksTable = generateBishopAttacksTable();
    }

public:
    static const tables* get() {
        if (!instance)
            instance = new tables();
        return instance;
    }

    std::array<u64, 64> rookAttacks;
    // std::array<u64, 64> bishopAttacks; // = generateBishopAttackTables();
    //std::array<std::array<u64, 1024>, 64> bishopAttacksTable; // = generateBishopAttacksTable();
    std::array<std::array<u64, 4096>, 64> rookAttacksTable; // = generateRookTable();

}; // class tables


inline u64 getRookAttacks(u8 sqr, u64 occupancy) {
    u64 key = occupancy & tables::get()->rookAttacks[sqr];
    u64 index = (key * magics::constants::rook[sqr]) >> (magics::constants::rook_shifts[sqr]);
    return tables::get()->rookAttacksTable[sqr][index];
}

} // namespace attacks