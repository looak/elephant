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
#include <search/transposition_table.hpp>

struct SearchResult;
struct SearchParameters;

struct GameHistory {
    GameHistory() = default;    

    u32 age = 0;
    // needs to be a vector to allow validation of move repetition.
    std::vector<MoveUndoUnit> moveUndoUnits;

    bool IsRepetition(u64 hashKey) const;

    private:
    GameHistory(const GameHistory& rhs) = delete;
    GameHistory& operator=(const GameHistory& rhs) = delete;
};

class GameContext {
public:
    GameContext() :
        m_transpositionTable(64)
    {
        Reset();
    }    

    void Reset();
    void NewGame();
        
    template<bool validation = false>
    void MakeMove(const PackedMove move);
    bool UnmakeMove();

    bool GameOver() const;
    

    /**
     * @brief Checks if the game is over.    */
    bool isGameOver() const;

    const Chessboard& readChessboard() const { return m_board; }
    Chessboard& editChessboard() { return m_board; }
    Chessboard copyChessboard() const { return m_board; }

    PositionReader readChessPosition() const { return m_board.readPosition(); }
    PositionEditor editChessPosition() { return m_board.editPosition(); }

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