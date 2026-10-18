#pragma once
#include <move/move.hpp>
#include <sstream>

struct PVLine {
    static const u32 c_maxPVLength = 128;
    PackedMove moves[c_maxPVLength];
    u32 length = 0;

    void clear() {
        length = 0;
        for (u32 i = 0; i < c_maxPVLength; ++i) {
            moves[i] = PackedMove::NullMove();
        }
    }

    // quiescence's line policy, see NoQuiescenceLine: empty, or move followed by child's line.
    void reset() { length = 0; }
    void raise(PackedMove move, const PVLine& child) {
        moves[0] = move;
        const u32 childLength = child.length < c_maxPVLength - 1 ? child.length : c_maxPVLength - 1;
        for (u32 i = 0; i < childLength; ++i)
            moves[i + 1] = child.moves[i];
        length = childLength + 1;
    }

    std::string toString() const {
        std::stringstream ss;
        for (u32 i = 0; i < length; ++i) {
            ss << moves[i].toString() << " ";
        }
        return ss.str();
    }
};

/**
 * Search::quiescence's line policy when nothing records the line, the search's own path. Both calls compile away.
 * PVLine is the policy that records it: reset() when a node starts, raise(move, child) when move raises alpha, the
 * tuner fits the evaluation on the position at the end of that line.    */
struct NoQuiescenceLine {
    constexpr void reset() {}
    constexpr void raise(PackedMove, const NoQuiescenceLine&) {}
};

/**
 * @brief Represents the result of a search.   */
struct SearchResult {
    i32 score;
    PackedMove move() { return pvLine.moves[0]; };
    bool ForcedMate = false;
    u64 count = 0;
    PVLine pvLine;    
};