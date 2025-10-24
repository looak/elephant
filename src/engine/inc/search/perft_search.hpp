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
#include <defines.hpp>
#include <material/chess_piece_defines.hpp>
#include <move/move.hpp>

#include <vector>

class GameContext;
struct PackedMove;

struct PerftResult {
    u64 Nodes = 0;
    u64 NPS = 0;
    u64 Captures = 0;
    u64 EnPassants = 0;
    u64 Castles = 0;
    u64 Promotions = 0;
    u64 Checks = 0;
    u64 Checkmates = 0;
    u64 Depth = 0;
    bool Passed = false;

    void operator+=(const PerftResult& rhs)
    {
        this->Nodes += rhs.Nodes;
        this->Captures += rhs.Captures;
        this->EnPassants += rhs.EnPassants;
        this->Castles += rhs.Castles;
        this->Promotions += rhs.Promotions;
        this->Checks += rhs.Checks;
        this->Checkmates += rhs.Checkmates;
        this->Depth += rhs.Depth;
    }
};

struct DivideResult {
    PackedMove Move;
    u64 Nodes = 0;
};

class PerftSearch {
public:
    PerftSearch(GameContext& context);

    /**
     * @brief Runs the perft search to the specified depth.
     * @param depth The depth to search to.
     * @return The result of the perft search.     */
    template<Set us>
    PerftResult Run(int depth);

    /**
     * @brief Deepens the search by one iteration.
     * @return The result of the perft search.     */
    PerftResult Deepen();

    /**
     * @brief Divides the perft search into a node search per move at origin.
     * @param atDepth The depth to divide the search at.
     * @return A vector of results from the divided searches.    */
    template<Set us>
    std::vector<DivideResult> Divide(int atDepth);
    /**
     * @brief Divides the perft search into a node search per move at origin.
     * @param toPlay The set to play.
     * @param atDepth The depth to divide the search at.
     * @return A vector of results from the divided searches.    */
    std::vector<DivideResult> Divide(Set toPlay, int atDepth);
    

private:
    void count(PackedMove move, PerftResult& result);

    template<Set us>
    u64 internalDivide(int depth);

    GameContext& m_context;
    int m_depth;
};
