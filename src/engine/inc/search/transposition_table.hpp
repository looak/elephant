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
#include <memory>


struct PackedMove;
using Score = i16;

namespace transposition_table {

    namespace constants {
    // absolute maximum size of the transposition table
    constexpr u32 c_tableMaxSize = 1024; // 1 gb
    constexpr u8 c_maxAge = 255;
    } // namespace constants
    
    /**
 * Transposition Table Entry Structure 10 bytes -- padded to 16.
 *  move        : 2 bytes
 * data         : 8 bytes
 *  - key       : 4 bytes
 *  - score     : 2 bytes
 *  - depth     : 1 byte
 *  - flags
 *      & age   : 1 byte  */
struct alignas(16) entry {    
    // Written last, read first (atomic 64-bit)
    // Layout: [32-bit key][16-bit score][8-bit depth][2-bit flags][6-bit age]
    // Written first, read last.
    u64 data;

    // [16-bits move]
    // Written last, read first.
    PackedMove move;

    // paddoing  -- 6 bytes
    u8 padding[6];
};

static_assert(sizeof(entry) == 16, "transposition table entry must be exactly 16 bytes");
static_assert(std::is_trivial_v<entry>, "transposition table entry must be a trivial type");

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
    std::vector<transposition_table::entry> m_table;
    size_t m_size;       // Number of entries
    size_t m_mask;       // m_size - 1, for fast modulo
    u8 m_age;       // Current age counter (0-63)
    
    //[[no_unique_address]] 
    TTStats<Debug> m_stats;
    

    // --- Packing and Unpacking ---
    // Pack data into 64-bit atomic value
    // Layout: [32-bit key][16-bit score][8-bit depth][2-bit flags][6-bit age]
    static u64 pack(u32 key, Score score, u8 depth, TranspositionFlag bound, u8 age);
    // Unpack 64-bit data into components
    static void unpack(u64 data, u32& key, Score& score, u8& depth, TranspositionFlag& bound, u8& age);
    // Extract just the key for fast verification
    static u32 extract_key(u64 data) { return (u32)(data >> 32); }    
    // Extract just the age for replacement policy
    static u8 extract_age(u64 data) { return (u8)(data & 0x3F); }    
    // Extract just the depth for replacement policy
    static u8 extract_depth(u64 data) { return (u8)(data >> 8); }
    
    // --- Replacement Policy ---    
    bool should_replace(u64 oldData, u8 newData) const;

public:    
    // size_mb: Size of table in megabytes (will be rounded down to power of 2)
    explicit TranspositionTableImpl(size_t size_mb = 64);
    
    // --- Probe and Store ---    
    bool probe(u64 hash, PackedMove& move, Score& score, u8& depth, TranspositionFlag& bound);
    void store(u64 hash, PackedMove move, Score score, u8 depth, TranspositionFlag bound);
    
    // --- Age Management ---    
    void incrementAge() { m_age = (m_age + 1) & 0x3F;  /* Wrap at 64 */ }    
    u8 getAge() const { return m_age; }
    void setAge(u8 age) { m_age = age & 0x3F; }

    // --- Table Management ---    
    void clear();
    void resize(size_t size_mb);    
    size_t size() const { return m_size; }
    size_t size_bytes() const { return m_size * sizeof(transposition_table::entry); }
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
    size_t num_entries = size_bytes / sizeof(transposition_table::entry);
    
    // Round down to power of 2 for fast masking
    size_t power = 0;
    while ((1ULL << (power + 1)) <= num_entries) {
        power++;
    }
    num_entries = 1ULL << power;
    m_mask = num_entries - 1;
    
    m_table.resize(num_entries);
    m_size = num_entries;
}

template<bool Debug>
void TranspositionTableImpl<Debug>::clear() {
    std::fill(m_table.begin(), m_table.end(), transposition_table::entry{});
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
    
    // Get the entry index from lower bits of hash
    size_t index = hash & m_mask;
    transposition_table::entry& entry = m_table[index];
    
    // Step 1: Atomically read the 64-bit data block
    std::atomic_ref<u64> atomic_data(entry.data);
    u64 data = atomic_data.load(std::memory_order_acquire);
    
    // Step 2: Verify the hash key (upper 32 bits)    
    u32 search_key = (u32)(hash >> 32);        
    if (search_key != extract_key(data)) {
        // Key mismatch - either empty, different position, or write in progress
        if constexpr (Debug) {
            if (extract_key(data) != 0) {
                m_stats.record_collision();
            }
            m_stats.record_miss();
        }
        return false;
    }
    
    // Step 3: Key matches! Now it's safe to read the move
    move = entry.move;
    
    // Step 4: Unpack the data
    u8 stored_age;
    unpack(data, search_key, score, depth, bound, stored_age);
    
    if constexpr (Debug) {
        m_stats.record_hit();
    }
    
    return true;
}

template<bool Debug>
void TranspositionTableImpl<Debug>::store(u64 hash, PackedMove move, Score score, u8 depth, TranspositionFlag bound) {
    if constexpr (Debug) {
        m_stats.record_store();
    }    
    // Get the entry index from lower bits of hash
    size_t index = hash & m_mask;
    transposition_table::entry& entry = m_table[index];
    std::atomic_ref<u64> atomic_data(entry.data);
    
    // Check replacement policy
    u64 old_data = atomic_data.load(std::memory_order_relaxed);
    u32 old_key = extract_key(old_data);
    
    if (old_key != 0 && old_key != (u32)(hash >> 32)) {
        // Different position - check if we should replace
        if (!should_replace(old_data, depth)) {
            return;  // Keep existing entry
        }
        if constexpr (Debug) {
            m_stats.record_overwrite();
        }
    }
    
    // Step 1: Write the move (non-atomic)
    entry.move = move;
    
    // Step 2: Pack and atomically write the 64-bit data block
    u32 key = (u32)(hash >> 32);
    u64 new_data = pack(key, score, depth, bound, m_age);
    atomic_data.store(new_data, std::memory_order_release);
}

template<bool Debug>
u64 TranspositionTableImpl<Debug>::pack(u32 key, Score score, u8 depth, TranspositionFlag bound, u8 age) {
    u64 data = 0;
    data |= (u64)key << 32;                    // Bits 63-32: key
    data |= (u64)(uint16_t)score << 16;        // Bits 31-16: score
    data |= (u64)depth << 8;                   // Bits 15-8:  depth
    data |= (u64)((u8)bound & 0x3) << 6;       // Bits 7-6:   flags
    data |= (u64)(age & 0x3F);                 // Bits 5-0:   age
    return data;
}

template<bool Debug>
void TranspositionTableImpl<Debug>::unpack(u64 data, u32& key, Score& score, u8& depth, TranspositionFlag& bound, u8& age) {
    key = (u32)(data >> 32);
    score = (Score)(uint16_t)(data >> 16);
    depth = (u8)(data >> 8);
    bound = (TranspositionFlag)((data >> 6) & 0x3);
    age = (u8)(data & 0x3F);
}

template<bool Debug>
bool TranspositionTableImpl<Debug>::should_replace(u64 oldData, u8 newDepth) const {
    u8 oldDepth = extract_depth(oldData);
    u8 oldAge = extract_age(oldData);
    
    // Always replace if from a previous search (old age)
    if (oldAge != m_age) {
        return true;
    }
    
    // Replace if new search is deeper
    if (newDepth > oldDepth) {
        return true;
    }
    
    // Otherwise keep existing entry
    return false;
}

