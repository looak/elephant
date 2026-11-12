#include <search/worker.hpp>

#include <math/cast.hpp>
#include <diagnostics/logger.hpp>

#include <search/core/search_move_source.hpp>
#include <search/state/thread_context.hpp>
#include <search/state/transposition_table.hpp>
#include <system/time_manager.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>

void Worker::newGame()
{
    m_heuristics.reset();
    m_pawnHash.clear();
}

SearchResult Worker::run(const SearchRequest& request, const TimeManager& clock, TranspositionTable& tt, SharedSearchState& shared,
                         SearchListener& listener, u16 threadId, u16 threadCount)
{
    ThreadContext context(request, clock, tt, m_heuristics, m_pawnHash);
    context.threadId = threadId;
    context.shared = &shared;
    context.debug_print_begin(threadId, threadCount);
    // proper 3-fold repetition avoidance needs the game's earlier positions.
    context.repetitions.setGameHashes(request.gameHashes);
    // the game's hashes are the positions before each move, the root is the first occurrence the search can repeat.
    context.repetitions.push(context.position.read().hash());
    context.frame(1).halfmoveClock = request.rootHalfmoveClock;

    SearchResult result = request.toPlay == Set::WHITE ? iterativeDeepening<Set::WHITE>(context, listener)
                                                       : iterativeDeepening<Set::BLACK>(context, listener);
    context.publishNodes();
    result.mainThreadCount = context.nodeCount + context.qNodeCount;
    // the main thread is done, the helpers' work is only useful to it.
    if (context.isMainThread())
        shared.stop.store(true, std::memory_order_relaxed);
    context.debug_print_end(threadId);
    return result;
}

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
SearchResult Worker::iterativeDeepening(ThreadContext& context, SearchListener& listener) {
    SearchResult result;
    const SearchLimits& limits = context.request.limits;
    const bool restrictedRoot = context.request.rootMoves.empty() == false;

    // the hard ply limit in alphaBeta makes deeper iterations pointless.
    const u8 maxDepth = static_cast<u8>(c_maxSearchDepth - 1);
    const u8 depthLimit = limits.SearchDepth == 0 ? maxDepth : std::min(limits.SearchDepth, maxDepth);

    u8 unchangedBestMove = 0;

    for (u8 itrDepth = 1; itrDepth <= depthLimit; ++itrDepth) {
        if (helperSkipsDepth(context.threadId, itrDepth))
            continue;

        // the first iteration always completes so there is a move to return, deeper ones may be aborted.
        // TODO: rename and move out of iterartive deepening to allow context to be const.
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

            itrResult.score = restrictedRoot
                ? alphaBeta<us, NodeKind::RESTRICTED_ROOT>(context, NodeInputs{ checked_cast<i16>(alpha), checked_cast<i16>(beta), itrDepth, 1 }, &itrResult.pvLine)
                : alphaBeta<us, NodeKind::ROOT>(context, NodeInputs{ checked_cast<i16>(alpha), checked_cast<i16>(beta), itrDepth, 1 }, &itrResult.pvLine);
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

        // the time this iteration finished. hashfull walks a thousand entries and nothing reads it from a helper.
        const bool isMain = context.isMainThread();
        listener.onIteration({ .threadId = context.threadId, .depth = itrDepth, .score = itrResult.score,
            .nodes = isMain ? context.totalNodes() : context.nodeCount + context.qNodeCount,
            .timeMs = context.clock.now(), .hashfull = isMain ? context.tt.hashfull() : 0u,
            .pv = itrResult.pvLine });

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
        
        if (result.pvLine.moves[0] == itrResult.pvLine.moves[0])
            unchangedBestMove = static_cast<u8>(std::min(5, unchangedBestMove + 1));
        else 
            unchangedBestMove = 0;

        result = itrResult;

        if (context.clock.shouldStop() || context.nodeLimitReached() || context.searchStopped())
            break;

        // whether another iteration fits the time is the main thread's call, the helpers search until it's done.
        if (context.isMainThread() 
            && context.clock.continueIterativeDeepening(time_management::unchangedMoveFactors[unchangedBestMove]) == false)
            break;
    }
    
    if constexpr (search_policies::TT::enabled) {
        if (context.isMainThread())
            search_policies::TT::printStats(context.tt);
    }
    return result;
}

template SearchResult Worker::iterativeDeepening<Set::WHITE>(ThreadContext&, SearchListener&);
template SearchResult Worker::iterativeDeepening<Set::BLACK>(ThreadContext&, SearchListener&);