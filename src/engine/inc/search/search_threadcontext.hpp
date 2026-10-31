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
#include <array>
#include <vector>

#include <core/chessboard.hpp>
#include <search/search.hpp>
#include <search/search_constants.hpp>
#include <search/search_heuristic_structures.hpp>
#include <system/platform.hpp>
#include <system/clock.hpp>
#include <system/time_manager.hpp>
#include <eval/evaluator.hpp>
#include <eval/evaluator_data.hpp>
#include <eval/pawn_hash.hpp>
#include <eval/pesto_accumulator.hpp>
#include <move/move_executor.hpp>

struct ThreadSearchContext {
    ThreadSearchContext(Position _position, bool whiteToMove, ThreadHeuristics& _heuristics, const TimeManager& _clock)
        : position(_position),
          accumulator(PestoAccumulator::computeFromScratch(position.read().material())),
          heuristics(_heuristics),
          clock(_clock) {
            gameState.whiteToMove = whiteToMove;

        }
    Position position;
    // PeSTO sums for position, kept in step by the executors from executor() & read by evaluate().
    PestoAccumulator accumulator;
    // this thread's pawn structure scores, see pawn_hash.hpp.
    PawnHashTable pawnHash;
    GameState gameState;
    MoveHistory history;
    ThreadHeuristics& heuristics;

    // nullMoveAt[ply] is set while the reply to a null move made at ply is searched, so the reply doesn't null move
    // straight back.
    std::array<bool, c_maxSearchDepth> nullMoveAt{};
    // halfmoveClock[ply] is the fifty move rule counter of the position at ply, the root (ply 1) gets the game's.
    std::array<u16, c_maxSearchDepth + 1> halfmoveClock{};
    // While a null move fail high is verified, nmpColor doesn't null move before nmpMinPly.
    u16 nmpMinPly = 0;
    bool nmpColorWhite = false;
    u64 nodeCount = 0;
    u64 qNodeCount = 0;
    u64 evalCount = 0;
    const TimeManager& clock;

    // Set once the clock says stop. From then on every search function unwinds immediately and its return value is
    // meaningless, callers must check stopped before trusting a score, updating alpha, pv, killers or the TT.
    bool stopped = false;
    // Iterative deepening disables stopping for the first iteration so there is always one complete result.
    bool stopEnabled = true;

    u32 stopCheckCounter = 0;

    // go nodes, 0 = no limit. Checked against the nodes of all threads, with one thread the search is deterministic.
    u64 nodeLimit = 0;

    // go searchmoves, the only moves searched at the root. Empty = all moves.
    std::vector<PackedMove> rootMoves;

    // Lazy SMP, every thread searches the same root and they share the TT. Thread 0 is the main thread, it reports,
    // manages time and its result is the one played. The helpers only fill the TT.
    u16 threadId = 0;
    SharedSearchState* shared = nullptr;
    // this thread's nodes already added to shared->nodes.
    u64 publishedNodes = 0;

    [[nodiscard]] bool isMainThread() const { return threadId == 0; }

    // adds the nodes searched since the last call to the shared total, every 1024 stop checks & when the thread ends.
    void publishNodes() {
        const u64 own = nodeCount + qNodeCount;
        if (shared != nullptr)
            shared->nodes.fetch_add(own - publishedNodes, std::memory_order_relaxed);
        publishedNodes = own;
    }

    // nodes of all threads, exact for this one and up to 1024 stop checks behind for the others.
    [[nodiscard]] u64 totalNodes() const {
        const u64 own = nodeCount + qNodeCount;
        if (shared == nullptr)
            return own;
        return shared->nodes.load(std::memory_order_relaxed) + own - publishedNodes;
    }

    [[nodiscard]] bool nodeLimitReached() const {
        return nodeLimit != 0 && totalNodes() >= nodeLimit;
    }

    [[nodiscard]] bool searchStopped() const {
        return shared != nullptr && shared->stop.load(std::memory_order_relaxed);
    }

    // A move parsed from UCI notation carries no capture or special flags, compare what the notation holds.
    [[nodiscard]] static bool sameUciMove(PackedMove lhs, PackedMove rhs) {
        return lhs.sourceSqr() == rhs.sourceSqr() && lhs.targetSqr() == rhs.targetSqr()
            && lhs.isPromotion() == rhs.isPromotion()
            && (!lhs.isPromotion() || lhs.readPromoteToPieceType() == rhs.readPromoteToPieceType());
    }

    [[nodiscard]] bool isRootMoveAllowed(PackedMove move) const {
        for (PackedMove allowed : rootMoves) {
            if (sameUciMove(allowed, move))
                return true;
        }
        return false;
    }

    // Throttled, the clock and the main thread's stop are only consulted every 1024 calls. The node limit is checked on
    // every call so go nodes stops close to exact, the branch is cheap & predictable when no limit is set.
    bool shouldStop() {
        if (stopped)
            return true;
        if (!stopEnabled)
            return false;
        if (nodeLimitReached())
            stopped = true;
        else if ((++stopCheckCounter & 1023) == 0) {
            publishNodes();
            if (clock.shouldStop() || searchStopped())
                stopped = true;
        }
        return stopped;
    }

    // Static eval from the side-to-move's perspective, with its tempo bonus.
    template<Set us>
    i16 evaluate() {
        ++evalCount;
        constexpr i16 perspective = (us == Set::WHITE) ? 1 : -1;
        Evaluator evaluator(position.read(), accumulator, &pawnHash);
        return static_cast<i16>(evaluator.Evaluate() * perspective + evaluator_data::tempoBonus);
    }

    // Makes & unmakes moves on position, keeping the accumulator in step.
    MoveExecutor executor() { return MoveExecutor(position.edit(), &accumulator); }

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
    
    void scout_search() {
        auto logger = logging::debug_search_logger();
        if (logger) {
            logger->debug("[TID:{:X}] Scout Search triggered.",
                (u64)std::hash<std::thread::id>{}(std::this_thread::get_id()));
        }
    }

    void scout_re_search() {
        auto logger = logging::debug_search_logger();
        if (logger) {
            logger->debug("[TID:{:X}] Scout Re-Search triggered.",
                (u64)std::hash<std::thread::id>{}(std::this_thread::get_id()));
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
    void scout_search() {}
    void scout_re_search() {}
    void begin(int, const SearchParameters&) {}
    void end(int) {}
#endif


};