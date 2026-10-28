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
#include <move/move.hpp>
#include <system/platform.hpp>

class Chessboard;

namespace indexer {

// the version of the entry's fields and their JSON.
inline constexpr u32 c_formatVersion = 2;

// the ply after which the opening's position is hashed.
inline constexpr u32 c_openingPly = 16;

// the version of the --positions records, which have no header to carry it: 1 was 12 bytes (hash, game, ply), 2 adds
// the move played from the position, 14 bytes.
inline constexpr u32 c_positionsVersion = 2;
inline constexpr size_t c_positionRecordSize = 14;

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
    u64 start = 0;                  // the start position's Polyglot hash, see polyglot.hpp
    std::optional<u64> opening;     // the position's after c_openingPly plies, when the game is that long
    // white's material less black's, pawn 1, knight & bishop 3, rook 5, queen 9; after the plies in c_materialPlies
    // the game reached, and at the end. The key is the ply, or "end".
    std::vector<std::pair<std::string, i32>> material;

    // with verbose on: the moves in UCI notation and the final position as FEN, what the smoke test compares.
    std::vector<std::string> moves;
    std::string fen;

    // with positions on: every position's Polyglot hash, the start position's first, then the one after each move, up
    // to an error.
    std::vector<u64> positions;

    // with moves on: the position the game started from as a FEN, and the moves played from it, a null move as
    // PackedMove::NullMove(), up to an error. What --draw replays to reach the positions it looks at.
    std::string startFen;
    std::vector<PackedMove> played;
};

// Appends a --positions record: the hash, the game's place in its input, the ply and the move played from the position
// as the engine packs it (source square in bits 0-5, target in 6-11, the move type in 12-15), 0 when there is none,
// at a game's last position, and for a null move too; all little-endian.
void appendPositionRecord(std::string& out, u64 hash, u32 game, u32 ply, PackedMove move);

// whether the side to move is in check and has a legal move; no legal move is checkmate or stalemate.
struct PositionFacts {
    bool inCheck = false;
    bool canMove = false;
};
PositionFacts positionFacts(const Chessboard& board);

// Replays a game, from its FEN tag when it has one, and reports what it finds. A Variant tag that isn't standard
// chess is an error before the moves.
// With positions on, every position's Polyglot hash is kept too, the start position's first, then the one after each
// move: what --positions writes and --position scans.
// With moves on, the start position and the moves are kept too.
GameIndex index(const io::pgn_parser::Game& game, u32 number, bool verbose = false, bool positions = false,
    bool moves = false);

// The entry as one line of JSON, without its newline. The moves and the FEN are written when verbose.
std::string toJson(const GameIndex& entry, bool verbose = false);

// A JSON string, quotes and escapes, as the entries write their text.
std::string toJsonString(std::string_view text);

} // namespace indexer
