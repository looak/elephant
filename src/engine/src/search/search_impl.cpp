#include <search/search.hpp>

#include <core/game_context.hpp>
#include <math/cast.hpp>

#include <search/search_move_source.hpp>
#include <search/search_threadcontext.hpp>
#include <search/transposition_table.hpp>
#include <system/time_manager.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <thread>
#include <future>

template<Set us>
SearchResult Search::go(SearchParameters params, TimeManager& clock) {   
    clock.begin();
    m_transpositionTable.incrementAge();

    // go searchmoves, keep the moves that are legal here. With none left the root isn't restricted, the search must
    // still return a move.
    std::vector<PackedMove> rootMoves;
    if (params.SearchMoves.empty() == false) {
        MoveOrderingView ordering;
        SearchMoveSource<us> legalMoves(m_originPosition);
        legalMoves.start(&ordering, MoveTypes::ALL);
        for (PrioritizedMove legal = legalMoves.next(); legal.move.isNull() == false; legal = legalMoves.next()) {
            for (PackedMove requested : params.SearchMoves) {
                if (ThreadSearchContext::sameUciMove(requested, legal.move)) {
                    rootMoves.push_back(legal.move);
                    break;
                }
            }
        }
        if (rootMoves.empty())
            LOG_WARN("go searchmoves: none of the given moves are legal, searching all moves.");
    }

    m_shared.stop.store(false, std::memory_order_relaxed);
    m_shared.nodes.store(0, std::memory_order_relaxed);

    std::vector<std::future<SearchResult>> searchResults;

    for(u16 threadId = 0; threadId < params.ThreadCount; ++threadId) {
        searchResults.push_back(std::async(std::launch::async,
            [this, &params, &clock, &rootMoves, threadId]() {
            ThreadSearchContext searchContext(this->m_originPosition.copy(), us == Set::WHITE, clock);
            searchContext.threadId = threadId;
            searchContext.shared = &m_shared;
            searchContext.nodeLimit = params.NodeLimit;
            searchContext.rootMoves = rootMoves;
            searchContext.begin(threadId, params);
            // prime hashes from historical positions to allow proper 3-fold repetition avoidance
            // TODO: This could be done once for all ThreadContexts.
            for (auto undoUnit : m_gameContext.readGameHistory().moveUndoUnits) {
                searchContext.history.push(undoUnit.hash);
            }
            // the game history holds the positions before each move, the root is the first occurrence the search
            // can repeat.
            searchContext.history.push(searchContext.position.read().hash());
            searchContext.halfmoveClock[1] = m_gameContext.readPly();
            auto result = iterativeDeepening<us>(searchContext, params);
            searchContext.publishNodes();
            // the main thread is done, the helpers' work is only useful to it.
            if (searchContext.isMainThread())
                m_shared.stop.store(true, std::memory_order_relaxed);
            searchContext.end(threadId);
            return result;

        }));
    }

    // the main thread's result is played. fut.get() blocks until a thread is finished & propagates its exceptions.
    SearchResult finalResult;
    for (size_t threadId = 0; threadId < searchResults.size(); ++threadId) {
        try {
            SearchResult result = searchResults[threadId].get();
            if (threadId == 0)
                finalResult = result;
        } catch (const std::exception& e) {
            LOG_ERROR("Search thread exception: {}", e.what());
        }
        // also when the main thread threw, the helpers mustn't keep searching.
        if (threadId == 0)
            m_shared.stop.store(true, std::memory_order_relaxed);
    }
    finalResult.count = m_shared.nodes.load(std::memory_order_relaxed);

    return finalResult;
}

template SearchResult Search::go<Set::WHITE>(SearchParameters, TimeManager&);
template SearchResult Search::go<Set::BLACK>(SearchParameters, TimeManager&);

namespace {
// Lazy SMP helpers skip iterations in blocks so they search ahead of the main thread and of each other instead of all
// at the same depth, Stockfish 8's skip blocks. Helper i uses entry (i - 1) % 20 and skips a depth when
// (depth + phase) / size is odd.
constexpr std::array<u8, 20> c_skipSize  = { 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4 };
constexpr std::array<u8, 20> c_skipPhase = { 0, 1, 0, 1, 2, 3, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 6, 7 };

bool helperSkipsDepth(u16 threadId, u8 depth) {
    if (threadId == 0)
        return false;
    const size_t entry = (threadId - 1u) % c_skipSize.size();
    return ((depth + c_skipPhase[entry]) / c_skipSize[entry]) % 2 != 0;
}
} // namespace

