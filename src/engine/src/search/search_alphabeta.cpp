#include <search/search.hpp>

#include <array>
#include <span>

#include <eval/evaluator.hpp>

#include <move/move_executor.hpp>

#include <search/search_constants.hpp>
#include <search/search_move_source.hpp>
#include <search/search_policies.hpp>
#include <search/search_threadcontext.hpp>
#include <search/transposition_table.hpp>

#include <system/time_manager.hpp>

template<Set us>
i16 Search::alphaBeta(ThreadSearchContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv) {
    ASSERT_MSG(depth >= 0, "Depth cannot be negative in alphaBeta.");
    ASSERT_MSG(ply < c_maxSearchDepth, "Ply exceeds maximum search depth in alphaBeta.");
    ASSERT_MSG(alpha >= -c_infinity && beta <= c_infinity, "Alpha and Beta must be within valid bounds in alphaBeta.");

    context.debug_print_alphabeta_entry(depth, ply, alpha, beta, context.position.read().hash());
    
    // PVS searches everything off the principal variation with a null window, so a wider window means a PV node.
    // The root is always a PV node. Pruning and TT cutoffs are only safe at non-PV nodes.
    const bool isPV = beta - alpha > 1;

    PositionReader pos = context.position.read();
    if (context.history.isRepetition(pos.hash()) == true) {
        return -c_drawConstant;
    }

    // hard ply limit, killers & pv are sized by it and asserts are compiled out. Check extensions can otherwise keep
    // depth from ever reaching 0 along a chain of checks.
    if (ply >= c_maxSearchDepth - 1)
        return context.evaluate<us>();

    // Knows whether we're in check before generating anything, start() is called once ordering is primed below.
    SearchMoveSource<us> moves(pos);

    // --- Check Extension ---
    // Search one ply deeper when in check so forcing sequences aren't cut off at the horizon. Covers direct and
    // discovered checks alike. Done before the TT probe so stored and probed depths agree.
    if (moves.isChecked())
        depth++;

    PackedMove bestMove = PackedMove::NullMove();

    // --- Transposition Table Probe ---
    TranspositionFlag flag = TranspositionFlag::TTF_NONE;
    if constexpr (search_policies::TT::enabled) {
        // probe only returns a score when the entry is deep enough and its bound allows a cutoff with this window.
        // bestMove is set on any hit, also when the entry is too shallow to cut.
        std::optional<i16> ttProbeResult = search_policies::TT::probe(pos.hash(), depth, ply, alpha, beta, flag, bestMove);

        // No cutoffs at PV nodes, the stored score may come from an earlier search or another path with different
        // repetitions. At the root it would also play the stored move without searching it. The TT move still
        // orders first below.
        if (!isPV && ttProbeResult.has_value()) {
            pv->length = 0;
            return ttProbeResult.value();
        }
    }

    // --- Internal Iterative Reduction ---
    // Without a TT or PV move the ordering is a guess and the node is likely new to the search, a ply shallower is
    // cheaper and stores a move for the next visit.
    if constexpr (search_policies::enabled_policies::IIR) {
        if (depth >= iir_params::minDepth && bestMove.isNull() && pv->length == 0)
            depth--;
    }

    // --- Leaf Node Check ---
    // Before generating moves, most nodes are leaves and quiescence generates its own. In check the extension above
    // keeps depth > 0, so mates are still found by the full search; a stalemate at the horizon gets quiescence's
    // stand pat instead of a draw score.
    if (depth <= 0) {
        pv->length = 0;
        if constexpr (search_policies::QuiescencePolicy::enabled) {
            // Start Q-Search with its *own* depth limit, configured with search params.
            return quiescence<us>(context, search_policies::QuiescencePolicy::maxDepth, alpha, beta, ply);
        } else {
            return context.evaluate<us>();
        }
    }

    // --- No-Moves Check (Mate/Stalemate) ---
    MoveOrderingView orderingView;

    // --- prime move ordering ---
    if (bestMove.isNull() == false) orderingView.ttMove = bestMove;
    if (pv->length > 0) orderingView.pvMove = pv->moves[0];
    search_policies::MoveOrdering::prime(context.moveOrdering, orderingView, ply);

    moves.start(&orderingView, MoveTypes::ALL);

    // --- Terminal Node ---
    if (moves.peek().isNull()) {
        if (moves.isChecked()){
            ASSERT(ply < c_checkmateMaxDistance);
            return -c_checkmateConstant + (i16)ply; // Mate score adjusted by ply
        }
        return -c_drawConstant; // Stalemate
    }

    // --- Static Eval ---
    // Only the pruning at non-PV nodes uses it, and it means nothing in check.
    const bool inCheck = moves.isChecked();
    const i16 staticEval = (isPV || inCheck) ? -c_infinity : context.evaluate<us>();

    // --- Reverse Futility Pruning ---
    if constexpr (search_policies::RFP::enabled) {
        if (search_policies::RFP::prune(isPV, inCheck, depth, staticEval, beta)) {
            pv->length = 0;
            return staticEval;
        }
    }

    // --- Null Move Pruning ---
    if constexpr (search_policies::NMP::enabled) {
        if (!isPV && !inCheck) {
            if (std::optional<i16> nullScore = tryNullMovePrune<us>(context, depth, beta, ply, staticEval)) {
                pv->length = 0;
                return nullScore.value();
            }
        }
    }

    // --- Futility Pruning ---
    // decided per node, searchMoves skips the quiet moves.
    bool futile = false;
    if constexpr (search_policies::Futility::enabled)
        futile = search_policies::Futility::nodeIsFutile(isPV, inCheck, depth, staticEval, alpha);

    // --- Main Search Loop ---
    flag = TranspositionFlag::TTF_CUT_ALPHA; // Assume we'll fail-low
    i16 eval = searchMoves<us>(moves, context, depth, alpha, beta, ply, pv, flag, bestMove, futile);

    // aborted, the score is meaningless and must not reach the TT.
    if (context.stopped)
        return 0;

    // --- Store to TT ---
    if constexpr (search_policies::TT::enabled) {
        // All moves searched, no cutoff.
        search_policies::TT::update(
            pos.hash(),
            bestMove, // Store the best move found
            eval, // Store the best score (which is alpha if it was a PV node)
            depth,
            ply,
            flag); // Flag is either TTF_CUT_ALPHA or TTF_CUT_EXACT
    }

    return eval;
}

