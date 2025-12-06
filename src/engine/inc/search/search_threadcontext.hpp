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

#pragma once
#include <core/chessboard.hpp>
#include <search/search_heuristic_structures.hpp>
#include <system/platform.hpp>
#include <system/clock.hpp>

struct ThreadSearchContext {
    ThreadSearchContext(Position _position, bool whiteToMove, const TimeManager& _clock)
        : position(_position), clock(_clock) {
            gameState.whiteToMove = whiteToMove;

        }
    Position position;
    GameState gameState;
    MoveHistory history;
    MoveOrderingHeuristic moveOrdering;
    u64 nodeCount = 0;
    u64 qNodeCount = 0;
    const TimeManager& clock;

#ifdef DEBUG_SEARCH_TREE
    int m_debugIndentation = 0;
    Clock m_debugTimer;
    // function for logging the start of an alpha-beta search:    
    void debug_print_alphabeta_entry(u8 depth, u16 ply, i16 alpha, i16 beta, u64 hash) {
        auto logger = logging::debug_search_logger();
        if (logger) {
            // Create a string of spaces for indentation
            // 2 spaces per level is usually enough            
            m_debugTimer.Start();
            std::string pad(std::max(0, m_debugIndentation * 2), ' ');

            // Use TID (Thread ID) for multithreaded debugging
            logger->debug("[TID:{:X}] {} >> AB | P:{} | D:{} | α:{:05} | β:{:05} | Hash:{:016X}",
                (u64)std::hash<std::thread::id>{}(std::this_thread::get_id()), // Hex Thread ID
                pad, ply, (int)depth, alpha, beta, hash);

            m_debugIndentation++;
        }
    }

    // function for logging the result of a search:
    void debug_print_eval(PackedMove move, i16 eval, i16 alpha, i16 beta, u8 depth, u16 ply, u64 hash) {
        auto logger = logging::debug_search_logger();
        if (logger) {
            // Create a string of spaces for indentation
            m_debugIndentation--;            
            // 2 spaces per level is usually enough
            std::string pad(std::max(0, m_debugIndentation * 2), ' ');

            logger->debug("[TID:{:X}] {} << AB | P:{} | D:{} | α:{:05} | β:{:05} | Hash:{:016X} | Eval:{:05} | Move:{}",
                (u64)std::hash<std::thread::id>{}(std::this_thread::get_id()),
                pad, ply, (int)depth, alpha, beta, hash, eval, move.toString());
            
        }
    }

    void tt_probe_score(i16 score, u8 depth, u16 ply, u64 hash) const {
        auto logger = logging::debug_search_logger();
        if (logger) {
            logger->debug("[TID:{:X}] TT | P:{} | D:{} | Hash:{:016X} | Score:{:05}",
                (u64)std::hash<std::thread::id>{}(std::this_thread::get_id()),
                ply, (int)depth, hash, score);
        }
    }

    void begin(int threadId, const SearchParameters& params) {
        auto logger = logging::debug_search_logger();
        if (logger) {
        logger->debug("[TID:{:X}] Starting search with {} threads, time control: {}",
            params.ThreadCount, params.ThreadCount, params.MoveTime);
        }
    }

    void end(int threadId) {
        auto logger = logging::debug_search_logger();
        if (logger) {
            logger->debug("[TID:{:X}]Ending search thread.", threadId);
        }
    }
#else
    void debug_print_alphabeta_entry(u8, u16, i16, i16, u64) const {}
    void debug_print_eval(PackedMove, i16, i16, i16, u8, u16, u64) const {}
    void tt_probe_score(i16, u8, u16, u64) const {}
    void begin(int, const SearchParameters&) {}
    void end(int) {}
#endif


};