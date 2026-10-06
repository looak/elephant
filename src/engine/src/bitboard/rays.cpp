#include "bitboard/rays/rays.hpp"

namespace ray {
namespace internals {
std::array<std::array<u64, 64>, 64> raysTable;
std::array<std::array<u64, 64>, 64> linesTable;

void initialize() {
    raysTable = computeRays();
    linesTable = computeLines();
}

} // namespace internals

u64 getRay(u32 from, u32 to) {
    return internals::raysTable[from][to];
}

u64 getLine(u32 from, u32 to) {
    return internals::linesTable[from][to];
}

} // namespace ray