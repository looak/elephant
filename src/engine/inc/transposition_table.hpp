#pragma once
#include "defines.h"
#include "move.h"

#include <unordered_map>

#ifndef DEBUG_SEARCHING
    #define DEBUG_SEARCHING
#endif

class Move;
/**
 * Transposition Entry bits
 * - move: 16-bits
 * - score: 16-bits
 * - depth: 8-bits
 * - flag: 4-bits
 * - age: 4-bits
 * 
 * Transposition Entry Flag */

enum TranspositionFlag
{
    TTF_NONE = 0,
    TTF_CUT_EXACT = 1,
    TTF_CUT_BETA = 2,
    TTF_CUT_ALPHA = 4
};

struct TranspositionEntry
{    
    PackedMove move;
    i16 score;
    u8 depth;
    byte flag : 4;
    byte age : 4;

    inline bool exact() const { return flag & TranspositionFlag::TTF_CUT_EXACT; }
    inline bool beta() const { return flag & TranspositionFlag::TTF_CUT_BETA; }
    inline bool alpha() const { return flag & TranspositionFlag::TTF_CUT_ALPHA; }
};

class TranspositionTable
{
public:
    TranspositionTable();

    void clear();
    void store(u64 boardHash, Move mv, u8 depth, i32 score, TranspositionFlag flag);
    bool probe(u64 boardHash, u8 depth, i32 alpha, i32 beta, i32& score) const;

#ifdef DEBUG_SEARCHING
    void debugStatistics() const;
#endif

private:
    std::unordered_map<uint64_t, TranspositionEntry> m_table;

#ifdef DEBUG_SEARCHING
    mutable i32 m_writes;
    mutable i32 m_reads;
    mutable i32 m_hits;
    mutable i32 m_overwrites;
#endif

};