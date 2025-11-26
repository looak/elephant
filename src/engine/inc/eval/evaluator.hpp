/******************************************************************************
 * Elephant Gambit Chess Engine - a Chess AI
 * Copyright(C) 2025  Alexander Loodin Ek
 * 
 * This program is free software : you can redistribute it and /or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.If not, see < http://www.gnu.org/licenses/>. 
 *****************************************************************************/

/**
 * @file evaluator.hpp
 * @brief Implements a chess position evaluator for assessing board states and move effectiveness 
 * 
 */

#pragma once
#include "defines.hpp"
#include <position/position_accessors.hpp>

#include <unordered_map>

class Chessboard;
struct PackedMove;
class Position;

class Evaluator
{
public:
    Evaluator(PositionReader position) : m_position(position) {}

    i16 Evaluate();
    i16 EvaluatePlus(PackedMove move);

    /**
     * @brief Calculates the end game coeficient.
     * Used for tapered evaluation.
     * @return a float between 0 and 1 where 1 is endgame and 0 is midgame.     */    
    float calculateEndGameCoeficient() const;

private:
    i16 EvaluateMaterial() const;
    i16 EvaluatePiecePositions() const;
    i16 EvaluatePawnStructure() const;
    i16 EvaluatePawnManhattanDistance() const;
    i16 EvaluateKingSafety() const;

    i16 MopUpValue(i16 materialScore) const;

    template<Set us>
    i16 EvaluatePassedPawn() const;
    template<Set us>
    i16 EvaluatePawnProtection( Bitboard pawns) const;

    template<Set us>
    i16 MopUpValue(i16 materialScore) const;


    PositionProxy<PositionReadOnlyPolicy> m_position;

};