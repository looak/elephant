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
#include "chessboard.h"


struct EngineParameters
{
    int WhiteTimelimit = 0;
    int BlackTimelimit = 0;
    int WhiteTimeIncrement = 0;
    int BlackTimeIncrement = 0;
};

struct SearchParameters
{
    SearchParameters() :
        SearchDepth(5),
        MoveTime(0)
    {}
    
    // search depth in half moves, a.k.a. ply or plies.
    int SearchDepth = 5;

    // total amount of time allowed to search for a move in milliseconds.
    int MoveTime = 0;
};

struct MoveHistory
{
    //Move move;
    //Chessboard board;
    u64 HashKey;
    u32 PlyCount;
    u32 MoveCount;
    u32 FiftyMoveRule;
    // Short Algebraic Notation
    std::string SAN;
};

class GameContext
{
public:
    GameContext() :
        m_toPlay(Set::WHITE),
        m_plyCount(0),
        m_moveCount(1)
    {}

    GameContext(const GameContext& rhs) :
        m_board(rhs.m_board),
        m_toPlay(rhs.m_toPlay),
        m_plyCount(rhs.m_plyCount),
        m_moveCount(rhs.m_moveCount)
    {}

    void Reset();
    void NewGame();

    void PlayMoves(const Move& move, bool print = false);
    bool PlayMove(Move& move);
    
    bool MakeLegalMove(Move& move);
    bool MakeMove(Move& move);
    bool UnmakeMove(const Move& move);

    Move CalculateBestMove(SearchParameters params = SearchParameters());

    bool GameOver() const;

    /**
     * @brief Checks if the game is over.    */
    bool isGameOver() const;

    const Chessboard& readChessboard() const { return m_board; }
    Chessboard& editChessboard() { return m_board; }
    Chessboard copyChessboard() const { return m_board; }

    u32 readPly() const { return m_plyCount; }
    u32& editPly() { return m_plyCount; }

    u32 readMoveCount() const { return m_moveCount; }
    u32& editMoveCount() { return m_moveCount; }

    Set readToPlay() const { return m_toPlay; }
    Set& editToPlay() { return m_toPlay; }

    const std::vector<MoveHistory>& readMoveHistory() const { return m_moveHistory; }

private:
    //std::pair<u64, Move> concurrentBestMove(int depth, Chessboard& board, Set toPlay);

    Chessboard m_board;
    Set m_toPlay;
    u32 m_plyCount;    
    u32 m_moveCount;
    u32 m_fiftyMoveRule;

    std::vector<MoveHistory> m_moveHistory;
};