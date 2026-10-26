// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021-2026  Alexander Loodin Ek

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
 * @file draw.hpp
 * @brief Training positions drawn from games: `phant-indexer --draw`. From each game that read and has a result, the
 * positions after the opening, naturally quiet ones, a sample of them, none twice in the run, as `<fen> [result]` lines,
 * the format ElephantTuner's dataset reader takes. Every position written is one the game reached.
 * @author Alexander Loodin Ek  */
#pragma once
#include <memory>
#include <string>
#include <vector>

#include <game_index.hpp>
#include <system/platform.hpp>

namespace indexer {

struct DrawOptions {
    u32 perGame = 10;       // at most this many positions from a game
    u32 minPly = 0;         // skip the positions before this ply: the first minPly plies
    bool quiet = false;     // only naturally quiet positions
    u64 seed = 1;           // the sample is a function of it, the input & these options
};

struct DrawStats {
    u64 games = 0;              // games given to draw()
    u64 drawn = 0;              // of them with a result, that read, and so looked at
    u64 candidates = 0;         // positions looked at: ply minPly or later, with a legal move
    u64 quiet = 0;              // of them naturally quiet
    u64 written = 0;
    u64 duplicates = 0;         // quiet positions left out because an earlier one had their hash
    std::vector<u32> quietPerGame;  // for each game looked at, how many quiet positions it had
};

class Drawer {
public:
    explicit Drawer(DrawOptions options);
    ~Drawer();

    // Appends the game's lines to out. entry comes from index() with positions and moves on.
    void draw(const GameIndex& entry, std::string& out);

    const DrawStats& stats() const { return m_stats; }

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    DrawStats m_stats;
};

// white's result as the tuner reads it: "[1.0]", "[0.5]" or "[0.0]"; empty for a game without one.
std::string resultLabel(const std::string& result);

} // namespace indexer
