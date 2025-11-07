#pragma once

template<Set us>
SearchResult Search::go(SearchParameters params)
{   
    m_transpositionTable.incrementAge();
    
    ThreadSearchContext searchContext(m_originPosition.copy(), us == Set::WHITE);    
    return dispatchSearch<us>(searchContext, params);
}

template<Set us, typename config>
SearchResult Search::iterativeDeepening(ThreadSearchContext& context, SearchParameters params) {    
    SearchResult result;

    u32 timeLeft = params.BlackTimelimit;
    u32 timeIncrement = params.BlackTimeIncrement;
    if constexpr (us == Set::WHITE) {
        timeLeft = params.WhiteTimelimit;
        timeIncrement = params.WhiteTimeIncrement;
    }

    // iterative deepening loop -- might make this optional.
    for (u8 itrDepth = 1; itrDepth <= params.SearchDepth; ++itrDepth) {        
        const Clock& itrClock = config::Debug_Policy::pushClock();

        SearchResult itrResult;        
        itrResult.score = alphaBeta<us, config>(context, itrDepth, -c_infinity, c_infinity, 1, &itrResult.pvLine);

        reportResult(itrResult, itrDepth, context.nodeCount, itrClock);

        config::Debug_Policy::reportNps(context.nodeCount, context.qNodeCount);
        config::Debug_Policy::popClock();

        // bool cancelled = cancellationFunc();
        // if (cancelled) {
        //     itrResult = result;
        // }


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
        
        if (allowAnotherIteration(itrClock.getElapsedTime(), timeLeft, timeIncrement, itrDepth) == false)
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
        return dispatchDebug<us, TT, NMP, LMR, search_policies::QSearchEnabled>(context, params);
    } else {
        return dispatchDebug<us, TT, NMP, LMR, search_policies::QSearchDisabled>(context, params);
    }
}

template<Set us, typename TT, typename NMP, typename LMR, typename QSearch>
SearchResult Search::dispatchDebug(ThreadSearchContext& context, SearchParameters params) {    
#if defined(DEVELOPMENT_BUILD)
    using Config = SearchConfig<TT, NMP, LMR, QSearch, search_policies::DebugEnabled>;
    return iterativeDeepening<us, Config>(context, params);
#else
    using Config = SearchConfig<TT, NMP, LMR, QSearch, search_policies::DebugDisabled>;
    return iterativeDeepening<us, Config>(context, params);
#endif
}