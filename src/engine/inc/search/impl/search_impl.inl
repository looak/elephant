#pragma once
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
            LOG_ERROR() << "Search thread exception: " << e.what();
        }
    }

    return finalResult;
}

template<Set us>
SearchResult Search::iterativeDeepening(ThreadSearchContext& context, SearchParameters params) {    
    SearchResult result;

    u64 lastIterationTimeSpan = context.clock.now();

    // iterative deepening loop -- might make this optional.
    for (u8 itrDepth = 1; itrDepth <= params.SearchDepth; ++itrDepth) {
        SearchResult itrResult;        
        itrResult.score = alphaBeta<us>(context, itrDepth, -c_infinity, c_infinity, 1, &itrResult.pvLine);

        reportResult(itrResult, itrDepth, context.nodeCount + context.qNodeCount, lastIterationTimeSpan);

        // forced mate check
        i16 checkmateDistance = c_checkmateConstant - abs((int)itrResult.score);
        checkmateDistance = abs(checkmateDistance);
        if ((u32)checkmateDistance <= c_maxSearchDepth)
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