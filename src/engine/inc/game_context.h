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
#include "chessboard.h"
#include "transposition_table.hpp"

struct SearchResult;
struct SearchParameters;

struct GameHistory {
    GameHistory() = default;    

    u32 age = 0;
    std::vector<MoveUndoUnit> moveUndoUnits;

    private:
    GameHistory(const GameHistory& rhs) = delete;
    GameHistory& operator=(const GameHistory& rhs) = delete;
};

class GameContext {
public:
    GameContext() {
        m_transpositionTable.resize(64);
        Reset();
    }    

    void Reset();
    void NewGame();
    
    /**
     * @brief Makes a move on the board, assumes move is legal. */
    void MakeMove(const PackedMove move);
    bool UnmakeMove();

    bool GameOver() const;
    bool IsRepetition(u64 hashKey) const;

    /**
     * @brief Checks if the game is over.    */
    bool isGameOver() const;

    const Chessboard& readChessboard() const { return m_board; }
    Chessboard& editChessboard() { return m_board; }
    Chessboard copyChessboard() const { return m_board; }

    short readPly() const { return m_board.readPlyCount(); }
    short readMoveCount() const { return m_board.readMoveCount(); }

    Set readToPlay() const { return m_board.readToPlay(); }

    TranspositionTable& editTranspositionTable() { return m_transpositionTable; }
    GameHistory& editGameHistory() { return m_history; }
    const GameHistory& readGameHistory() const { return m_history; }

private:
    GameContext(const GameContext& rhs) = delete;
    GameContext& operator=(const GameContext& rhs) = delete;

    Chessboard m_board;
    TranspositionTable m_transpositionTable;

    GameHistory m_history;
};