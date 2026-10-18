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
#include <system/platform.hpp>
#include <eval/pesto_accumulator.hpp>
#include <position/position_accessors.hpp>

#include <unordered_map>

class Chessboard;
struct PackedMove;
class Position;
class PawnHashTable;

class Evaluator
{
public:
    // accumulator must match position, search keeps one in step through MoveExecutor. pawnHash caches pawn structure
    // scores, search passes its thread's table; without one they're counted every time.
    Evaluator(PositionReader position, const PestoAccumulator& accumulator, PawnHashTable* pawnHash = nullptr) :
        m_position(position), m_accumulator(accumulator), m_pawnHash(pawnHash) {}
    // computes the accumulator from scratch, for one off evaluations outside search.
    Evaluator(PositionReader position) :
        m_position(position), m_accumulator(PestoAccumulator::computeFromScratch(position.material())) {}

    i16 Evaluate();
    i16 EvaluatePlus(PackedMove move);

    /**
     * @brief The tapered terms of the evaluation, white's side: PeSTO's material & piece square tables and mobility,
     * summed as one midgame & endgame TaperedScore and tapered once by the game phase. Evaluate adds mop-up to it.
     * The tuner fits exactly these terms.    */
    i32 EvaluateTapered() const;

    /**
     * @brief Calculates the end game coeficient.
     * Used for tapered evaluation.
     * @return a float between 0 and 1 where 1 is endgame and 0 is midgame.     */    
    float calculateEndGameCoeficient() const;

    /**
     * @brief PeSTO game phase, knights & bishops add 1, rooks 2 and queens 4.
     * @return 24 with all pieces on the board (capped there after promotions) down to 0 with only kings & pawns. */
    i32 gamePhase() const;

private:
    i16 EvaluateMaterial() const;
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
    PestoAccumulator m_accumulator;
    PawnHashTable* m_pawnHash = nullptr;

};