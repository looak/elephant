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
 * @file transposition_table.hpp
 * @brief Implements a transposition table for efficient move retrieval in search algorithms 
 * 
 */

#pragma once
#include <xmmintrin.h>
#include <system/platform.hpp>
#include <diagnostics/logger.hpp>
#include <move/move.hpp>
#include <search/search_constants.hpp>
#include "transposition_table_fwd.hpp"

#include <algorithm>
#include <optional>
#include <vector>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>


struct PackedMove;
using Score = i16;

namespace transposition_table {

    namespace constants {
    // absolute maximum size of the transposition table
    constexpr u32 c_tableMaxSize = 1024; // 1 gb
    constexpr u8 c_maxAge = 255;
    constexpr u32 c_bucketEntries = 8;
    } // namespace constants

/**
 * A bucket is one cache line of 8 entries, a probe reads a single line. Each entry is one 64-bit word, read and
 * written atomically, so threads sharing the table never see half an entry.
 * Entry layout: [16-bit key][16-bit move][16-bit score][8-bit depth][2-bit bound][6-bit age]
 * The bucket index comes from the low bits of the hash and the key from the top 16, an entry with no bound is empty. */
struct alignas(64) bucket {
    u64 entries[constants::c_bucketEntries];
};

static_assert(sizeof(bucket) == 64, "transposition table bucket must be exactly one cache line");
static_assert(std::is_trivial_v<bucket>, "transposition table bucket must be a trivial type");

} // namespace transposition_table

enum TranspositionFlag : u8 {
    TTF_NONE = 0,
    TTF_CUT_EXACT = 1,
    TTF_CUT_ALPHA = 2,
    TTF_CUT_BETA = 3,
};

// ============================================================================
// Debug Statistics (compiled out when Debug=false)
// ============================================================================
template<bool Enabled>
struct TTStats {
    // Empty in release mode - compiler will optimize this away completely
};

template<>
struct TTStats<true> {
    std::atomic<u64> probes{0};
    std::atomic<u64> hits{0};
    std::atomic<u64> misses{0};
    std::atomic<u64> collisions{0};
    std::atomic<u64> stores{0};
    std::atomic<u64> overwrites{0};
    
    void record_probe() { probes.fetch_add(1, std::memory_order_relaxed); }
    void record_hit() { hits.fetch_add(1, std::memory_order_relaxed); }
    void record_miss() { misses.fetch_add(1, std::memory_order_relaxed); }
    void record_collision() { collisions.fetch_add(1, std::memory_order_relaxed); }
    void record_store() { stores.fetch_add(1, std::memory_order_relaxed); }
    void record_overwrite() { overwrites.fetch_add(1, std::memory_order_relaxed); }

    constexpr double calculate_rate(u64 numerator, u64 denominator) const {
        return denominator > 0 ? static_cast<double>(numerator) / static_cast<double>(denominator) : 0.0;
    }
    
    void print_stats() const {
        u64 p = probes.load();
        u64 h = hits.load();
        u64 m = misses.load();
        u64 c = collisions.load();
        u64 s = stores.load();
        u64 o = overwrites.load();

        double missRate = calculate_rate(100 * m, p);
        double hitRate = calculate_rate(100 * h, p);
        double overwriteRate = calculate_rate(100 * o, s);
        
        spdlog::debug("=== Transposition Table Statistics ===");
        spdlog::debug("Probes:      {}", p);
        spdlog::debug("Hits:        {} ({:.2f}%)", h, hitRate);
        spdlog::debug("Misses:      {} ({:.2f}%)", m, missRate);
        spdlog::debug("Collisions:  {}", c);
        spdlog::debug("Stores:      {}", s);
        spdlog::debug("Overwrites:  {} ({:.2f}%)", o, overwriteRate);
        spdlog::debug("=====================================");
    }
    
    void reset() {
        probes = hits = misses = collisions = stores = overwrites = 0;
    }
};

// ============================================================================
// Lock-Free Transposition Table
// ============================================================================
template<bool Debug>
class TranspositionTableImpl {
private:
    std::vector<transposition_table::bucket> m_table;
    size_t m_size;       // Number of entries
    size_t m_mask;       // bucket count - 1, for fast modulo
    u8 m_age;       // Current age counter (0-63)

    //[[no_unique_address]]
    TTStats<Debug> m_stats;


