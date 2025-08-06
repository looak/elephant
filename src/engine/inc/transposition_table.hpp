#pragma once
#include "defines.hpp"
#include "log.h"
#include <move/move.hpp>
#include "search_constants.hpp"

#include <algorithm>
#include <optional>
#include <vector>

struct Move;

// absolute maximum size of the transposition table
constexpr u32 c_tableMaxSize = 1024; // 1 gb

#ifdef DEBUG_TRANSITION_TABLE
static u64 s_writes;
static u64 s_reads;
static u64 s_hits;
static u64 s_overwrites;
static u64 s_age_replaced;
static u64 s_hash_collisions;
#endif

enum TranspositionFlag {
    TTF_NONE = 0,
    TTF_CUT_BETA = 1,
    TTF_CUT_ALPHA = 2,
    TTF_CUT_EXACT = 3
};

/**
 * Currently 128-bits but not using all of them:
 * Transposition Entry bits
 * - hash: 64-bits
 * - move: 16-bits
 * - score: 16-bits
 * - depth: 8-bits
 * - flag: 2-bits
 * - age: 6-bits    */
struct TranspositionEntry
{
    u64 hash;
    PackedMove move;
    i16 score;
    u8 depth;
    byte flag;
    u16 age;

    inline bool exact() const { return flag == TranspositionFlag::TTF_CUT_EXACT; }
    inline bool beta() const { return flag == TranspositionFlag::TTF_CUT_BETA; }
    inline bool alpha() const { return flag == TranspositionFlag::TTF_CUT_ALPHA; }
    inline bool valid() const { return flag != TranspositionFlag::TTF_NONE; }

    inline i16 adjustedScore(i32 ply) const {
        if (score >= c_checkmateMinScore)
            return score - ply;
        if (score <= -c_checkmateMinScore)
            return score + ply;
        return score;
    }

    inline void update(u64 hash, PackedMove move, u32 age, i16 score, i32 ply, u8 depth, TranspositionFlag flag) {
#ifdef DEBUG_TRANSITION_TABLE
        if (this->hash == 0)
            s_writes++;
        else
            s_overwrites++;
#endif
        if (this->depth > depth && this->age > age)
            return;

        this->hash = hash;
        this->move = move;
        this->age = age;
        this->depth = depth;
        this->flag = flag;
        this->score = score >= c_checkmateMinScore ? score + ply : score <= -c_checkmateMinScore ? score - ply : score;
    }

    /* @brief evaluate if this node is useful for the current search, i.e. a cut node or not.
     * @param posHash current position we're trying to evaluate
     * @param depth current depth of the search
     * @param alpha current alpha value
     * @param beta current beta value
     * @return optioanl score if this node is useful, otherwise nullopt    */
    std::optional<i32> evaluate(u64 posHash, u8 depth, i32 alpha, i32 beta) const {
#ifdef DEBUG_TRANSITION_TABLE
        s_reads++;
#endif
        if (this->hash != posHash)
            return std::nullopt;

        if (this->depth >= depth) {
#ifdef DEBUG_TRANSITION_TABLE
            s_hits++;
#endif
            if (this->exact())
                return this->score;
            else if (this->alpha() && this->score <= alpha)
                return this->score;
            else if (this->beta() && this->score >= beta)
                return this->score;
        }

        return std::nullopt;
    }
};

static_assert(sizeof(TranspositionEntry) == 16, "TranspositionEntry size is not 14 bytes");

template<typename T>
class TranspositionTableImpl {
public:
    TranspositionTableImpl();
    void resize(u32 megabytes);
    void clear();

    //inline u64 entryIndex(u64 hash) const { return ((i128)hash * (i128)m_elementCountMax) >> 64; }
    inline u64 entryIndex(u64 hash) const { return hash & m_mask; }

    inline u64 readSize() const { return m_table.size(); }
    inline u64 readSizeMegaBytes() const { return m_table.size() * sizeof(T) / (1024 * 1024); }

    const T& readEntry(u64 hash) const { return m_table[entryIndex(hash)]; }
    T& editEntry(u64 hash) { return m_table[entryIndex(hash)]; }

    PackedMove probe(u64 boardHash) const;
    std::pair<PackedMove, i32> probeScore(u64 boardHash) const;


#ifdef DEBUG_TRANSITION_TABLE
    void debugStatistics() const;
#endif

private:
    std::vector<T> m_table;
    u64 m_elementCountMax;
    u64 m_mask;
};


template<class T>
TranspositionTableImpl<T>::TranspositionTableImpl() :
    m_table(),
    m_elementCountMax(0)
{
    static const u32 defaultSize = 8; // 8mb
    resize(defaultSize);

#ifdef DEBUG_TRANSITION_TABLE
    LOG_INFO() << "TranspositionTableImpl debugging enabled";
    s_writes = 0;
    s_reads = 0;
    s_hits = 0;
    s_overwrites = 0;
    s_age_replaced = 0;
#endif
}

template<class T>
void TranspositionTableImpl<T>::resize(u32 megabytes)
{
    u64 newSize = (std::min(c_tableMaxSize, megabytes) * 1024 * 1024) / sizeof(T);
    LOG_WARNING_EXPR(megabytes < c_tableMaxSize) << "TranspositionTableImpl::resize() requested size is too large, resizing to "
        << c_tableMaxSize << "mb instead of " << megabytes << "mb.";

    this->clear();
    m_table.resize(newSize);
    m_table.shrink_to_fit();
    m_elementCountMax = newSize;
    m_mask = newSize - 1;
}

template<class T>
void TranspositionTableImpl<T>::clear()
{
    m_table.clear();
#ifdef DEBUG_TRANSITION_TABLE
    s_writes = 0;
    s_reads = 0;
    s_hits = 0;
    s_overwrites = 0;
    s_age_replaced = 0;
#endif
}

template<class T>
PackedMove TranspositionTableImpl<T>::probe(u64 boardHash) const {
    auto entry = m_table[entryIndex(boardHash)];
    if (boardHash == entry.hash && entry.exact())
        return entry.move;

    return PackedMove::NullMove();
}

template<class T>
std::pair<PackedMove, i32> TranspositionTableImpl<T>::probeScore(u64 boardHash) const {
    auto entry = m_table[entryIndex(boardHash)];
    if (boardHash == entry.hash && entry.exact())
        return std::make_pair(entry.move, entry.score);

    return std::make_pair(PackedMove::NullMove(), 0);
}

#ifdef DEBUG_TRANSITION_TABLE
template<class T>
void TranspositionTableImpl<T>::debugStatistics() const
{
    LOG_INFO() << "TranspositionTable:\n\t" << s_writes << " writes,\n\t" << s_age_replaced << " aged-out,\n\t" << s_reads << " reads,\n\t" << s_hits << " hits,\n\t" << s_overwrites << " overwrites";
    LOG_INFO() << "Size: " << readSize() << " elements, " << readSizeMegaBytes() << "mb";
}
#endif

typedef TranspositionTableImpl<TranspositionEntry> TranspositionTable;