#include "bitboard/rays/rays.hpp"

namespace ray {
namespace internals {
std::array<std::array<u64, 64>, 64> raysTable;

void initialize() {
    raysTable = computeRays();
}

} // namespace internals

u64 getRay(u8 from, u8 to) {
    return internals::raysTable[from][to];
}

} // namespace ray