    // --- Packing and Unpacking ---
    // Pack an entry into its 64-bit word
    // Layout: [16-bit key][16-bit move][16-bit score][8-bit depth][2-bit flags][6-bit age]
    static u64 pack(u16 key, PackedMove move, Score score, u8 depth, TranspositionFlag bound, u8 age);
    // Unpack 64-bit data into components
    static void unpack(u64 data, u16& key, PackedMove& move, Score& score, u8& depth, TranspositionFlag& bound, u8& age);
    // The key is the top 16 bits of the hash, the bucket index comes from the low bits.
    static u16 hash_key(u64 hash) { return (u16)(hash >> 48); }
    // Extract just the key for fast verification
    static u16 extract_key(u64 data) { return (u16)(data >> 48); }
    static u16 extract_move(u64 data) { return (u16)(data >> 32); }
    // Extract just the age for replacement policy
    static u8 extract_age(u64 data) { return (u8)(data & 0x3F); }
    // Extract just the depth for replacement policy
    static u8 extract_depth(u64 data) { return (u8)(data >> 8); }
    static TranspositionFlag extract_bound(u64 data) { return (TranspositionFlag)((data >> 6) & 0x3); }
    // A stored entry always has a bound, a cleared one is all zero.
    static bool is_empty(u64 data) { return extract_bound(data) == TTF_NONE; }

    // --- Replacement Policy ---
    // How much an entry is worth keeping when a different position needs its slot, deep and recent entries are worth
    // more. The least valuable entry in the bucket is replaced.
    i32 worth(u64 data) const;

public:
    // size_mb: Size of table in megabytes (will be rounded down to power of 2)
    explicit TranspositionTableImpl(size_t size_mb = 64);

    // --- Probe and Store ---
    bool probe(u64 hash, PackedMove& move, Score& score, u8& depth, TranspositionFlag& bound);
    void store(u64 hash, PackedMove move, Score score, u8 depth, TranspositionFlag bound);
    void prefetch(u64 hash) const { _mm_prefetch(reinterpret_cast<const char*>(&m_table[hash & m_mask]), _MM_HINT_T0); }
    
    // --- Age Management ---    
    void incrementAge() { m_age = (m_age + 1) & 0x3F;  /* Wrap at 64 */ }    
    u8 getAge() const { return m_age; }
    void setAge(u8 age) { m_age = age & 0x3F; }

    // --- Table Management ---    
    void clear();
    void resize(size_t size_mb);    
    size_t size() const { return m_size; }
    size_t size_bytes() const { return m_table.size() * sizeof(transposition_table::bucket); }
    size_t size_mb() const { return size_bytes() / (1024 * 1024); }
    
    // ========================================================================
    // Debug Statistics (only available when Debug=true)
    // ========================================================================
    
    template<bool D = Debug>
    std::enable_if_t<D> print_stats() const {
        m_stats.print_stats();
    }
    
    template<bool D = Debug>
    std::enable_if_t<D> reset_stats() {
        m_stats.reset();
    }
    
    template<bool D = Debug>
    std::enable_if_t<D, u64> get_probes() const {
        return m_stats.probes.load();
    }
    
    template<bool D = Debug>
    std::enable_if_t<D, u64> get_hits() const {
        return m_stats.hits.load();
    }
    
    template<bool D = Debug>
    std::enable_if_t<D, double> get_hit_rate() const {
        u64 p = m_stats.probes.load();
        u64 h = m_stats.hits.load();
        return calculate_rate<D>(h, p);
    }
};

template<bool Debug>
TranspositionTableImpl<Debug>::TranspositionTableImpl(size_t size_mb) 
    : m_age(0) 
{    
    resize(size_mb);
}

template<bool Debug>
void TranspositionTableImpl<Debug>::resize(size_t size_mb) {
    clear();
    size_t size_bytes = size_mb * 1024 * 1024;
    size_t num_buckets = size_bytes / sizeof(transposition_table::bucket);
    
    // Round down to power of 2 for fast masking
    size_t power = 0;
    while ((1ULL << (power + 1)) <= num_buckets) {
        power++;
    }
    num_buckets = (size_t)1 << power;
    m_mask = num_buckets - 1;
    
    m_table.resize(num_buckets);
    m_size = num_buckets * transposition_table::constants::c_bucketEntries;
}

template<bool Debug>
void TranspositionTableImpl<Debug>::clear() {
    std::fill(m_table.begin(), m_table.end(), transposition_table::bucket{});
    m_age = 0;
    if constexpr (Debug) {
        m_stats.reset();
    }
}

