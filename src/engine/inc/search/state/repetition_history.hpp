#pragma once
#include <algorithm>
#include <cstddef>
#include <span>
#include <vector>

#include <diagnostics/assert.hpp>
#include <system/platform.hpp>

// The positions a search can repeat: the game's before its root and its own line since. One per search thread for one go.
struct RepetitionHistory {
private:
    // the game's earlier positions, read only & shared by every thread of a search.
    std::span<const u64> gameHashes;
    // this thread's root and the line searched to here.
    std::vector<u64> recentHashes;

public:
    RepetitionHistory() {
        // rarely will we have a game that goes this deep.
        recentHashes.reserve(128);
    }

    void setGameHashes(std::span<const u64> hashes) {
        gameHashes = hashes;
    }

    void push(u64 hash) {
        recentHashes.push_back(hash);
    }
    void pop() {
        if (!recentHashes.empty()) {
            recentHashes.pop_back();
        }
    }

    // Threefold repetition of the last pushed position, hashKey. A capture or pawn move can't be undone, so only the
    // positions since the last one (halfmoveClock half moves back) can repeat it, and of those only the ones with the
    // same side to move, every second one.
    bool isRepetition(u64 hashKey, u16 halfmoveClock) const {
        ASSERT_MSG(!recentHashes.empty() && recentHashes.back() == hashKey, "the position checked has to be the last pushed.");
        const size_t lineLast = recentHashes.size() - 1;
        const size_t gameCount = gameHashes.size();
        const size_t window = std::min<size_t>(halfmoveClock, gameCount + lineLast);

        int occurrences = 1;  // the position itself
        size_t back = 2;
        for (; back <= std::min(window, lineLast); back += 2) {
            if (recentHashes[lineLast - back] == hashKey && ++occurrences >= 3)
                return true;
        }
        // only when the fifty move counter reaches past the root.
        for (; back <= window; back += 2) {
            if (gameHashes[gameCount + lineLast - back] == hashKey && ++occurrences >= 3)
                return true;
        }
        return false;
    }
};
