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
 * @file perft_search.hpp
 * @brief Defines structures and utilities for running Performance Tests (Perft).
 *
 * This file contains the necessary components for verifying the correctness and
 * measuring the performance of the move generator. It provides a standardized
 * way to define test cases from FEN strings and execute them to a specified
 * depth, comparing the resulting node count against a known correct value.
 * https://www.chessprogramming.org/Perft    */
#pragma once
#include <system/platform.hpp>

#include <core/game_context.hpp>
#include <material/chess_piece_defines.hpp>
#include <move/move.hpp>
#include <move/generation/move_gen_policy.hpp>
#include <move/move_executor.hpp>


#include <vector>

class GameContext;
struct PackedMove;

struct PerftResult {
    u64 Nodes = 0;

    void operator+=(const PerftResult& rhs) { Nodes += rhs.Nodes; }
};

struct DivideResult {
    PackedMove Move;
    u64 Nodes = 0;
};

/**
 * @brief Perft, templated on a move generator policy (see move_gen_policy.hpp) so generators can be A/B tested:
 *     perft.Run(depth);                           // tusk generator
 *     perft.Run<move_gen_policy::Tusk>(depth);    // the same, a new generator's policy goes here to A/B it
 * Run & Divide are explicitly instantiated for move_gen_policy::Tusk.   */
class PerftSearch {
public:
    PerftSearch(GameContext& context);

    /**
     * @brief Runs the perft search to the specified depth.
     * @param depth The depth to search to.
     * @return The result of the perft search.     */    
    template<typename TMoveGen = move_gen_policy::Tusk>
    PerftResult Run(int depth);

    /**
     * @brief Deepens the search by one iteration.
     * @return The result of the perft search.     */
    PerftResult Deepen();
    
    /**
     * @brief Divides the perft search into a node search per move at origin.
     * @param atDepth The depth to divide the search at.
     * @return A vector of results from the divided searches.    */
    template<typename TMoveGen = move_gen_policy::Tusk>
    std::vector<DivideResult> Divide(int atDepth);
    

private:
    template<Set us, typename TMoveGen>
    u64 internalRun(int depth);

    template<typename TMoveGen>
    u64 internalRunEntryPoint(int depth);

    GameContext& m_context;
};

// counts in bulk at depth 1, the last ply's moves are counted by the generator and never made.
template<Set us, typename TMoveGen>
u64 PerftSearch::internalRun(int depth)
{
    if (depth <= 0)
        return 0;

    const PositionReader position = m_context.readChessboard().readPosition();
    if (depth == 1)
        return TMoveGen::template countMoves<us>(position);

    u64 nodes = 0;
    TMoveGen::template forEachMove<us>(position, [&](PackedMove move) {
        m_context.MakeMove(move);
        nodes += internalRun<opposing_set<us>(), TMoveGen>(depth - 1);
        m_context.UnmakeMove();
    });

    return nodes;
}

template<typename TMoveGen>
u64 PerftSearch::internalRunEntryPoint(int depth)
{
    if (m_context.readToPlay() == Set::WHITE)
        return internalRun<Set::WHITE, TMoveGen>(depth);
    return internalRun<Set::BLACK, TMoveGen>(depth);
}
