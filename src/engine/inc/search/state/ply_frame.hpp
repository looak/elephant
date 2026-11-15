#pragma once
#include <array>

#include <diagnostics/assert.hpp>
#include <move/move.hpp>
#include <search/core/search_constants.hpp>
#include <search/state/search_heuristic_structures.hpp>
#include <system/platform.hpp>

// What one ply of the line being searched knows. Lives for one go, on the search thread's stack with its ThreadContext, so
// nothing here can carry over to the next search.
struct PlyFrame {
    // the continuation history replies to the move made at this ply. A null move, or a ply before the root, points at the
    // shared table of no previous move.
    PieceToHistory* continuation = &heuristic_defaults::noPreviousMove;
    // quiet moves that failed high at this ply, indexed by ply so they only ever meet positions at the same distance.
    PackedMove killers[2] = { PackedMove::NullMove(), PackedMove::NullMove() };
    // the fifty move rule counter of the position at this ply.
    u16 halfmoveClock = 0;
    // set while the reply to a null move made at this ply is searched, so the reply doesn't null move straight back.
    bool nullMove = false;
    // set while this node searches its other moves without the TT move, to see if the TT move is the only good one. The
    // search at this ply skips it, and stays out of the TT.
    PackedMove excludedMove = PackedMove::NullMove();

    void pushKiller(PackedMove move) {
        if (killers[0] != move) {
            killers[1] = killers[0];
            killers[0] = move;
        }
    }
};

// The frames of a search thread, by ply with the root at 1. The frames before the root are there to look back on, they keep
// their defaults.
class PlyFrames {
public:
    [[nodiscard]] PlyFrame& operator[](i32 ply) { return m_frames[index(ply)]; }
    [[nodiscard]] const PlyFrame& operator[](i32 ply) const { return m_frames[index(ply)]; }

private:
    static size_t index(i32 ply) {
        ASSERT_MSG(ply >= 1 - static_cast<i32>(continuation_params::pliesBack) && ply <= static_cast<i32>(c_maxSearchDepth),
                   "A ply outside the frames.");
        return static_cast<size_t>(ply + continuation_params::pliesBack);
    }

    std::array<PlyFrame, c_maxSearchDepth + 1 + continuation_params::pliesBack> m_frames{};
};
