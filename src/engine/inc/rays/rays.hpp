#pragma once
#include <array>

#include "attacks/attacks.hpp"
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

u64 getRay(u8 from, u8 to);

} // namespace ray