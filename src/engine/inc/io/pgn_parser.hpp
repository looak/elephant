// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021-2025  Alexander Loodin Ek

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
 * @brief Portable Game Notation - a standard format for recording chess games
 * https://www.wikiwand.com/en/articles/Portable_Game_Notation
 * Example:
 *   [Event "F/S Return Match"]
 *   [Site "Belgrade, Serbia JUG"]
 *   [Date "1992.11.04"]
 *   [Round "29"]
 *   [White "Fischer, Robert J."]
 *   [Black "Spassky, Boris V."]
 *   [Result "1/2-1/2"]
 *
 *   1.e4 e5 2.Nf3 Nc6 3.Bb5 {This opening is called the Ruy Lopez.} 3...a6
 *   4.Ba4 Nf6 5.O-O Be7 6.Re1 b5 7.Bb3 d6 8.c3 O-O 9.h3 Nb8 10.d4 Nbd7
 *   11.c4 c6 12.cxb5 axb5 13.Nc3 Bb7 14.Bg5 b4 15.Nb1 h6 16.Bh4 c5 17.dxe5
 *   Nxe4 18.Bxe7 Qxe7 19.exd6 Qf6 20.Nbd2 Nxd6 21.Nc4 Nxc4 22.Bxc4 Nb6
 *   23.Ne5 Rae8 24.Bxf7+ Rxf7 25.Nxf7 Rxe1+ 26.Qxe1 Kxf7 27.Qe3 Qg5 28.Qxg5
 *   hxg5 29.b3 Ke6 30.a3 Kd6 31.axb4 cxb4 32.Ra5 Nd5 33.f3 Bc8 34.Kf2 Bf5
 *   35.Ra7 g6 36.Ra6+ Kc5 37.Ke1 Nf4 38.g3 Nxh3 39.Kd2 Kb5 40.Rd6 Kc5 41.Ra6
 *   Nf2 42.g4 Bd3 43.Re6 1/2-1/2
 * 
 * @file pgn_parser.hpp
 * 
 * @author Alexander Loodin Ek  */
#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <system/platform.hpp>

class GameContext;

namespace io {
namespace pgn_parser {
    // A token of movetext: a move in SAN, a null move (-- Z0 0000 @@@@), a comment's text, or a result. Views into the
    // text it came from.
    struct Token {
        enum class Kind : u8 { Move, NullMove, Comment, Result };
        Kind kind;
        std::string_view text;
    };

    // Reads movetext a token at a time, as python-chess does. Move numbers (12. 12...), NAGs ($1), move suffixes (!?),
    // variations and escape lines (%) are skipped; {} and ; comments are tokens. A result doesn't end it, the moves
    // after one are read too; it ends at a blank line outside a comment, a variation's included, or before a '[',
    // which starts the next game's tags.
    class MovetextTokenizer {
    public:
        explicit MovetextTokenizer(std::string_view text) : m_text(text) {}

        std::optional<Token> next();
        // how far into the text it has read.
        size_t position() const { return m_pos; }

    private:
        void skipVariation();

        std::string_view m_text;
        size_t m_pos = 0;
        bool m_done = false;
    };

    struct Tag {
        std::string name;
        std::string value;
    };

    // A game as it stands in a PGN stream: its tags and its movetext, not yet played.
    struct Game {
        size_t offset = 0;              // where the game starts in the stream, in bytes
        size_t length = 0;              // its length in bytes, through its movetext
        std::vector<Tag> tags;
        std::string_view movetext;      // a view into the stream

        // the tag's value, or nullptr.
        const std::string* tag(std::string_view name) const;
    };

    // Splits a PGN stream into games, one at a time, where python-chess (chess.pgn.read_game) splits it. The stream has
    // to outlive the games read from it. Tags are read as python-chess reads them: values as written, \" not
    // unescaped, a line that isn't a tag skipped, % and ; lines skipped, one blank line allowed among the tags and
    // before the movetext; a second one ends the game there, without moves.
    class GameReader {
    public:
        explicit GameReader(std::string_view stream) : m_stream(stream) {}

        std::optional<Game> next();

    private:
        bool readTag(Game& game);
        void skipWhitespace();

        std::string_view m_stream;
        size_t m_pos = 0;
    };

    // Whether a Variant tag's value is the rules the engine plays, standard chess: python-chess's names for it, in
    // any case. Chess960 and the rest aren't.
    bool standardVariant(std::string_view variant);

    // Plays a game: a whole game with its tags, or movetext alone. A FEN tag sets the start position, with or without
    // SetUp; without one the moves are played on the game's current position. Throws io_error on a Variant that isn't
    // standard chess, a FEN that doesn't read, a null move (a GameContext can't take one back), and a move that
    // doesn't read or isn't legal.
    void deserialize(GameContext& game, std::string_view pgn);

} // namespace pgn_parser
} // namespace io
