#include "transposition_table.hpp"

#include "defines.h"
#include "move.h"

TranspositionTable::TranspositionTable()
{
    m_table.reserve(1024*8); // 8MB
#ifdef DEBUG_SEARCHING
    m_writes = 0;
    m_reads = 0;
    m_hits = 0;
    m_overwrites = 0;
#endif
}

void TranspositionTable::clear()
{
    m_table.clear();
#ifdef DEBUG_SEARCHING
    m_writes = 0;
    m_reads = 0;
    m_hits = 0;
    m_overwrites = 0;
#endif
}

bool TranspositionTable::probe(u64 boardHash, u8 depth, i32 alpha, i32 beta, i32& score) const
{
#ifdef DEBUG_SEARCHING
    ++m_reads;
#endif
    const auto itr = m_table.find(boardHash);
    if (itr == m_table.end())
        return false;

    const auto& entry = itr->second;
    if (entry.depth >= depth)
    {
#ifdef DEBUG_SEARCHING
    ++m_hits;
#endif
        if (entry.exact())
        {
            score = entry.score;
            return true;
        }
        if (entry.alpha() && entry.score <= alpha)
        {
            score = alpha;
            return true;
        }
        if(entry.beta() && entry.score >= beta)
        {
            score = beta;
            return true;
        }
    }

    return false;
}

void TranspositionTable::store(u64 boardHash, Move mv, u8 depth, i32 score, TranspositionFlag flag)
{
    const auto& itr = m_table.find(boardHash);
    if (itr == m_table.end())
    {
#ifdef DEBUG_SEARCHING
    ++m_writes;
#endif
        TranspositionEntry entry;
        entry.move = mv.readPackedMove();
        entry.flag = flag;
        entry.depth = depth;
        entry.score = (i16)score;

        m_table.emplace(boardHash, entry);        
    }
    else
    {
#ifdef DEBUG_SEARCHING
    ++m_overwrites;
#endif        
        auto& entry = m_table[boardHash];
        entry.score = (i16)score;
        entry.depth = depth;
        entry.flag = flag;
        entry.move = mv.readPackedMove();
    }
}

#ifdef DEBUG_SEARCHING
    void TranspositionTable::debugStatistics() const
    {
        LOG_DEBUG() << "TranspositionTable:\n\t" << m_writes << " writes,\n\t" << m_reads << " reads,\n\t" << m_hits << " hits,\n\t" << m_overwrites << " overwrites";
    }
#endif