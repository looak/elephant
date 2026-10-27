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
 * positions after its first plies, naturally quiet ones, a sample of them, none twice in the run or each once with its
 * results averaged, as `<fen> [result]` lines, the format ElephantTuner's dataset reader takes. Every position written
 * is one the game reached.
 * @author Alexander Loodin Ek  */
#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <game_index.hpp>
#include <system/platform.hpp>

namespace indexer {

// A game can carry its own minimum in a tag, [DrawMinPly "4"], which overrides the minimum of its file and the global
// one for that game: how a caller feeding standard input says which minimum a batch's games take.
inline constexpr const char* c_minPlyTag = "DrawMinPly";

// the version of the --stats JSON.
inline constexpr u32 c_statsVersion = 1;

struct DrawOptions {
    u32 perGame = 10;       // at most this many positions from a game
    u32 minPly = 0;         // skip a game's first minPly plies, counted from the position it starts at; a file's or a
                            // tag's minimum replaces it
    bool quiet = false;     // only naturally quiet positions
    bool merge = false;     // each position once, its result the average over the games that drew it: see finish()
    u64 seed = 1;           // the sample is a function of it, the input & these options
};

struct DrawStats {
    u64 games = 0;              // games given to draw()
    u64 unread = 0;             // of them skipped because a move didn't read or wasn't legal, or the FEN didn't
    u64 noResult = 0;           // skipped because they have no result (1-0, 0-1 or 1/2-1/2)
    u64 nullMoves = 0;          // skipped because a null move was played, which was never played over a board
    u64 drawn = 0;              // the rest, looked at
    u64 candidates = 0;         // positions looked at: past the game's minimum, with a legal move, not in check
    u64 quiet = 0;              // of them naturally quiet; with quiet off, the same as candidates
    u64 written = 0;            // lines written; with merge, once finish() has
    u64 duplicates = 0;         // quiet positions left out because an earlier one had their hash
    u64 draws = 0;              // with merge: positions drawn from games, a position several games drew counted for each
    u64 merged = 0;             // with merge: of the positions written, those more than one game drew
    u64 whiteWins = 0, drawResults = 0, blackWins = 0, mixed = 0;   // the results of the lines written; mixed is an average
    u64 ownMinimum = 0;         // games whose minimum came from their file or a tag
    u64 badMinimumTags = 0;     // games whose DrawMinPly tag isn't a number, which count as if they had none
    std::vector<u32> quietPerGame;  // for each game looked at, how many quiet positions it had
};

class Drawer {
public:
    explicit Drawer(DrawOptions options);
    ~Drawer();

    // Appends the game's lines to out. entry comes from index() with positions and moves on. minPlyHere is the minimum
    // of the file the game came from, which the game's own tag still overrides; without either the options' minPly
    // counts. A minimum is a number of the game's own first plies to skip.
    void draw(const GameIndex& entry, std::string& out, std::optional<u32> minPlyHere = std::nullopt);

    // With merge, the lines are written here, once the last game has been drawn: each position once, in the order it
    // was first drawn, with the average of the results of the games that drew it, white's, [1.0] for one win,
    // [0.75] for three wins in four. Without merge it writes nothing.
    void finish(std::string& out);

    const DrawStats& stats() const { return m_stats; }

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    DrawStats m_stats;
};

// An input given as FILE:P, a file and the minimum its games take: the part before the last colon and, when what follows
// it is a number and there is a file before it, P. Anything else, a colon in a Windows path (C:\dir\x.pgn) included,
// is the whole file with no minimum of its own.
std::pair<std::string, std::optional<u32>> splitInputMinimum(const std::string& argument);

// white's result as the tuner reads it: "[1.0]", "[0.5]" or "[0.0]"; empty for a game without one.
std::string resultLabel(const std::string& result);

// What an input contributed, for the statistics.
struct DrawInput {
    std::string file;
    std::optional<u32> minPly;      // the file's own minimum
    u64 games = 0;                  // games read from it
    u64 drawn = 0;                  // of them looked at
};

// The run's statistics as one JSON object, version c_statsVersion, see indexer/README.md. reader is the Elephant version.
std::string statsJson(const DrawStats& stats, const DrawOptions& options, const std::vector<DrawInput>& inputs,
    const std::string& reader);

} // namespace indexer
