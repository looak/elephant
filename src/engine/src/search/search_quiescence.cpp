#include <search/search.hpp>

#include <eval/evaluator.hpp>

#include <move/move_executor.hpp>

#include <search/search_move_source.hpp>
#include <search/search_policies.hpp>
#include <search/search_threadcontext.hpp>
#include <search/static_exchange.hpp>
#include <search/transposition_table.hpp>
#include <system/time_manager.hpp>

template<Set us>
i16 Search::quiescence(ThreadSearchContext& context, u8 depth, i16 alpha, i16 beta, u16 ply) {
    ASSERT_MSG(ply < c_maxSearchDepth, "Ply exceeds maximum search depth in quiescence.");
    ASSERT_MSG(alpha >= -c_infinity && beta <= c_infinity, "Alpha and Beta must be within valid bounds in quiescence.");

    // hard ply limit, the static eval is the best we can do even when in check.
    if (ply >= c_maxSearchDepth - 1)
        return context.evaluate<us>();

    // window at entry, alpha moves with stand pat & the loop.
    const i16 alphaAtEntry = alpha;
    const bool isPV = beta - alpha > 1;
    const u64 hash = context.position.read().hash();

    // --- Transposition Table Probe ---
    // Quiescence results are depth 0, every entry is deep enough. No cutoffs at PV nodes, as in alphaBeta. The TT
    // move is searched first when it's a capture.
    MoveOrderingView ordering{};
    TranspositionFlag ttFlag = TranspositionFlag::TTF_NONE;
    if constexpr (search_policies::TT::enabled && search_policies::QuiescencePolicy::transpositionTable) {
        std::optional<i16> ttScore = search_policies::TT::probe(hash, 0, ply, alpha, beta, ttFlag, ordering.ttMove);
        if (!isPV && ttScore.has_value())
            return ttScore.value();
    }

    // A quiescence result is only stored for a position the table doesn't know yet, a store always overwrites an
    // entry for the same position, also a deeper one from the main search.
    auto store = [&](i16 score, PackedMove move) {
        if constexpr (search_policies::TT::enabled && search_policies::QuiescencePolicy::transpositionTable) {
            if (ttFlag != TranspositionFlag::TTF_NONE)
                return;
            const TranspositionFlag bound = score >= beta ? TranspositionFlag::TTF_CUT_BETA
                : score > alphaAtEntry ? TranspositionFlag::TTF_CUT_EXACT
                : TranspositionFlag::TTF_CUT_ALPHA;
            search_policies::TT::update(hash, move, score, 0, ply, bound);
        }
    };

    // The move source knows whether we're in check before generating anything, the filter is decided after.
    SearchMoveSource<us> moves(context.position.read());
    const bool inCheck = moves.isChecked();

    i16 bestEval = -c_infinity;
    i16 standPat = -c_infinity;
    PackedMove bestMove = PackedMove::NullMove();
    if (inCheck) {
        // no stand pat while in check, every evasion has to be searched. Captures alone could miss the only escapes.
        moves.start(&ordering, MoveTypes::ALL);
    }
    else {
        standPat = context.evaluate<us>();

        // Stand-pat beta cutoff
        if (standPat >= beta)
            return standPat;

        if (standPat > alpha)
            alpha = standPat;

        // Leaf node - return stand-pat
        if (depth == 0)
            return standPat;

        bestEval = standPat;

        // --- Delta Pruning (node) ---
        if constexpr (search_policies::QuiescencePolicy::deltaPruning) {
            if (search_policies::QuiescencePolicy::deltaPruneNode(context.position.read().material(), us, standPat, alpha))
                return standPat;
        }

        moves.start(&ordering, MoveTypes::CAPTURES_ONLY);
    }

    // evasions don't consume quiescence depth, the line stays bounded since only captures are searched once out of
    // check, and the ply limit above.
    const u8 childDepth = (inCheck || depth == 0) ? depth : static_cast<u8>(depth - 1);
    u32 movesSearched = 0;
    PrioritizedMove ordered = moves.next();

    while (!ordered.move.isNull()) {
        if (context.shouldStop())
            break;

        PackedMove move = ordered.move;

        // --- Delta Pruning (move) ---
        if constexpr (search_policies::QuiescencePolicy::deltaPruning) {
            if (!inCheck && search_policies::QuiescencePolicy::deltaPruneMove(context.position.read().material(), move, standPat, alpha)) {
                ordered = moves.next();
                continue;
            }
        }

        // --- SEE Pruning ---
        // a capture that loses material once the exchange on its square plays out won't raise alpha.
        if constexpr (search_policies::QuiescencePolicy::seePruning) {
            if (!inCheck && !see::ge(context.position.read().material(), move, 0)) {
                ordered = moves.next();
                continue;
            }
        }

        MoveExecutor executor(context.position.edit());
        MoveUndoUnit undoState;
        // makeMove updates its ply argument as the fifty move counter (reset on captures), never hand it the search ply.
        u16 fiftyMoveCounter = ply;
        executor.makeMove(move, undoState, fiftyMoveCounter);
        
        i16 qEval = -quiescence<opposing_set<us>()>(context, childDepth, -beta, -alpha, ply + 1);
        context.qNodeCount++;

        executor.unmakeMove(undoState);

        // the child was aborted, its score is meaningless.
        if (context.stopped)
            return 0;

        ++movesSearched;

        if (qEval > bestEval) {
            bestEval = qEval;
            bestMove = move;
        }

        if (bestEval >= beta) {
            store(bestEval, bestMove);
            return bestEval;
        }
        
        if (bestEval > alpha)
            alpha = bestEval;

        ordered = moves.next();
    }

    if (context.stopped)
        return 0;

    // In check with every legal move generated and none found -> Checkmate.
    if (inCheck && movesSearched == 0) {
        return checked_cast<i16>(-c_checkmateConstant + ply); // Mate score
    }

    store(bestEval, bestMove);
    return bestEval;
}

template i16 Search::quiescence<Set::WHITE>(ThreadSearchContext&, u8, i16, i16, u16);
template i16 Search::quiescence<Set::BLACK>(ThreadSearchContext&, u8, i16, i16, u16);