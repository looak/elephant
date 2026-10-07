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

    std::string toString() const {
        std::stringstream ss;
        for (u32 i = 0; i < length; ++i) {
            ss << moves[i].toString() << " ";
        }
        return ss.str();
    }
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