template<Set us>
SearchResult Search::iterativeDeepening(ThreadSearchContext& context, SearchParameters params) {
    SearchResult result;

    // iterative deepening loop -- might make this optional.

    // the hard ply limit in alphaBeta makes deeper iterations pointless.
    const u8 maxDepth = static_cast<u8>(c_maxSearchDepth - 1);
    const u8 depthLimit = params.SearchDepth == 0 ? maxDepth : std::min(params.SearchDepth, maxDepth);

    for (u8 itrDepth = 1; itrDepth <= depthLimit; ++itrDepth) {
        if (helperSkipsDepth(context.threadId, itrDepth))
            continue;

        // the first iteration always completes so there is a move to return, deeper ones may be aborted.
        context.stopEnabled = itrDepth > 1;

        // --- Aspiration Window ---
        // The score rarely moves far between iterations, a narrow window around the last one prunes more. When the
        // score lands outside it the failing side widens and the iteration is searched again. Never around a mate.
        i32 delta = aspiration_params::initialDelta;
        i32 alpha = -c_infinity;
        i32 beta = c_infinity;
        if (itrDepth >= aspiration_params::minDepth && std::abs(result.score) < c_checkmateMinScore) {
            alpha = std::max(result.score - delta, -c_infinity);
            beta = std::min(result.score + delta, c_infinity);
        }

        SearchResult itrResult;
        while (true) {
            itrResult = SearchResult{};
            if (result.pvLine.length > 0) {
                // carry over best move from previous iteration
                itrResult.pvLine.moves[0] = result.pvLine.moves[0];
                itrResult.pvLine.length = 1;
            }

            itrResult.score = alphaBeta<us>(context, itrDepth, checked_cast<i16>(alpha), checked_cast<i16>(beta), 1, &itrResult.pvLine);
            if (context.stopped)
                break;

            // only a side that isn't fully open yet can fail, a full window always ends the loop.
            delta *= 2;
            if (itrResult.score <= alpha && alpha > -c_infinity)
                alpha = delta > aspiration_params::maxDelta ? -c_infinity : std::max(itrResult.score - delta, -c_infinity);
            else if (itrResult.score >= beta && beta < c_infinity)
                beta = delta > aspiration_params::maxDelta ? c_infinity : std::min(itrResult.score + delta, c_infinity);
            else
                break;
        }

        // aborted iteration, its score & pv can't be trusted. Keep the last complete iteration.
        if (context.stopped)
            break;

        // the time this iteration finished, reported with the nodes of all threads so nodes / time is the real nps.
        // Only the main thread reports, the helpers are silent.
        if (context.isMainThread())
            reportResult(itrResult, itrDepth, context.totalNodes(), context.clock.now());

        // forced mate check
        i32 checkmateDistance = c_checkmateConstant - abs(itrResult.score);
        checkmateDistance = abs(checkmateDistance);
        ASSERT_MSG(checkmateDistance >= 0, "Checkmate distance should never be negative.");
        if (static_cast<u32>(checkmateDistance) <= c_maxSearchDepth)
            itrResult.ForcedMate = true;

        if (itrResult.ForcedMate) {
            result = itrResult;
            break;
        }

        result = itrResult;

        if (context.clock.shouldStop() || context.nodeLimitReached() || context.searchStopped())
            break;

        // whether another iteration fits the time is the main thread's call, the helpers search until it's done.
        if (context.isMainThread() && context.clock.continueIterativeDeepening() == false)
            break;
    }
    
    if constexpr (search_policies::TT::enabled) {
        if (context.isMainThread())
            search_policies::TT::printStats();
    }
    return result;
}

template SearchResult Search::iterativeDeepening<Set::WHITE>(ThreadSearchContext&, SearchParameters);
template SearchResult Search::iterativeDeepening<Set::BLACK>(ThreadSearchContext&, SearchParameters);