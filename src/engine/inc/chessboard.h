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

/**
 * @file chessboard.h
 * @brief Represents the chessboard and its current state.
 * 
 * @author Alexander Loodin Ek
 */

#pragma once
#include <defines.hpp>
#include <position/position.hpp>

/**
 * Represents the state important for some rules of a game such as half move count. 
 */
 struct GameState {
    GameState() = default;
    GameState(const GameState& rhs) :
        plyCount(rhs.plyCount),
        moveCount(rhs.moveCount),
        whiteToMove(rhs.whiteToMove)
    {}

    u16 plyCount = 0;  // number of half moves made
    u16 moveCount = 0;  // number of full moves made
    bool whiteToMove = true;  // true if it's white's turn to move
 };

/**
 * The Chessboard class represents a chess board and its current state.  */
class Chessboard {
public:
    Chessboard();
    ~Chessboard() = default;
    Chessboard(const Chessboard& other);
    
    PositionReader readPosition() const { return m_position.read(); }
    PositionEditor editPosition() { return m_position.edit(); }
    GameState& editState() { return m_gameState; }

    u16 readMoveCount() const { return m_gameState.moveCount; }
    u16 readPlyCount() const { return m_gameState.plyCount; }
    Set readToPlay() const { return m_gameState.whiteToMove ? Set::WHITE : Set::BLACK; }    

    std::string toString() const;

private:
    GameState m_gameState;
    Position m_position;    
};