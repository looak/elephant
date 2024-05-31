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
#include <stack>
#include "chessboard.h"
#include "transposition_table.hpp"

struct SearchResult;
struct SearchParameters;

struct MoveHistory {
    // Move move;
    // Chessboard board;
    u64 HashKey;
    u32 PlyCount;
    u32 MoveCount;
    u32 FiftyMoveRule;
    // Short Algebraic Notation
    std::string SAN;
};

class GameContext {
public:
    GameContext() {
        m_transpositionTable.clear();
        Reset();        
    }

    GameContext(const GameContext& rhs) :
        m_board(rhs.m_board)
    {
    }

    void Reset();
    void NewGame();

    /**
     * @brief Tries to make a move on board, returns true if legal succesfull move.
     * @param move The move which is being asked to make, can be ambigious. */
    bool TryMakeMove(Move move);

    /**
     * @brief Makes a move on the board, assumes move is legal. */
    bool MakeMove(const PackedMove move);
    bool UnmakeMove();

    SearchResult CalculateBestMove(SearchParameters params);

    bool GameOver() const;

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

private:
    Chessboard m_board;
    TranspositionTable m_transpositionTable;

    std::stack<MoveUndoUnit> m_undoUnits;
    std::vector<MoveHistory> m_moveHistory;
};