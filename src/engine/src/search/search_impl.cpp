#include <search/search.hpp>

#include <core/game_context.hpp>
#include <math/cast.hpp>

#include <search/search_threadcontext.hpp>
#include <search/transposition_table.hpp>
#include <system/time_manager.hpp>

#include <algorithm>
#include <cstdlib>
#include <thread>
#include <future>

template<Set us>
SearchResult Search::go(SearchParameters params, TimeManager& clock) {   
    clock.begin();
    m_transpositionTable.incrementAge();
    
    std::vector<std::future<SearchResult>> searchResults;
    
    for(u16 threadId = 0; threadId < params.ThreadCount; ++threadId) {
        searchResults.push_back(std::async(std::launch::async,
            [this, &params, &clock, threadId]() {
            ThreadSearchContext searchContext(this->m_originPosition.copy(), us == Set::WHITE, clock);
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
            result.count = searchContext.nodeCount + searchContext.qNodeCount;
            searchContext.end(threadId);
            return result;

        }));
    }    

    SearchResult finalResult;
    for (auto& fut : searchResults) {
        // fut.get() will BLOCK until the thread is finished
        // (either by completing or by seeing the cancel signal)
        // It also propagates exceptions.
        try {
            finalResult = fut.get();            
        } catch (const std::exception& e) {
            LOG_ERROR("Search thread exception: {}", e.what());
        }
    }

    return finalResult;
}

template SearchResult Search::go<Set::WHITE>(SearchParameters, TimeManager&);
template SearchResult Search::go<Set::BLACK>(SearchParameters, TimeManager&);

template<Set us>
SearchResult Search::iterativeDeepening(ThreadSearchContext& context, SearchParameters params) {    
    SearchResult result;

    // iterative deepening loop -- might make this optional.
    
    // the hard ply limit in alphaBeta makes deeper iterations pointless.
    const u8 maxDepth = static_cast<u8>(c_maxSearchDepth - 1);
    const u8 depthLimit = params.SearchDepth == 0 ? maxDepth : std::min(params.SearchDepth, maxDepth);

    for (u8 itrDepth = 1; itrDepth <= depthLimit; ++itrDepth) {
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

        // the time this iteration finished, reported with its node count so nodes / time is the real nps.
        reportResult(itrResult, itrDepth, context.nodeCount + context.qNodeCount, context.clock.now());

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

        if (context.clock.shouldStop())
            break;

        if (context.clock.continueIterativeDeepening() == false)
            break;
    }
    
    if constexpr (search_policies::TT::enabled)
        search_policies::TT::printStats();
    return result;
}

template SearchResult Search::iterativeDeepening<Set::WHITE>(ThreadSearchContext&, SearchParameters);
template SearchResult Search::iterativeDeepening<Set::BLACK>(ThreadSearchContext&, SearchParameters);