template i16 Search::alphaBeta<Set::WHITE>(ThreadSearchContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv);
template i16 Search::alphaBeta<Set::BLACK>(ThreadSearchContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv);

template<Set us>
i16 Search::searchMoves(SearchMoveSource<us>& moves, ThreadSearchContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv, TranspositionFlag& flag, PackedMove& outMove, bool futile) {
    // --- Main Search Loop ---
    PositionReader pos = context.position.read();
    // window at entry, alpha moves during the loop.
    const bool isPV = beta - alpha > 1;

    i16 bestEval = -c_infinity; // Start at -infinity
    PVLine childPv;
    u16 index = 0;

    MoveExecutor executor(context.position.edit());
    PrioritizedMove ordered = moves.next();
    
    // We need to store the "Best Move Found So Far" locally to update outMove correctly
    PackedMove intermmediateMove = PackedMove::NullMove();

    // quiets searched without failing high, they get a history malus if a later quiet does. Beyond the buffer they're
    // simply not penalised.
    std::array<PackedMove, 64> quietsTried;
    u16 quietCount = 0;

    // --- Late Move Pruning ---
    // at shallow non-PV nodes only the first quiets are searched, the rest rarely matter.
    u16 quietLimit = 0;
    u16 quietsSearched = 0;
    if constexpr (search_policies::LMP::enabled)
        quietLimit = search_policies::LMP::quietLimit(isPV, moves.isChecked(), depth);

    u16 movingPly = ply; 

    do {
        if (context.shouldStop()) break;

        PackedMove move = ordered.move;

        // --- Futility Pruning ---
        // skipped quiets weren't searched, they get no history malus either.
        if (search_policies::Futility::skipMove(futile, index, move, ordered.isCheck())) {
            ordered = moves.next();
            index++;
            continue;
        }

        // --- Late Move Pruning ---
        // skipped quiets weren't searched, they get no history malus either.
        if (search_policies::LMP::skipMove(quietLimit, quietsSearched, bestEval, move, ordered.isCheck())) {
            ordered = moves.next();
            index++;
            continue;
        }

        // check extensions happen in the child, at alphaBeta entry when it's in check.
        u8 adjustedDepth = depth;

        MoveUndoUnit undoState;
        executor.makeMove(move, undoState, movingPly);
        context.history.push(pos.hash());
        
        i16 eval;

        // update depth
        i32 nextDepth = static_cast<i32>(adjustedDepth) - 1;
        adjustedDepth = static_cast<u8>(std::max(nextDepth, 0));

        if (index == 0 && depth > 0) {
            // --- PV Search (First Move) ---
            // Full window, full trust.
            eval = -alphaBeta<opposing_set<us>()>(context, adjustedDepth, -beta, -alpha, ply + 1, &childPv);            
        } 
        else {
            // --- Scout Search (Subsequent Moves) ---
            // Zero window: Try to prove move is <= alpha
            this->scout_search_count++;
            context.scout_search();

            // --- Late Move Reduction ---
            // Quiet moves late in the ordering rarely matter, scout them shallower first.
            u8 reduction = 0;
            if constexpr (search_policies::LMR::enabled) {
                reduction = search_policies::LMR::reduction(depth, index, isPV, search_policies::MoveOrdering::isQuiet(move),
                    moves.isChecked(), ordered.isCheck());
            }

            eval = -alphaBeta<opposing_set<us>()>(context, static_cast<u8>(adjustedDepth - reduction), -alpha - 1, -alpha, ply + 1, &childPv);

            // the reduced search beat alpha, check it at full depth before trusting it.
            if (reduction > 0 && eval > alpha && !context.stopped)
                eval = -alphaBeta<opposing_set<us>()>(context, adjustedDepth, -alpha - 1, -alpha, ply + 1, &childPv);
            
            // --- The Re-Search Trigger ---
            // If eval > alpha, the move is better than we thought. 
            // We must re-search with the full window to get the exact score.
            // (Only if it's also < beta, otherwise we just take the beta cutoff)            
            if (eval > alpha && eval < beta) {
                context.scout_re_search();
                this->scout_re_search_count++;
                eval = -alphaBeta<opposing_set<us>()>(context, adjustedDepth, -beta, -alpha, ply + 1, &childPv);
                
            }
        }

        context.history.pop();
        executor.unmakeMove(undoState);
        context.nodeCount++;

        // the child was aborted, its score is meaningless. Unwind without touching alpha, pv or killers.
        if (context.stopped)
            return 0;

        context.debug_print_eval(move, eval, alpha, beta, depth, ply, pos.hash());

        // --- Update Best Score (Fail-Soft) ---
        if (eval > bestEval) {
            bestEval = eval;
            intermmediateMove = move; // Update local tracker
            
            // --- Beta Cutoff (Fail-High) ---
            if (bestEval >= beta) {
                flag = TranspositionFlag::TTF_CUT_BETA;
                if (search_policies::MoveOrdering::isQuiet(move))
                    search_policies::MoveOrdering::updateQuietCutoff(context.moveOrdering, us, move,
                        std::span<const PackedMove>(quietsTried.data(), quietCount), depth, ply);
                outMove = intermmediateMove;
                return bestEval;
            }

            // --- Alpha Update (PV Node) ---
            if (bestEval > alpha) {
                alpha = bestEval;
                flag = TranspositionFlag::TTF_CUT_EXACT;
                outMove = intermmediateMove;

                // Update PV
                pv->moves[0] = intermmediateMove;
                memcpy(pv->moves + 1, childPv.moves, childPv.length * sizeof(PackedMove));
                pv->length = childPv.length + 1;
            }
        }

        if (search_policies::MoveOrdering::isQuiet(move)) {
            quietsSearched++;
            if (quietCount < quietsTried.size())
                quietsTried[quietCount++] = move;
        }

        ordered = moves.next();
        index++;
    } while (ordered.move.isNull() == false);

    if (context.stopped)
        return 0;

    // Ensure outMove is set if we found *any* valid move that improved bestEval (even if it didn't beat alpha)
    // Though usually, we only care about outMove if it beat alpha (PV) or beta (Cutoff).
    if (intermmediateMove.isNull() == false && outMove.isNull()) {
        outMove = intermmediateMove;
    }

    return bestEval;
}

template i16 Search::searchMoves<Set::WHITE>(SearchMoveSource<Set::WHITE>& moves, ThreadSearchContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv, TranspositionFlag& flag, PackedMove& outMove, bool futile);
template i16 Search::searchMoves<Set::BLACK>(SearchMoveSource<Set::BLACK>& moves, ThreadSearchContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv, TranspositionFlag& flag, PackedMove& outMove, bool futile);