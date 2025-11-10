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
            auto result = dispatchSearch<us>(searchContext, params);
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

template<Set us, typename config>
SearchResult Search::iterativeDeepening(ThreadSearchContext& context, SearchParameters params) {    
    SearchResult result;

    u64 lastIterationTimeSpan = context.clock.now();

    // iterative deepening loop -- might make this optional.
    for (u8 itrDepth = 1; itrDepth <= params.SearchDepth; ++itrDepth) {
        const Clock& itrClock = config::Debug_Policy::pushClock();        

        SearchResult itrResult;        
        itrResult.score = alphaBeta<us, config>(context, itrDepth, -c_infinity, c_infinity, 1, &itrResult.pvLine);

        reportResult(itrResult, itrDepth, context.nodeCount + context.qNodeCount, itrClock);

        config::Debug_Policy::reportNps(context.nodeCount, context.qNodeCount);
        config::Debug_Policy::popClock();

        // forced mate check
        i16 checkmateDistance = c_checkmateConstant - abs((int)itrResult.score);
        checkmateDistance = abs(checkmateDistance);
        if ((u32)checkmateDistance <= c_maxSearchDepth)
            itrResult.ForcedMate = true;

        if (itrResult.ForcedMate) {
            config::TT_Policy::printStats();
            return itrResult;
        }

        result = itrResult;
        
        u64 iterationTimeSpan = context.clock.now() - lastIterationTimeSpan;
        lastIterationTimeSpan = context.clock.now();

        if (context.clock.shouldStop() == true)
            break;

        if (context.clock.continueIterativeDeepening(iterationTimeSpan) == false)
            break;
    }
    
    config::TT_Policy::printStats();
    return result;
}


// --- Dispatcher Implementation ---
template<Set us>
SearchResult Search::dispatchSearch(ThreadSearchContext& context, SearchParameters params) {
    if (params.UseTranspositionTable) {
        search_policies::TTEnabled::assign(m_transpositionTable);
        return dispatchNMP<us, search_policies::TTEnabled>(context, params);
    } else {
        return dispatchNMP<us, search_policies::TTDisabled>(context, params);
    }
}

template<Set us, typename TT>
SearchResult Search::dispatchNMP(ThreadSearchContext& context, SearchParameters params) {
    if (params.UseNullMovePruning) {        
        return dispatchLMR<us, TT, search_policies::NmpEnabled>(context, params);
    } else {        
        return dispatchLMR<us, TT, search_policies::NmpDisabled>(context, params);
    }
}

template<Set us, typename TT, typename NMP>
SearchResult Search::dispatchLMR(ThreadSearchContext& context, SearchParameters params) {
    if (params.UseLateMoveReduction) {
        return dispatchQSearch<us, TT, NMP, search_policies::LmrEnabled>(context, params);
    } else {
        return dispatchQSearch<us, TT, NMP, search_policies::LmrDisabled>(context, params);
    }
}

template<Set us, typename TT, typename NMP, typename LMR>
SearchResult Search::dispatchQSearch(ThreadSearchContext& context, SearchParameters params)
{
    if (params.UseQuiescenceSearch) {
        search_policies::QSearchEnabled::maxDepth = params.QuiescenceDepth;
        return dispatchOrdering<us, TT, NMP, LMR, search_policies::QSearchEnabled>(context, params);
    } else {
        return dispatchOrdering<us, TT, NMP, LMR, search_policies::QSearchDisabled>(context, params);
    }
}

template<Set us, typename TT, typename NMP, typename LMR, typename QSearch>
SearchResult Search::dispatchOrdering(ThreadSearchContext& context, SearchParameters params)
{
    if (params.UseMoveOrdering) {        
        return dispatchDebug<us, TT, NMP, LMR, QSearch, search_policies::OrderingEnabled>(context, params);
    } else {
        return dispatchDebug<us, TT, NMP, LMR, QSearch, search_policies::OrderingDisabled>(context, params);
    }
}

template<Set us, typename TT, typename NMP, typename LMR, typename QSearch, typename Ordering>
SearchResult Search::dispatchDebug(ThreadSearchContext& context, SearchParameters params) {    
#if defined(DEVELOPMENT_BUILD)
    using Config = SearchConfig<TT, NMP, LMR, QSearch, Ordering, search_policies::DebugEnabled>;
    return iterativeDeepening<us, Config>(context, params);
#else
    using Config = SearchConfig<TT, NMP, LMR, QSearch, Ordering, search_policies::DebugDisabled>;
    return iterativeDeepening<us, Config>(context, params);
#endif
}