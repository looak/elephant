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
#include <optional>
#include <string>
#include <vector>

#include <game_index.hpp>
#include <system/platform.hpp>

namespace indexer {

// A position is too early, when a game's ply from the initial position isn't known, while its game phase (PeSTO's:
// knights & bishops 1, rooks 2, queens 4, 24 with everything on the board) is above this: nothing has been traded
// yet, or a piece for a pawn. On fastchess's games from the opening book, whose ply is known, 93% of the positions
// from ply 16 pass it and 12% of those before.
inline constexpr u32 c_earlyPhase = 22;

struct DrawOptions {
    u32 perGame = 10;       // at most this many positions from a game
    u32 minPly = 0;         // skip the positions before this ply, counted from the initial position, see startPly()
    bool quiet = false;     // only naturally quiet positions
    u64 seed = 1;           // the sample is a function of it, the input & these options
};

struct DrawStats {
    u64 games = 0;              // games given to draw()
    u64 drawn = 0;              // of them with a result, that read, and so looked at
    u64 candidates = 0;         // positions looked at: ply minPly or later, with a legal move
    u64 unknownStart = 0;       // games looked at whose ply at the start isn't known, the game phase decided
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

// The ply, counted from the initial position, of the position a game starts from, given as a FEN with counters: 0
// for the initial position, 2 * (fullmove - 1) plus 1 when black is to move for a FEN with a real move number, 1 for
// the position after one white move. Empty when the information isn't there: a FEN that isn't the initial position
// but whose move number is 1 (a book's position with the counters reset).
std::optional<u32> startPly(const std::string& fen);

// white's result as the tuner reads it: "[1.0]", "[0.5]" or "[0.0]"; empty for a game without one.
std::string resultLabel(const std::string& result);

} // namespace indexer
