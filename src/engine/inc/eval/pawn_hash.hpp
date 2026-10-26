// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2025  Alexander Loodin Ek

// This program is free software : you can redistribute it and /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.If not, see < http://www.gnu.org/licenses/>.

/**
 * @file pawn_hash.hpp
 * @brief A cache of pawnInfo by pawn structure. The pawns change on few moves, most positions a search evaluates
 * share their pawns with many others, so their score and passed pawns are looked up instead of counted again.
 *
 * Each entry keeps both sides' pawn bitboards, a hit is an exact match and never someone else's info. An entry never
 * written holds no pawns, a score of 0 and no passed pawns, which is right for a position without pawns, there's no
 * empty marker.
 * One table per search thread, nothing is shared or locked.    */

#pragma once

#include <algorithm>
#include <vector>

#include <eval/pawn_structure.hpp>
#include <eval/tapered_score.hpp>
#include <material/material_mask.hpp>
#include <system/platform.hpp>

class PawnHashTable {
public:
    // 2^14 entries of 40 bytes, 640 KB.
    static constexpr u32 c_indexBits = 14;

    PawnHashTable() : m_entries(std::size_t{ 1 } << c_indexBits) {}

    // pawnInfo of material, from the table when its pawns are there, counted and stored when not.
    [[nodiscard]] const PawnInfo& probe(const MaterialPositionMask& material) {
        const u64 white = material.pawns<Set::WHITE>().read();
        const u64 black = material.pawns<Set::BLACK>().read();
        Entry& entry = m_entries[index(white, black)];
        if (entry.white == white && entry.black == black) {
            ++m_hits;
            return entry.info;
        }
        ++m_misses;
        entry = { white, black, pawnInfo(material) };
        return entry.info;
    }

    void clear() {
        std::fill(m_entries.begin(), m_entries.end(), Entry{});
        m_hits = 0;
        m_misses = 0;
    }

    [[nodiscard]] u64 hits() const { return m_hits; }
    [[nodiscard]] u64 misses() const { return m_misses; }

private:
    struct Entry {
        u64 white = 0;
        u64 black = 0;
        PawnInfo info;
    };
    static_assert(sizeof(Entry) == 40, "An entry grew, the table's size above is out of date.");

    // both bitboards multiplied and the top bits of their mix taken, every pawn moves them. Pawns sit in the middle
    // ranks, a plain mask of the bitboards would put most structures in few entries.
    static u32 index(u64 white, u64 black) {
        const u64 mixed = (white * UINT64_C(0x9E3779B97F4A7C15)) ^ (black * UINT64_C(0xC2B2AE3D27D4EB4F));
        return static_cast<u32>(mixed >> (64 - c_indexBits));
    }

    std::vector<Entry> m_entries;
    u64 m_hits = 0;
    u64 m_misses = 0;
};
