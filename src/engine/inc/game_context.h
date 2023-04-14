// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021  Alexander Loodin Ek

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

    const Chessboard& readChessboard() const { return m_board; }
    Chessboard& editChessboard() { return m_board; }
    Chessboard copyChessboard() const { return m_board; }

    u32 readPly() const { return m_plyCount; }
    u32& editPly() { return m_plyCount; }

    u32 readMoveCount() const { return m_moveCount; }
    u32& editMoveCount() { return m_moveCount; }

    Set readToPlay() const { return m_toPlay; }
    Set& editToPlay() { return m_toPlay; }

    bool MakeMove(Move& move);
    bool UnmakeMove(const Move& move);

    void PlayMoves(const Move& move, bool print = false);

    bool endOfGame() const;
    
private:
    Chessboard m_board;
    Set m_toPlay;
    u32 m_plyCount;    
    u32 m_moveCount;
    u32 m_fiftyMoveRule;
    
    
};