// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021-2023  Alexander Loodin Ek

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
#include <span>
#include <string_view>
#include <vector>

#include <core/chessboard.hpp>
#include <move/move_undo.hpp>

// A game: where it stands and the moves played to get there. Nothing clears its past, a game starts at a position and
// another game replaces it, so the moves always lead to the board. What a search needs of it is copied into a
// SearchRequest, so a Game can be replaced while a search runs.
class Game {
public:
    // At the start position.
    Game();
    // At start, whatever it came from, with no moves played.
    explicit Game(const Chessboard& start) : m_board(start) {}

    // Replaces this game with one starting at fen. When fen doesn't read, returns false and the game stays as it was.
    bool loadFen(std::string_view fen);

    template<bool validation = false>
    void makeMove(PackedMove move);
    bool unmakeMove();

    const Chessboard& board() const { return m_board; }
    // the moves played, oldest first. Each holds the hash of the position it was played from.
    std::span<const MoveUndoUnit> moves() const { return m_moves; }

    PositionReader readPosition() const { return m_board.readPosition(); }
    u16 readHalfmoveClock() const { return m_board.readHalfmoveClock(); }
    u16 readMoveCount() const { return m_board.readMoveCount(); }
    Set readToPlay() const { return m_board.readToPlay(); }

private:
    Chessboard m_board;
    std::vector<MoveUndoUnit> m_moves;
};
