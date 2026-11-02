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
#include <vector>
#include <core/chessboard.hpp>
#include <move/move_undo.hpp>

struct SearchResult;
struct SearchLimits;

struct GameHistory {
    GameHistory() = default;    

    // needs to be a vector to allow validation of move repetition.
    std::vector<MoveUndoUnit> moveUndoUnits;


    private:
    GameHistory(const GameHistory& rhs) = delete;
    GameHistory& operator=(const GameHistory& rhs) = delete;
};

class Game {
public:
    Game()
    {
        Reset();
    }

    void Reset();
    void NewGame();
        
    template<bool validation = false>
    void makeMove(const PackedMove move);
    bool unmakeMove();

    const Chessboard& readChessboard() const { return m_board; }
    Chessboard& editChessboard() { return m_board; }
    Chessboard copyChessboard() const { return m_board; }

    PositionReader readPosition() const { return m_board.readPosition(); }
    PositionEditor editChessPosition() { return m_board.editPosition(); }

    u16 readHalfmoveClock() const { return m_board.readHalfmoveClock(); }
    u16 readMoveCount() const { return m_board.readMoveCount(); }

    Set readToPlay() const { return m_board.readToPlay(); }

    GameHistory& editGameHistory() { return m_history; }
    const GameHistory& readGameHistory() const { return m_history; }

private:
    Game(const Game& rhs) = delete;
    Game& operator=(const Game& rhs) = delete;

    Chessboard m_board;
    GameHistory m_history;
};