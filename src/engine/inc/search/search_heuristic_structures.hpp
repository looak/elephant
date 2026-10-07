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
#pragma once
#include <system/platform.hpp>
#include <move/move.hpp>
#include <search/search_constants.hpp>
#include <vector>

struct MoveHistory {
private:
    std::vector<u64> recentHashes;

public:
    MoveHistory() {
        // rarely will we have a game that goes this deep.
        recentHashes.reserve(128);
    }

    void push(u64 hash) {
        recentHashes.push_back(hash);
    }
    void pop() {
        if (!recentHashes.empty()) {
            recentHashes.pop_back();
        }
    }

    bool isRepetition(u64 hashKey) const {
        int occurrences = 0;
        for (auto it = recentHashes.rbegin(); it != recentHashes.rend(); ++it) {
            if (*it == hashKey) {
                occurrences++;
                if (occurrences >= 3)
                    return true;
            }
        }        
        return false;
    }
};

struct KillerMoves {
    private:
    PackedMove m_killers[c_maxSearchDepth][2];

    public:
    KillerMoves() {
        clear();
    }
    void clear() {
        for (u32 i = 0; i < c_maxSearchDepth; ++i) {
            m_killers[i][0] = PackedMove::NullMove();
            m_killers[i][1] = PackedMove::NullMove();
        }
    }
    void push(PackedMove move, u16 ply) {
        if (m_killers[ply][0] != move) {
            m_killers[ply][1] = m_killers[ply][0];
            m_killers[ply][0] = move;
        }
    }

    void retrieve(u16 ply, PackedMove& outFirst, PackedMove& outSecond) const {
        outFirst = m_killers[ply][0];
        outSecond = m_killers[ply][1];
    }
};

struct MoveOrderingHeuristic {
    KillerMoves killers;
    //i32 score;
};
