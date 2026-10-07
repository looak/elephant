#pragma once

#include <move/move.hpp>
#include <system/platform.hpp>

// A plain data view into heuristics owned by the Search module.
// This has no virtual functions and allows MoveGenerator to remain
// decoupled and fast.
struct MoveOrderingView {
    PackedMove pvMove = PackedMove::NullMove();
    PackedMove ttMove = PackedMove::NullMove();
    PackedMove killers[2];    
    const i32* history = nullptr; // Pointer to a flat history table

    // Helper to safely access history score
    inline i32 getHistoryScore(Set side, Square src, Square dst) const noexcept {
        if (history == nullptr) return 0;
        // Assumes a layout like history[side][src][dst]
        return history[static_cast<u8>(side) * 64 * 64 + static_cast<u8>(src) * 64 + static_cast<u8>(dst)];
    }
};
