#include <search/transposition_table.hpp>
#include <diagnostics/logger.hpp>

namespace transposition_table {

namespace {
double percent(u64 numerator, u64 denominator) {
    return denominator > 0 ? 100.0 * static_cast<double>(numerator) / static_cast<double>(denominator) : 0.0;
}
} // namespace

void logStats(u64 probes, u64 hits, u64 misses, u64 collisions, u64 stores, u64 overwrites) {
    spdlog::debug("=== Transposition Table Statistics ===");
    spdlog::debug("Probes:      {}", probes);
    spdlog::debug("Hits:        {} ({:.2f}%)", hits, percent(hits, probes));
    spdlog::debug("Misses:      {} ({:.2f}%)", misses, percent(misses, probes));
    spdlog::debug("Collisions:  {}", collisions);
    spdlog::debug("Stores:      {}", stores);
    spdlog::debug("Overwrites:  {} ({:.2f}%)", overwrites, percent(overwrites, stores));
    spdlog::debug("=====================================");
}

void logSuspiciousScore(const char* where, u64 hash, i16 score, u8 depth) {
    spdlog::warn("TT TRAP {}: Suspicious Score! Hash:{:X} Score:{} (0x{:X}) Depth:{}", where, hash, score, score, depth);
}

} // namespace transposition_table
