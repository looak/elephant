#pragma once

#include <move/move.hpp>
#include <system/platform.hpp>

// A plain data view into heuristics owned by the Search module.
// This has no virtual functions and allows MoveGenerator to remain
// decoupled and fast.
struct MoveOrderingView {
    // history scores stay within [-historyMax, historyMax].
    static constexpr i32 historyMax = 0x3FFF;

    PackedMove pvMove = PackedMove::NullMove();
    PackedMove ttMove = PackedMove::NullMove();
    PackedMove killers[2];    
    const i32* history = nullptr; // Pointer to a flat history table

    // continuation history pointers for negative plies (lookbacks)
    const i16* negativeOnePly = nullptr; 
    const i16* negativeTwoPly = nullptr; 

    // Helper to safely access history score
    inline i32 getHistoryScore(Set side, Square src, Square dst) const noexcept {
        if (history == nullptr) return 0;
        // Assumes a layout like history[side][src][dst]
        return history[static_cast<u8>(side) * 64 * 64 + static_cast<u8>(src) * 64 + static_cast<u8>(dst)];
    }

    // Continuation score helper
    inline i32 getContinuationScore(Set side, u8 pieceId, Square to) const noexcept {
        if (negativeOnePly == nullptr || negativeTwoPly == nullptr) return 0;
        const size_t index = static_cast<size_t>(static_cast<u8>(side) * 6 + pieceId);
        return negativeOnePly[index * 64 + static_cast<u8>(to)] + negativeTwoPly[index * 64 + static_cast<u8>(to)];
    }
};
