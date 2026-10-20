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
 * @file game_index.hpp
 * @brief A game's index entry: what phant-indexer reports about a game after replaying it, and the JSON line it
 * writes for it. The entry's fields and their JSON are a contract with nyayo, versioned by c_formatVersion; a field
 * added or changed means a new version. ramani's plans/game-database.md has the reasons.
 * @author Alexander Loodin Ek  */
#pragma once
#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <io/pgn_parser.hpp>
#include <system/platform.hpp>

namespace indexer {

// the version of the entry's fields and their JSON.
inline constexpr u32 c_formatVersion = 1;

// the plies after which the material balance is taken, besides at the end.
inline constexpr std::array<u32, 2> c_materialPlies = { 20, 40 };

// how the final position ended the game by the rules, if it did. Anything else (a resignation, an adjudication, a
// loss on time, a crash) leaves a position that didn't: None.
enum class Ending : u8 { None, Checkmate, Stalemate, InsufficientMaterial, FiftyMoves, Repetition };
std::string_view toString(Ending ending);

struct GameIndex {
    std::string reader;             // the version of Elephant phant-indexer was built from
    std::string source;             // the file the game came from, empty for standard input
    u32 number = 0;                 // the game's place in its input, from 1
    size_t offset = 0;              // where it starts in its input, in bytes
    size_t length = 0;              // its length in bytes
    std::vector<io::pgn_parser::Tag> tags;  // as python-chess keeps them: a name once, at its first place, its last value

    bool ok = true;                 // every move read and was legal
    std::string error;              // why not, when not ok
    u32 errorPly = 0;               // the ply that didn't read, from 1; 0 for an error before the moves

    std::string result;             // the Result tag, else the movetext's first result, else "*"
    u32 plies = 0;                  // moves played, null moves included, up to an error
    Ending ending = Ending::None;
    std::array<u32, 4> promotions = {};         // to queen, rook, bishop, knight
    std::optional<u32> castledWhite;            // the ply of the castling move
    std::optional<u32> castledBlack;
    u32 captures = 0;               // en passant captures included
    u32 epCaptures = 0;
    u32 checks = 0;                 // moves that give check
    u32 minPieces = 0;              // the fewest pieces on the board, kings included, the start position's counted
    // white's material less black's, pawn 1, knight & bishop 3, rook 5, queen 9; after the plies in c_materialPlies
    // the game reached, and at the end. The key is the ply, or "end".
    std::vector<std::pair<std::string, i32>> material;

    // with verbose on: the moves in UCI notation and the final position as FEN, what the smoke test compares.
    std::vector<std::string> moves;
    std::string fen;
};

// Replays a game, from its FEN tag when it has one, and reports what it finds. A Variant tag that isn't standard
// chess is an error before the moves.
GameIndex index(const io::pgn_parser::Game& game, u32 number, bool verbose = false);

// The entry as one line of JSON, without its newline. The moves and the FEN are written when verbose.
std::string toJson(const GameIndex& entry, bool verbose = false);

} // namespace indexer