template<bool Debug>
bool TranspositionTableImpl<Debug>::probe(u64 hash, PackedMove& move, Score& score, u8& depth, TranspositionFlag& bound) {
    if constexpr (Debug) {
        m_stats.record_probe();
    }
    
    transposition_table::bucket& bucket = m_table[hash & m_mask];
    const u16 search_key = hash_key(hash);

    for (u64& entry : bucket.entries) {
        // the whole entry is one atomic word, a matching key always comes with its own move & score.
        u64 data = std::atomic_ref<u64>(entry).load(std::memory_order_relaxed);
        if (is_empty(data) || extract_key(data) != search_key)
            continue;

        u16 stored_key;
        u8 stored_age;
        unpack(data, stored_key, move, score, depth, bound, stored_age);

        if constexpr (Debug) {
            m_stats.record_hit();
        }

        // TRAP: Did we retrieve a mangled score?
        if (score > 16000 && score < 23000) {
             spdlog::warn("TT TRAP PROBE: Suspicious Score Retrieved! Hash:{:X} Score:{} (0x{:X})", hash, score, score);
        }
        return true;
    }

    if constexpr (Debug) {
        m_stats.record_miss();
    }
    return false;
}

template<bool Debug>
void TranspositionTableImpl<Debug>::store(u64 hash, PackedMove move, Score score, u8 depth, TranspositionFlag bound) {
    if constexpr (Debug) {
        m_stats.record_store();
    }
    if (score > 16000 && score < 23000) {
        spdlog::warn("TT TRAP STORE: Suspicious Score! Hash:{:X} Score:{} (0x{:X}) Depth:{}", hash, score, score, depth);
    }

    transposition_table::bucket& bucket = m_table[hash & m_mask];
    const u16 key = hash_key(hash);

    // the slot is this position's own entry or the first empty one, otherwise the least valuable entry in the bucket.
    // Entries fill a bucket from the front, so the empty ones come after any match.
    u64* slot = nullptr;
    u64 old_data = 0;
    i32 lowest_worth = std::numeric_limits<i32>::max();
    for (u64& entry : bucket.entries) {
        u64 data = std::atomic_ref<u64>(entry).load(std::memory_order_relaxed);
        if (is_empty(data) || extract_key(data) == key) {
            slot = &entry;
            old_data = data;
            break;
        }
        i32 entry_worth = worth(data);
        if (entry_worth < lowest_worth) {
            lowest_worth = entry_worth;
            slot = &entry;
            old_data = data;
        }
    }

    if (!is_empty(old_data) && extract_key(old_data) == key) {
        // the same position is always replaced, the newest result is the most relevant. Keeping a deeper bound from an
        // earlier iteration over it pushed the null move zugzwang test (search_test.cpp) from depth 18 to 24.
        // A fail-low has no best move, keep the one found earlier.
        if (move.isNull())
            move = PackedMove(extract_move(old_data));
    }
    else if constexpr (Debug) {
        if (!is_empty(old_data)) {
            m_stats.record_overwrite();
            m_stats.record_collision();
        }
    }

    std::atomic_ref<u64>(*slot).store(pack(key, move, score, depth, bound, m_age), std::memory_order_relaxed);
}

template<bool Debug>
u64 TranspositionTableImpl<Debug>::pack(u16 key, PackedMove move, Score score, u8 depth, TranspositionFlag bound, u8 age) {
    u64 data = 0;
    data |= (u64)key << 48;                    // Bits 63-48: key
    data |= (u64)move.read() << 32;            // Bits 47-32: move
    data |= (u64)(uint16_t)score << 16;        // Bits 31-16: score
    data |= (u64)depth << 8;                   // Bits 15-8:  depth
    data |= (u64)((u8)bound & 0x3) << 6;       // Bits 7-6:   flags
    data |= (u64)(age & 0x3F);                 // Bits 5-0:   age
    return data;
}

template<bool Debug>
void TranspositionTableImpl<Debug>::unpack(u64 data, u16& key, PackedMove& move, Score& score, u8& depth, TranspositionFlag& bound, u8& age) {
    key = extract_key(data);
    move = PackedMove(extract_move(data));
    score = (Score)(uint16_t)(data >> 16);
    depth = extract_depth(data);
    bound = extract_bound(data);
    age = extract_age(data);
}

template<bool Debug>
i32 TranspositionTableImpl<Debug>::worth(u64 data) const {
    // searches since the entry was stored, the age wraps at 64.
    const i32 searches_old = (m_age - extract_age(data)) & 0x3F;
    return static_cast<i32>(extract_depth(data)) - 8 * searches_old;
}

