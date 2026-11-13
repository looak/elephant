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
i16 Worker::alphaBeta(ThreadContext& context, NodeInputs node, PVLine* pv) {
    ASSERT_MSG(node.depth >= 0, "Depth cannot be negative in alphaBeta.");
    ASSERT_MSG(node.ply < c_maxSearchDepth, "Ply exceeds maximum search depth in alphaBeta.");
    ASSERT_MSG(node.alpha >= -c_infinity && node.beta <= c_infinity, "Alpha and Beta must be within valid bounds in alphaBeta.");
    
    // the incremental hash, used by the TT & repetition checks, has to match the position after every make, unmake &
    // null move. Assertion builds only, recomputing it is far too slow otherwise.
    PositionReader pos = context.position.read();
    ASSERT_MSG(pos.hash() == zobrist::computePositionHash(pos, us == Set::WHITE), "The incremental hash differs from the position's in alphaBeta.");
    context.debug_print_alphabeta_entry(node.depth, node.ply, node.alpha, node.beta, pos.hash());

    PlyFrame& frame = context.frame(node.ply);
    
    // PVS searches everything off the principal variation with a null window, so a wider window means a PV node.
    // The root is always a PV node. Pruning and TT cutoffs are only safe at non-PV nodes.
    const bool isPV = node.beta - node.alpha > 1;

    // The incoming line's first move is searched first, iterative deepening hands the root its previous best move this
    // way. The line is cleared so every return below leaves an empty PV unless a move raises alpha.
    const PackedMove pvHint = pv->length > 0 ? pv->moves[0] : PackedMove::NullMove();
    pv->length = 0;

    // --- Draws ---
    // threefold repetition, the root & the game's earlier positions included, and the fifty move rule. Never at the
    // root, a search started in a drawn position still has to return a move. A mate given on the hundredth half move
    // would win but is scored a draw, rare enough not to generate moves for.
    if constexpr (kind == NodeKind::NORMAL) {
        if (context.repetitions.isRepetition(pos.hash(), frame.halfmoveClock) || frame.halfmoveClock >= 100)
            return -c_drawConstant;
    }

    // hard ply limit, frames & pv are sized by it and asserts are compiled out. Check extensions can otherwise keep
    // depth from ever reaching 0 along a chain of checks.
    if (node.ply >= c_maxSearchDepth - 1)
        return context.evaluate<us>();

    tusk::CheckInfo<us> chkInfo(pos);

    // Knows whether we're in check before generating anything, start() is called once ordering is primed below.
    using Moves = std::conditional_t<kind == NodeKind::RESTRICTED_ROOT, RootMoveSource<us>, SearchMoveSource<us>>;
    Moves moves = [&]() -> Moves {
        if constexpr (kind == NodeKind::RESTRICTED_ROOT)
            return Moves(pos, context.request.rootMoves, chkInfo);
        else
            return Moves(pos, chkInfo);
    }();

    // --- Check Extension ---
    // Search one ply deeper when in check so forcing sequences aren't cut off at the horizon. Covers direct and
    // discovered checks alike. Done before the TT probe so stored and probed depths agree.
    if (moves.isChecked())
        node.depth++;

    PackedMove ttMove = PackedMove::NullMove();
    
    // check if we're doing a validation search.
    bool notExcludedAllowCut = frame.excludedMove.isNull();

    // --- Transposition Table Probe ---    
    std::optional<search_policies::TT::ProbeResult> ttProbeResult = std::nullopt;
    if constexpr (search_policies::TT::enabled) {                
        ttProbeResult = search_policies::TT::probe(context.tt, pos.hash(), node.ply);
        
        if (ttProbeResult.has_value()) {
            // any hit, also when the entry is too shallow to cut.
            ttMove = ttProbeResult.value().move;

            // No cutoffs at PV nodes or at validation search.
            if (!isPV && notExcludedAllowCut && search_policies::TT::cuts(ttProbeResult.value(), node.depth, node.alpha, node.beta)) {
                return ttProbeResult.value().score;
            }
        }
    }

    // --- Internal Iterative Reduction ---
    // Without a TT or PV move the ordering is a guess and the node is likely new to the search, a ply shallower is
    // cheaper and stores a move for the next visit.
    if constexpr (search_policies::enabled_policies::IIR) {
        if (node.depth >= iir_params::minDepth && ttMove.isNull() && pvHint.isNull())
            node.depth--;
    }

    // --- Leaf Node Check ---
    // Before generating moves, most nodes are leaves and quiescence generates its own. In check the extension above
    // keeps depth > 0, so mates are still found by the full search; a stalemate at the horizon gets quiescence's
    // stand pat instead of a draw score.
    if (node.depth <= 0) {
        if constexpr (search_policies::QuiescencePolicy::enabled) {
            // Start Q-Search with its *own* depth limit, configured with search params.
            NoQuiescenceLine noLine;
            // the root never gets here, iterations start at depth 1 and IIR only takes a ply off deeper ones. A restricted
            // root's source isn't the type quiescence takes.
            if constexpr (kind == NodeKind::RESTRICTED_ROOT)
                return quiescence<us>(context, search_policies::QuiescencePolicy::maxDepth, node.alpha, node.beta, node.ply, noLine);
            else
                return quiescence<us>(context, search_policies::QuiescencePolicy::maxDepth, node.alpha, node.beta, node.ply, noLine, &moves);
        } else {
            return context.evaluate<us>();
        }
    }

    // --- Singular Extension ---
    // the TT move with a deep enough lower bound: if every other move fails low against a window a margin below its
    // score, at about half depth, it's the only good move here and is searched a ply deeper.
    NodePlan plan{};
    if constexpr (search_policies::SingularExtensions::enabled && kind == NodeKind::NORMAL) {
        if (ttProbeResult.has_value() && notExcludedAllowCut && search_policies::SingularExtensions::tryNode(*ttProbeResult, node.depth)) {
            const i16 singularBeta = static_cast<i16>(ttProbeResult->score - singular_params::marginPerDepth * node.depth);
            const u8 singularDepth = static_cast<u8>((node.depth - 1) / 2);

            frame.excludedMove = ttProbeResult->move;
            PVLine ignoredPv;
            const i16 score = alphaBeta<us>(context, NodeInputs{ static_cast<i16>(singularBeta - 1), singularBeta, singularDepth, node.ply }, &ignoredPv);
            frame.excludedMove = PackedMove::NullMove();

            if (score < singularBeta)
                plan.singularExtension = 1;
        }
    }

    // --- prime move ordering ---    
    if (ttMove.isNull() == false) plan.orderingView.ttMove = ttMove;
    if (pvHint.isNull() == false) plan.orderingView.pvMove = pvHint;
    search_policies::MoveOrdering::prime(context.heuristics, context.frames, plan.orderingView, node.ply);

    moves.beginGeneration(&plan.orderingView, MoveTypes::ALL);

    // --- Terminal Node ---
    if (moves.peek().isNull()) {
        if (moves.isChecked()){
            ASSERT(node.ply < c_checkmateMaxDistance);
            return -c_checkmateConstant + (i16)node.ply; // Mate score adjusted by ply
        }
        return -c_drawConstant; // Stalemate
    }

    // --- Static Eval ---
    // Only the pruning at non-PV nodes uses it, and it means nothing in check.    
    const i16 staticEval = (isPV || moves.isChecked()) ? -c_infinity : context.evaluate<us>();

    // --- Reverse Futility Pruning ---
    if constexpr (search_policies::RFP::enabled) {
        if (search_policies::RFP::shouldPrune(isPV, moves.isChecked(), node.depth, staticEval, node.beta) && notExcludedAllowCut) {
            return staticEval;
        }
    }

    // --- Null Move Pruning ---
    if constexpr (search_policies::NMP::enabled) {
        if (isPV == false && moves.isChecked() == false && notExcludedAllowCut) {
            if (std::optional<i16> nullScore = tryNullMovePrune<us>(context, node, staticEval)) {
                return nullScore.value();
            }
        }
    }

    // --- Futility Pruning ---
    // decided per node, searchMoves skips the quiet moves.
    plan.futile = false;
    if constexpr (search_policies::Futility::enabled)
        plan.futile = search_policies::Futility::isNodeFutile(isPV, moves.isChecked(), node.depth, staticEval, node.alpha);

    // --- Main Search Loop ---    
    NodeResult result = searchMoves<us>(moves, context, node, plan, pv);

    // aborted, the score is meaningless and must not reach the TT.
    if (context.stopped)
        return 0;

    // --- Store to TT ---
    if constexpr (search_policies::TT::enabled) {
        if (notExcludedAllowCut) {
            // All moves searched, no cutoff.
            search_policies::TT::update(context.tt,
                pos.hash(),
                result.bestMove, // Store the best move found
                result.score, // Store the best score (which is alpha if it was a PV node)
                node.depth,
                node.ply,
                result.flag); // Flag is either TTF_CUT_ALPHA or TTF_CUT_EXACT
        }
    }

    return result.score;
}

