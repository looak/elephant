#include <search/search.hpp>

#include <core/game_context.hpp>

#include <search/search_threadcontext.hpp>
#include <search/transposition_table.hpp>
#include <system/time_manager.hpp>

#include <thread>
#include <future>

template<Set us>
SearchResult Search::go(SearchParameters params, TimeManager& clock) {   
    clock.begin();
    m_transpositionTable.incrementAge();
    
    std::vector<std::future<SearchResult>> searchResults;
    
    for(u16 threadId = 0; threadId < params.ThreadCount; ++threadId) {
        searchResults.push_back(std::async(std::launch::async,
            [this, &params, &clock]() {
            ThreadSearchContext searchContext(this->m_originPosition.copy(), us == Set::WHITE, clock);
            // prime hashes from historical positions to allow proper 3-fold repetition avoidance
            // TODO: This could be done once for all ThreadContexts.
            for (auto undoUnit : m_gameContext.readGameHistory().moveUndoUnits) {
                searchContext.history.push(undoUnit.hash);
            }
            auto result = iterativeDeepening<us>(searchContext, params);
            result.count = searchContext.nodeCount + searchContext.qNodeCount;
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

    u64 lastIterationTimeSpan = context.clock.now();

    // iterative deepening loop -- might make this optional.
    
    for (u8 itrDepth = 1; itrDepth <= params.SearchDepth; ++itrDepth) {
        SearchResult itrResult;
        if (result.pvLine.length > 0) {
            // carry over best move from previous iteration
            itrResult.pvLine.moves[0] = result.pvLine.moves[0]; 
            itrResult.pvLine.length = 1;
        }

        itrResult.score = alphaBeta<us>(context, itrDepth, -c_infinity, c_infinity, 1, &itrResult.pvLine);

        reportResult(itrResult, itrDepth, context.nodeCount + context.qNodeCount, lastIterationTimeSpan);

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
        
        u64 iterationTimeSpan = context.clock.now() - lastIterationTimeSpan;
        lastIterationTimeSpan = context.clock.now();

        if (context.clock.shouldStop() == true)
            break;

        if (context.clock.continueIterativeDeepening(iterationTimeSpan) == false)
            break;
    }
    
    if constexpr (search_policies::TT::enabled)
        search_policies::TT::printStats();
    return result;
}

template SearchResult Search::iterativeDeepening<Set::WHITE>(ThreadSearchContext&, SearchParameters);
template SearchResult Search::iterativeDeepening<Set::BLACK>(ThreadSearchContext&, SearchParameters);