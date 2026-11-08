#include <search/worker.hpp>
#include <position/position_hash.hpp>

#include <array>
#include <span>
#include <type_traits>

#include <eval/evaluator.hpp>

#include <move/move_executor.hpp>

#include <search/core/search_constants.hpp>
#include <search/core/search_move_source.hpp>
#include <search/core/search_policies.hpp>
#include <search/state/thread_context.hpp>
#include <search/state/transposition_table.hpp>

#include <system/time_manager.hpp>

template<Set us, NodeKind kind>
i16 Worker::alphaBeta(ThreadContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv) {
    ASSERT_MSG(depth >= 0, "Depth cannot be negative in alphaBeta.");
    ASSERT_MSG(ply < c_maxSearchDepth, "Ply exceeds maximum search depth in alphaBeta.");
    ASSERT_MSG(alpha >= -c_infinity && beta <= c_infinity, "Alpha and Beta must be within valid bounds in alphaBeta.");

    context.debug_print_alphabeta_entry(depth, ply, alpha, beta, context.position.read().hash());
    
    // PVS searches everything off the principal variation with a null window, so a wider window means a PV node.
    // The root is always a PV node. Pruning and TT cutoffs are only safe at non-PV nodes.
    const bool isPV = beta - alpha > 1;

    // The incoming line's first move is searched first, iterative deepening hands the root its previous best move this
    // way. The line is cleared so every return below leaves an empty PV unless a move raises alpha.
    const PackedMove pvHint = pv->length > 0 ? pv->moves[0] : PackedMove::NullMove();
    pv->length = 0;

    PositionReader pos = context.position.read();
    // the incremental hash, used by the TT & repetition checks, has to match the position after every make, unmake &
    // null move. Assertion builds only, recomputing it is far too slow otherwise.
    ASSERT_MSG(pos.hash() == zobrist::computePositionHash(pos, us == Set::WHITE),
               "The incremental hash differs from the position's in alphaBeta.");

    // --- Draws ---
    // threefold repetition, the root & the game's earlier positions included, and the fifty move rule. Never at the
    // root, a search started in a drawn position still has to return a move. A mate given on the hundredth half move
    // would win but is scored a draw, rare enough not to generate moves for.
    if constexpr (kind == NodeKind::NORMAL) {
        if (context.repetitions.isRepetition(pos.hash(), context.frame(ply).halfmoveClock) || context.frame(ply).halfmoveClock >= 100)
            return -c_drawConstant;
    }

    // hard ply limit, frames & pv are sized by it and asserts are compiled out. Check extensions can otherwise keep
    // depth from ever reaching 0 along a chain of checks.
    if (ply >= c_maxSearchDepth - 1)
        return context.evaluate<us>();

    // Knows whether we're in check before generating anything, start() is called once ordering is primed below.
    using Moves = std::conditional_t<kind == NodeKind::RESTRICTED_ROOT, RootMoveSource<us>, SearchMoveSource<us>>;
    Moves moves = [&]() -> Moves {
        if constexpr (kind == NodeKind::RESTRICTED_ROOT)
            return Moves(pos, context.request.rootMoves);
        else
            return Moves(pos);
    }();

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
        std::optional<i16> ttProbeResult = search_policies::TT::probe(context.tt, pos.hash(), depth, ply, alpha, beta, flag, bestMove);

        // No cutoffs at PV nodes, the stored score may come from an earlier search or another path with different
        // repetitions. At the root it would also play the stored move without searching it. The TT move still
        // orders first below.
        if (!isPV && ttProbeResult.has_value()) {
            return ttProbeResult.value();
        }
    }

    // --- Internal Iterative Reduction ---
    // Without a TT or PV move the ordering is a guess and the node is likely new to the search, a ply shallower is
    // cheaper and stores a move for the next visit.
    if constexpr (search_policies::enabled_policies::IIR) {
        if (depth >= iir_params::minDepth && bestMove.isNull() && pvHint.isNull())
            depth--;
    }

    // --- Leaf Node Check ---
    // Before generating moves, most nodes are leaves and quiescence generates its own. In check the extension above
    // keeps depth > 0, so mates are still found by the full search; a stalemate at the horizon gets quiescence's
    // stand pat instead of a draw score.
    if (depth <= 0) {
        if constexpr (search_policies::QuiescencePolicy::enabled) {
            // Start Q-Search with its *own* depth limit, configured with search params.
            NoQuiescenceLine noLine;
            // the root never gets here, iterations start at depth 1 and IIR only takes a ply off deeper ones. A restricted
            // root's source isn't the type quiescence takes.
            if constexpr (kind == NodeKind::RESTRICTED_ROOT)
                return quiescence<us>(context, search_policies::QuiescencePolicy::maxDepth, alpha, beta, ply, noLine);
            else
                return quiescence<us>(context, search_policies::QuiescencePolicy::maxDepth, alpha, beta, ply, noLine, &moves);
        } else {
            return context.evaluate<us>();
        }
    }

    // --- No-Moves Check (Mate/Stalemate) ---
    MoveOrderingView orderingView;

    // --- prime move ordering ---
    if (bestMove.isNull() == false) orderingView.ttMove = bestMove;
    if (pvHint.isNull() == false) orderingView.pvMove = pvHint;
    search_policies::MoveOrdering::prime(context.heuristics, context.frames, orderingView, ply);

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
            return staticEval;
        }
    }

    // --- Null Move Pruning ---
    if constexpr (search_policies::NMP::enabled) {
        if (!isPV && !inCheck) {
            if (std::optional<i16> nullScore = tryNullMovePrune<us>(context, depth, beta, ply, staticEval)) {
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
    i16 eval = searchMoves<us>(moves, context, depth, alpha, beta, ply, pv, flag, bestMove, futile, orderingView);

    // aborted, the score is meaningless and must not reach the TT.
    if (context.stopped)
        return 0;

    // --- Store to TT ---
    if constexpr (search_policies::TT::enabled) {
        // All moves searched, no cutoff.
        search_policies::TT::update(context.tt,
            pos.hash(),
            bestMove, // Store the best move found
            eval, // Store the best score (which is alpha if it was a PV node)
            depth,
            ply,
            flag); // Flag is either TTF_CUT_ALPHA or TTF_CUT_EXACT
    }

    return eval;
}

#define INSTANTIATE_ALPHA_BETA(us, kind) \
    template i16 Worker::alphaBeta<us, kind>(ThreadContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv)
INSTANTIATE_ALPHA_BETA(Set::WHITE, NodeKind::NORMAL);
INSTANTIATE_ALPHA_BETA(Set::BLACK, NodeKind::NORMAL);
INSTANTIATE_ALPHA_BETA(Set::WHITE, NodeKind::ROOT);
INSTANTIATE_ALPHA_BETA(Set::BLACK, NodeKind::ROOT);
INSTANTIATE_ALPHA_BETA(Set::WHITE, NodeKind::RESTRICTED_ROOT);
INSTANTIATE_ALPHA_BETA(Set::BLACK, NodeKind::RESTRICTED_ROOT);
#undef INSTANTIATE_ALPHA_BETA

template<Set us, typename Moves>
i16 Worker::searchMoves(Moves& moves, ThreadContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv, TranspositionFlag& flag, PackedMove& outMove, bool futile, const MoveOrderingView& ordering) {
    // --- Main Search Loop ---
    PositionReader pos = context.position.read();
    // window at entry, alpha moves during the loop.
    const bool isPV = beta - alpha > 1;

    i16 bestEval = -c_infinity; // Start at -infinity
    PVLine childPv;
    u16 index = 0;

    MoveExecutor executor = context.executor();
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

        // butterfly & continuation history summed, used by history pruning and LMR. Only quiets have history, a quiet
        // move's piece is the same before and after it's made.
        const bool quiet = search_policies::MoveOrdering::isQuiet(move);
        const i32 history = quiet
            ? ordering.getHistoryScore(us, move.sourceSqr(), move.targetSqr())
                + ordering.getContinuationScore(us, pos.pieceAt(move.sourceSqr()).index(), move.targetSqr())
            : 0;

        // --- History Pruning ---
        // skipped quiets weren't searched, they get no history malus either.
        if constexpr (search_policies::HistoryPruning::enabled) {
            if (search_policies::HistoryPruning::skipMove(isPV, moves.isChecked(), depth, bestEval, move, ordered.isCheck(), history)) {
                ordered = moves.next();
                index++;
                continue;
            }
        }

        // check extensions happen in the child, at alphaBeta entry when it's in check.
        u8 adjustedDepth = depth;

        // makeMove advances the fifty move counter, reset by pawn moves & captures.
        u16 halfmoveClock = context.frame(ply).halfmoveClock;
        MoveUndoUnit undoState;
        executor.makeMove(move, undoState, halfmoveClock);
        context.frame(ply + 1).halfmoveClock = halfmoveClock;
        if constexpr (search_policies::TT::enabled)
            search_policies::TT::prefetch(context.tt, pos.hash());
        context.repetitions.push(pos.hash());

        // the move is made, the piece stands on its target.
        const ChessPiece movedPiece = pos.pieceAt(move.targetSqr());

        if constexpr (search_policies::ContinuationHistoryPolicy::enabled) {
            auto historyPtr = search_policies::ContinuationHistoryPolicy::slot(context.heuristics, movedPiece, move.targetSqr());
            context.frame(ply).continuation = historyPtr;
        }
        
        i16 eval;

        // the previous sibling's line isn't this move's, the child starts without one.
        childPv.length = 0;

        // update depth
        i32 nextDepth = static_cast<i32>(adjustedDepth) - 1;
        adjustedDepth = static_cast<u8>(std::max(nextDepth, 0));

        if (index == 0 && depth > 0) {
            // --- PV Search (First Move) ---
            // Full window, full trust.
            eval = static_cast<i16>(-alphaBeta<opposing_set<us>()>(context, adjustedDepth, static_cast<i16>(-beta),
                static_cast<i16>(-alpha), ply + 1, &childPv));
        } 
        else {
            // --- Scout Search (Subsequent Moves) ---
            // Zero window: Try to prove move is <= alpha            
            context.debug_print_scout_search();

            // --- Late Move Reduction ---
            // Quiet moves late in the ordering rarely matter, scout them shallower first.
            u8 reduction = 0;
            if constexpr (search_policies::LMR::enabled) {
                reduction = search_policies::LMR::reduction(depth, index, isPV, quiet, moves.isChecked(), ordered.isCheck(), history);
            }

            eval = static_cast<i16>(-alphaBeta<opposing_set<us>()>(context, static_cast<u8>(adjustedDepth - reduction),
                static_cast<i16>(-alpha - 1), static_cast<i16>(-alpha), ply + 1, &childPv));

            // the reduced search beat alpha, check it at full depth before trusting it.
            if (reduction > 0 && eval > alpha && !context.stopped)
                eval = static_cast<i16>(-alphaBeta<opposing_set<us>()>(context, adjustedDepth, static_cast<i16>(-alpha - 1),
                    static_cast<i16>(-alpha), ply + 1, &childPv));
            
            // --- The Re-Search Trigger ---
            // If eval > alpha, the move is better than we thought. 
            // We must re-search with the full window to get the exact score.
            // (Only if it's also < beta, otherwise we just take the beta cutoff)            
            if (eval > alpha && eval < beta) {
                context.debug_print_scout_re_search();
                eval = static_cast<i16>(-alphaBeta<opposing_set<us>()>(context, adjustedDepth, static_cast<i16>(-beta),
                    static_cast<i16>(-alpha), ply + 1, &childPv));
                
            }
        }

        context.repetitions.pop();
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
                if (search_policies::MoveOrdering::isQuiet(move)) {
                    search_policies::MoveOrdering::updateQuietCutoff(context.heuristics, us, move,
                        std::span<const PackedMove>(quietsTried.data(), quietCount), depth, context.frame(ply));

                    if constexpr (search_policies::ContinuationHistoryPolicy::enabled) {
                        using search_policies::ContinuationHistoryPolicy;
                        const i32 bonus = search_policies::MoveOrdering::quietBonus(depth);
                        ContinuationHistoryPolicy::update(context.frames, ply, pos.pieceAt(move.sourceSqr()),
                            move.targetSqr(), bonus);

                        for (u16 i = 0; i < quietCount; ++i)
                            ContinuationHistoryPolicy::update(context.frames, ply,
                                pos.pieceAt(quietsTried[i].sourceSqr()), quietsTried[i].targetSqr(), -bonus);
                    }
                }

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

template i16 Worker::searchMoves<Set::WHITE, SearchMoveSource<Set::WHITE>>(SearchMoveSource<Set::WHITE>& moves, ThreadContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv, TranspositionFlag& flag, PackedMove& outMove, bool futile, const MoveOrderingView& ordering);
template i16 Worker::searchMoves<Set::WHITE, RootMoveSource<Set::WHITE>>(RootMoveSource<Set::WHITE>& moves, ThreadContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv, TranspositionFlag& flag, PackedMove& outMove, bool futile, const MoveOrderingView& ordering);
template i16 Worker::searchMoves<Set::BLACK, SearchMoveSource<Set::BLACK>>(SearchMoveSource<Set::BLACK>& moves, ThreadContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv, TranspositionFlag& flag, PackedMove& outMove, bool futile, const MoveOrderingView& ordering);
template i16 Worker::searchMoves<Set::BLACK, RootMoveSource<Set::BLACK>>(RootMoveSource<Set::BLACK>& moves, ThreadContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv, TranspositionFlag& flag, PackedMove& outMove, bool futile, const MoveOrderingView& ordering);