#define INSTANTIATE_ALPHA_BETA(us, kind) \
    template i16 Worker::alphaBeta<us, kind>(ThreadContext&, NodeInputs, PVLine*)
INSTANTIATE_ALPHA_BETA(Set::WHITE, NodeKind::NORMAL);
INSTANTIATE_ALPHA_BETA(Set::BLACK, NodeKind::NORMAL);
INSTANTIATE_ALPHA_BETA(Set::WHITE, NodeKind::ROOT);
INSTANTIATE_ALPHA_BETA(Set::BLACK, NodeKind::ROOT);
INSTANTIATE_ALPHA_BETA(Set::WHITE, NodeKind::RESTRICTED_ROOT);
INSTANTIATE_ALPHA_BETA(Set::BLACK, NodeKind::RESTRICTED_ROOT);
#undef INSTANTIATE_ALPHA_BETA

template<Set us, typename Moves>
Worker::NodeResult Worker::searchMoves(Moves& moves, ThreadContext& context, NodeInputs node, const NodePlan& plan, PVLine* pv) {
    // --- Main Search Loop ---
    PositionReader pos = context.position.read();
    // window at entry, alpha moves during the loop.
    const bool isPV = node.beta - node.alpha > 1;
    
    PVLine childPv;
    u16 index = 0;

    MoveExecutor executor = context.makeExecutor();
    PrioritizedMove ordered = moves.next();
    
    // intermmediate: the highest scoring move so far and its score, fail-soft, whether or not it beat alpha.
    PackedMove intermmediateMove = PackedMove::NullMove();
    i16 intermmediateScore = -c_infinity;
    // settled: what the node reports for the TT. Until a move beats alpha it's a fail-low that keeps the TT move.
    PackedMove settledMove = plan.orderingView.ttMove;
    TranspositionFlag settledFlag = TranspositionFlag::TTF_CUT_ALPHA;

    // quiets searched without failing high, they get a history malus if a later quiet does. Beyond the buffer they're
    // simply not penalised.
    std::array<PackedMove, 64> quietsTried;
    u16 quietCount = 0;

    // --- Late Move Pruning ---
    // at shallow non-PV nodes only the first quiets are searched, the rest rarely matter.
    u16 quietLimit = 0;
    u16 quietsSearched = 0;
    if constexpr (search_policies::LMP::enabled)
        quietLimit = search_policies::LMP::quietLimit(isPV, moves.isChecked(), node.depth);

    do {
        if (context.shouldStop()) break;

        PackedMove move = ordered.move;

        // -- Singular Extension -- // --- Futility Pruning --- // --- Late Move Pruning ---
        // skipped quiets weren't searched, they get no history malus either.
        if (search_policies::SingularExtensions::shouldSkipMove(context.frame(node.ply), move)
        || search_policies::Futility::shouldSkipMove(plan.futile, index, move, ordered.givesCheck())
        || search_policies::LMP::shouldSkipMove(quietLimit, quietsSearched, intermmediateScore, move, ordered.givesCheck())) {
            ordered = moves.next();
            index++;
            continue;
        }

        // check extensions happen in the child, at alphaBeta entry when it's in check.
        u8 adjustedDepth = node.depth;
        if (move == plan.orderingView.ttMove)
            adjustedDepth += plan.singularExtension;

        // makeMove advances the fifty move counter, reset by pawn moves & captures.
        u16 halfmoveClock = context.frame(node.ply).halfmoveClock;
        MoveUndoUnit undoState;
        executor.makeMove(move, undoState, halfmoveClock);
        context.frame(node.ply + 1).halfmoveClock = halfmoveClock;
        if constexpr (search_policies::TT::enabled)
            search_policies::TT::prefetch(context.tt, pos.hash());
        context.repetitions.push(pos.hash());

        // the move is made, the piece stands on its target.
        const ChessPiece movedPiece = pos.pieceAt(move.targetSqr());

        if constexpr (search_policies::ContinuationHistoryPolicy::enabled)
            search_policies::ContinuationHistoryPolicy::recordContinuation(context.frame(node.ply), context.heuristics, movedPiece, move.targetSqr());

        i16 eval;

        // the previous sibling's line isn't this move's, the child starts without one.
        childPv.length = 0;

        // update depth
        i32 nextDepth = static_cast<i32>(adjustedDepth) - 1;
        adjustedDepth = static_cast<u8>(std::max(nextDepth, 0));

        if (index == 0 && node.depth > 0) {
            // --- PV Search (First Move) ---
            // Full window, full trust.
            eval = static_cast<i16>(-alphaBeta<opposing_set<us>()>(context, node.makeChildInputs(adjustedDepth), &childPv));
        } 
        else {
            // --- Scout Search (Subsequent Moves) ---
            // Zero window: Try to prove move is <= alpha            
            context.debug_print_scout_search();

            // --- Late Move Reduction ---
            // Quiet moves late in the ordering rarely matter, scout them shallower first.
            u8 reduction = 0;
            if constexpr (search_policies::LMR::enabled) {
                const bool quiet = search_policies::MoveOrdering::isQuiet(move);
                const i32 history = quiet
                    ? plan.orderingView.getHistoryScore(us, move.sourceSqr(), move.targetSqr())
                        + plan.orderingView.getContinuationScore(us, movedPiece.index(), move.targetSqr())
                    : 0;
                reduction = search_policies::LMR::reduction(node.depth, index, isPV, quiet, moves.isChecked(), ordered.givesCheck(), history);
            }

            eval = static_cast<i16>(-alphaBeta<opposing_set<us>()>(context, node.makeScoutInputs(static_cast<u8>(adjustedDepth - reduction)), &childPv));

            // the reduced search beat alpha, check it at full depth before trusting it.
            if (reduction > 0 && eval > node.alpha && !context.stopped)
                eval = static_cast<i16>(-alphaBeta<opposing_set<us>()>(context, node.makeScoutInputs(adjustedDepth), &childPv));
            
            // --- The Re-Search Trigger ---
            // If eval > alpha, the move is better than we thought. 
            // We must re-search with the full window to get the exact score.
            // (Only if it's also < beta, otherwise we just take the beta cutoff)            
            if (eval > node.alpha && eval < node.beta) {
                context.debug_print_scout_re_search();
                eval = static_cast<i16>(-alphaBeta<opposing_set<us>()>(context, node.makeChildInputs(adjustedDepth), &childPv));
                
            }
        }

        context.repetitions.pop();
        executor.unmakeMove(undoState);
        context.nodeCount++;

        // the child was aborted, its score is meaningless. Unwind without touching alpha, pv or killers.
        if (context.stopped) 
            return NodeResult{};

        context.debug_print_eval(move, eval, node.alpha, node.beta, node.depth, node.ply, pos.hash());

        // --- Update Best Score (Fail-Soft) ---
        if (eval > intermmediateScore) {
            intermmediateScore = eval;
            intermmediateMove = move;
            
            // --- Beta Cutoff (Fail-High) ---
            if (intermmediateScore >= node.beta) {
                if (search_policies::MoveOrdering::isQuiet(move)) {
                    search_policies::MoveOrdering::updateQuietCutoff(context.heuristics, us, move,
                        std::span<const PackedMove>(quietsTried.data(), quietCount), node.depth, context.frame(node.ply));

                    if constexpr (search_policies::ContinuationHistoryPolicy::enabled) {
                        using search_policies::ContinuationHistoryPolicy;
                        const i32 bonus = search_policies::MoveOrdering::quietBonus(node.depth);
                        ContinuationHistoryPolicy::update(context.frames, node.ply, pos.pieceAt(move.sourceSqr()),
                            move.targetSqr(), bonus);

                        for (u16 i = 0; i < quietCount; ++i)
                            ContinuationHistoryPolicy::update(context.frames, node.ply,
                                pos.pieceAt(quietsTried[i].sourceSqr()), quietsTried[i].targetSqr(), -bonus);
                    }
                }
                
                return { intermmediateScore, intermmediateMove, TranspositionFlag::TTF_CUT_BETA};
            }

            // --- Alpha Update (PV Node) ---
            if (intermmediateScore > node.alpha) {
                node.alpha = intermmediateScore;
                settledMove = intermmediateMove;
                settledFlag = TranspositionFlag::TTF_CUT_EXACT;

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

    if (context.stopped) {
        return NodeResult{};
    }

    // a fail-low without a TT move still stores a move, the highest scoring one.
    if (settledMove.isNull())
        settledMove = intermmediateMove;

    return { intermmediateScore, settledMove, settledFlag };
}

template Worker::NodeResult Worker::searchMoves<Set::WHITE, SearchMoveSource<Set::WHITE>>(SearchMoveSource<Set::WHITE>& moves, ThreadContext& context, NodeInputs, const NodePlan&, PVLine*);
template Worker::NodeResult Worker::searchMoves<Set::WHITE, RootMoveSource<Set::WHITE>>(RootMoveSource<Set::WHITE>& moves, ThreadContext& context, NodeInputs, const NodePlan&, PVLine*);
template Worker::NodeResult Worker::searchMoves<Set::BLACK, SearchMoveSource<Set::BLACK>>(SearchMoveSource<Set::BLACK>& moves, ThreadContext& context, NodeInputs, const NodePlan&, PVLine*);
template Worker::NodeResult Worker::searchMoves<Set::BLACK, RootMoveSource<Set::BLACK>>(RootMoveSource<Set::BLACK>& moves, ThreadContext& context, NodeInputs, const NodePlan&, PVLine*);