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
#if defined(_MSC_VER)
#include <intrin.h>
#endif
#include <system/platform.hpp>
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
#include <thread>


struct PackedMove;
using Score = i16;

namespace transposition_table {

// the debug table's logging, in transposition_table.cpp: the header doesn't pull in the logger.
void logStats(u64 probes, u64 hits, u64 misses, u64 collisions, u64 stores, u64 overwrites);
void logSuspiciousScore(const char* where, u64 hash, i16 score, u8 depth);

    namespace constants {
    // absolute maximum size of the transposition table
    constexpr u32 c_tableMaxSize = 32768; // 32 gb
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
// without lock-free 64-bit atomics atomic_ref falls back to a lock per access, the table would still work but crawl.
static_assert(std::atomic_ref<u64>::is_always_lock_free, "transposition table entries must be lock-free 64-bit atomics");

// The high 64 bits of a * b, maps a hash onto [0, b) using its high bits, any b works, not just powers of two.
inline u64 mul_hi64(u64 a, u64 b) {
#if defined(__SIZEOF_INT128__)
    // __extension__, __int128 isn't ISO C++ and -Wpedantic would warn about it.
    __extension__ using u128 = unsigned __int128;
    return static_cast<u64>((static_cast<u128>(a) * b) >> 64);
#elif defined(_MSC_VER) && defined(_M_X64)
    return __umulh(a, b);
#else
    const u64 aLow = a & 0xFFFFFFFF, aHigh = a >> 32, bLow = b & 0xFFFFFFFF, bHigh = b >> 32;
    const u64 middle = (aLow * bLow >> 32) + (aHigh * bLow & 0xFFFFFFFF) + aLow * bHigh;
    return aHigh * bHigh + (aHigh * bLow >> 32) + (middle >> 32);
#endif
}

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

        transposition_table::logStats(p, h, m, c, s, o);
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
    // trivial buckets, allocated uninitialized and zeroed by clear() with several threads.
    std::unique_ptr<transposition_table::bucket[]> m_table;
    size_t m_bucketCount = 0;
    size_t m_size = 0;   // Number of entries
    u8 m_age;       // Current age counter (0-63)

    //[[no_unique_address]]
    TTStats<Debug> m_stats;


    // --- Packing and Unpacking ---
    // Pack an entry into its 64-bit word
    // Layout: [16-bit key][16-bit move][16-bit score][8-bit depth][2-bit flags][6-bit age]
    static u64 pack(u16 key, PackedMove move, Score score, u8 depth, TranspositionFlag bound, u8 age);
    // Unpack 64-bit data into components
    static void unpack(u64 data, u16& key, PackedMove& move, Score& score, u8& depth, TranspositionFlag& bound, u8& age);
    // The bucket comes from the high bits of the hash (mul_hi64), the key is the low 16 bits.
    static u16 hash_key(u64 hash) { return (u16)hash; }
    transposition_table::bucket& bucket_for(u64 hash) const { return m_table[transposition_table::mul_hi64(hash, m_bucketCount)]; }
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
    // size_mb: Size of table in megabytes, all of it is used.
    explicit TranspositionTableImpl(size_t size_mb = 64);

    // --- Probe and Store ---
    bool probe(u64 hash, PackedMove& move, Score& score, u8& depth, TranspositionFlag& bound);
    void store(u64 hash, PackedMove move, Score score, u8 depth, TranspositionFlag bound);
    void prefetch(u64 hash) const { _mm_prefetch(reinterpret_cast<const char*>(&bucket_for(hash)), _MM_HINT_T0); }
    
    // --- Age Management ---    
    void incrementAge() { m_age = (m_age + 1) & 0x3F;  /* Wrap at 64 */ }    
    u8 getAge() const { return m_age; }
    void setAge(u8 age) { m_age = age & 0x3F; }

    // --- Table Management ---    
    // threads split the table between them, a large Hash clears in a fraction of the time.
    void clear(u32 threads = 1);
    void resize(size_t size_mb, u32 threads = 1);
    size_t size() const { return m_size; }
    size_t size_bytes() const { return m_bucketCount * sizeof(transposition_table::bucket); }
    // UCI hashfull, permille of the first 1000 entries that were stored during the current search.
    u32 hashfull() const;
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
void TranspositionTableImpl<Debug>::resize(size_t size_mb, u32 threads) {
    size_mb = std::clamp<size_t>(size_mb, 1, transposition_table::constants::c_tableMaxSize);
    const size_t num_buckets = size_mb * 1024 * 1024 / sizeof(transposition_table::bucket);

    // the old table is released first, both at once may not fit.
    m_table.reset();
    m_table.reset(new transposition_table::bucket[num_buckets]);
    m_bucketCount = num_buckets;
    m_size = num_buckets * transposition_table::constants::c_bucketEntries;
    clear(threads);
}

template<bool Debug>
void TranspositionTableImpl<Debug>::clear(u32 threads) {
    threads = std::clamp<u32>(threads, 1, 64);
    const size_t chunk = (m_bucketCount + threads - 1) / threads;
    auto clearRange = [this, chunk](size_t first) {
        const size_t last = std::min(first + chunk, m_bucketCount);
        if (first < last)
            std::memset(static_cast<void*>(&m_table[first]), 0, (last - first) * sizeof(transposition_table::bucket));
    };

    std::vector<std::thread> workers;
    for (u32 i = 1; i < threads; ++i)
        workers.emplace_back(clearRange, i * chunk);
    clearRange(0);
    for (std::thread& worker : workers)
        worker.join();

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
    
    transposition_table::bucket& bucket = bucket_for(hash);
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
            // a score between the evaluation's & the mate scores was mangled on the way in or out.
            if (score > 16000 && score < 23000)
                transposition_table::logSuspiciousScore("probe", hash, score, depth);
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
        if (score > 16000 && score < 23000)
            transposition_table::logSuspiciousScore("store", hash, score, depth);
    }

    transposition_table::bucket& bucket = bucket_for(hash);
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

template<bool Debug>
u32 TranspositionTableImpl<Debug>::hashfull() const {
    // 125 buckets of 8 are 1000 entries, the table always has more.
    constexpr size_t sampleBuckets = 1000 / transposition_table::constants::c_bucketEntries;
    u32 used = 0;
    for (size_t i = 0; i < std::min(sampleBuckets, m_bucketCount); ++i) {
        for (u64& entry : m_table[i].entries) {
            u64 data = std::atomic_ref<u64>(entry).load(std::memory_order_relaxed);
            if (!is_empty(data) && extract_age(data) == m_age)
                ++used;
        }
    }
    return used;
}
