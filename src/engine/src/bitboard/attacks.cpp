#include <bitboard/attacks/attacks.hpp>

namespace attacks {
namespace internals {

std::array<u64, 64> knightAttacks;
std::array<u64, 64> rookAttacks;
std::array<u64, 64> bishopAttacks;
std::array<std::array<u64, 4096>, 64> rookAttacksTable;
std::array<std::array<u64, 1024>, 64> bishopAttacksTable;

void generateRookTable(std::array<std::array<u64, 4096>, 64>& result) {
    for (u8 sqr = 0; sqr < 64; ++sqr) {

        u64 attkMask = tables::getRookAttacks()[sqr];
        u64 occupancyVariations = 1ULL << intrinsics::popcnt(attkMask);

        for (u64 i = 0; i < occupancyVariations; ++i) {
            u64 occupancy = intrinsics::pdep(i, attkMask);
            u64 magicIndex = (occupancy * magics::constants::rook[sqr]) >> (magics::constants::rook_shifts[sqr]);
            result[sqr][static_cast<size_t>(magicIndex)] = generateRookAttackMask<true>(sqr, occupancy);
        }
    }
}

void generateBishopTable(std::array<std::array<u64, 1024>, 64>& result) {
    for (u8 sqr = 0; sqr < 64; ++sqr) {

        u64 attkMask = tables::getBishopAttacks()[sqr];
        u64 occupancyVariations = 1ULL << intrinsics::popcnt(attkMask);

        for (u64 i = 0; i < occupancyVariations; ++i) {
            u64 occupancy = intrinsics::pdep(i, attkMask);
            u64 magicIndex = (occupancy * magics::constants::bishop[sqr]) >> (magics::constants::bishop_shifts[sqr]);
            result[sqr][static_cast<size_t>(magicIndex)] = generateBishopAttackMask<true>(sqr, occupancy);
        }
    }
}

void initialize() {
    knightAttacks = generateKnightAttackTable();
    rookAttacks = generateRookAttackTable();
    bishopAttacks = generateBishopAttackTable();
    generateRookTable(rookAttacksTable);
    generateBishopTable(bishopAttacksTable);
}

}  // namespace internals

namespace tables {
const std::array<u64, 64>& getKnightAttacks() {
    return internals::knightAttacks;

}
const std::array<u64, 64>& getRookAttacks() {
    return internals::rookAttacks;
}
const std::array<u64, 64>& getBishopAttacks() {
    return internals::bishopAttacks;
}
const std::array<std::array<u64, 4096>, 64>& getRookAttacksTable() {
    return internals::rookAttacksTable;
}
const std::array<std::array<u64, 1024>, 64>& getBishopAttacksTable() {
    return internals::bishopAttacksTable;
}

}  // namespace tables
}  // namespace attacks