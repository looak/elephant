#pragma once

template<Set us>
i16 Search::alphaBeta(ThreadSearchContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv) {
    ASSERT_MSG(depth >= 0, "Depth cannot be negative in alphaBeta.");
    ASSERT_MSG(ply < c_maxSearchDepth, "Ply exceeds maximum search depth in alphaBeta.");
    ASSERT_MSG(alpha >= -c_infinity && beta <= c_infinity, "Alpha and Beta must be within valid bounds in alphaBeta.");
    
    PositionReader pos = context.position.read();
    if (context.history.isRepetition(pos.hash()) == true) {
        return -c_drawConstant;
    }

    PackedMove bestMove = PackedMove::NullMove();

    // --- Transposition Table Probe ---
    TranspositionFlag flag = TranspositionFlag::TTF_NONE;
    if constexpr (search_policies::TT::enabled) {
        std::optional<i16> ttProbeResult = search_policies::TT::probe(pos.hash(), depth, alpha, beta, flag, bestMove);
        if (ttProbeResult.has_value()) {
            if (flag == TranspositionFlag::TTF_CUT_EXACT) {
                pv->moves[0] = bestMove;
                pv->length = 1;
                return ttProbeResult.value();
            }
            else if (flag == TranspositionFlag::TTF_CUT_BETA && ttProbeResult.value() >= beta) {
                return ttProbeResult.value();
            }
            else if (flag == TranspositionFlag::TTF_CUT_ALPHA && ttProbeResult.value() <= alpha) {
                return alpha;
            }
        }
    }

    // --- No-Moves Check (Mate/Stalemate) ---
    MoveGenParams genParams;
    MoveOrderingView orderingView;

    // --- prime move ordering ---    
    if (bestMove.isNull() == false) orderingView.ttMove = bestMove;
    if (pv->length > 0) orderingView.pvMove = pv->moves[0];
    search_policies::MoveOrdering::prime(context.moveOrdering.killers, orderingView, ply);
    genParams.ordering = &orderingView;

    MoveGenerator<us> generator(pos, genParams);   

    // --- Terminal Node ---
    if (generator.peek().isNull()) {
        if (generator.isChecked())
            return -c_checkmateConstant + (i16)ply; // Mate score adjusted by ply
        return -c_drawConstant; // Stalemate
    }

    // --- Leaf Node Check ---
    if (depth <= 0) {
        pv->length = 0;
        if constexpr (search_policies::QuiescencePolicy::enabled) {
            // Start Q-Search with its *own* depth limit, configured with search params.
            return quiescence<us>(context, search_policies::QuiescencePolicy::maxDepth, alpha, beta, ply, generator.isChecked());
        } else {
            pv->length = 0;
            constexpr i32 sidesPerspective = (us == Set::WHITE) ? 1 : -1;
            Evaluator evaluator(context.position.read());
            return evaluator.Evaluate() * sidesPerspective;
        }
    }

    // --- Null Move Pruning ---
    if constexpr (search_policies::NMP::enabled) {
        if (generator.isChecked() == false && tryNullMovePrune<us>(context, depth, alpha, beta, ply)) {
            return beta;
        }
    }

    // --- Main Search Loop ---    
    flag = TranspositionFlag::TTF_CUT_ALPHA; // Assume we'll fail-low
    i16 eval = searchMoves<us>(generator, context, depth, alpha, beta, ply, pv, flag, bestMove);

    // --- Store to TT ---
    if constexpr (search_policies::TT::enabled) {
        // All moves searched, no cutoff.
        search_policies::TT::update(
            pos.hash(),
            bestMove, // Store the best move found
            eval, // Store the best score (which is alpha if it was a PV node)
            depth,
            flag); // Flag is either TTF_CUT_ALPHA or TTF_CUT_EXACT
    }
    return eval;
}

template<Set us>
i16 Search::searchMoves(MoveGenerator<us>& gen, ThreadSearchContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv, TranspositionFlag& flag, PackedMove& outMove) {
    // --- Main Search Loop ---
    PositionReader pos = context.position.read();

    i16 bestEval = -c_infinity; // Start at -infinity
    PVLine childPv;
    u16 index = 0;

    MoveExecutor executor(context.position.edit());
    PrioritizedMove ordered = gen.pop();
    
    // We need to store the "Best Move Found So Far" locally to update outMove correctly
    PackedMove intermmediateMove = PackedMove::NullMove();

    u16 movingPly = ply; 

    do {
        if (context.clock.shouldStop()) break;

        PackedMove move = ordered.move;
        
        // --- Extensions ---
        u16 adjustedDepth = depth + static_cast<u8>(ordered.isCheck());
        
        MoveUndoUnit undoState;
        executor.makeMove(move, undoState, movingPly);
        context.history.push(pos.hash());
        
        i16 eval;

        if (index == 0) {
            // --- PV Search (First Move) ---
            // Full window, full trust.
            eval = -alphaBeta<opposing_set<us>()>(context, adjustedDepth - 1, -beta, -alpha, ply + 1, &childPv);
        } else {
            // --- Scout Search (Subsequent Moves) ---
            // Zero window: Try to prove move is <= alpha
            this->scout_search_count++;
            eval = -alphaBeta<opposing_set<us>()>(context, adjustedDepth - 1, -alpha - 1, -alpha, ply + 1, &childPv);
            
            // --- The Re-Search Trigger ---
            // If eval > alpha, the move is better than we thought. 
            // We must re-search with the full window to get the exact score.
            // (Only if it's also < beta, otherwise we just take the beta cutoff)
            if (eval > alpha && eval < beta) {
                this->scout_re_search_count++;
                eval = -alphaBeta<opposing_set<us>()>(context, adjustedDepth - 1, -beta, -alpha, ply + 1, &childPv);
            }
        }

        context.history.pop();
        executor.unmakeMove(undoState);
        context.nodeCount++;

        // --- Update Best Score (Fail-Soft) ---
        if (eval > bestEval) {
            bestEval = eval;
            intermmediateMove = move; // Update local tracker
            
            // --- Beta Cutoff (Fail-High) ---
            if (bestEval >= beta) {
                flag = TranspositionFlag::TTF_CUT_BETA;
                search_policies::MoveOrdering::push(context.moveOrdering.killers, move, ply);
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

        ordered = gen.pop();
        index++;
    } while (ordered.move.isNull() == false);

    // Ensure outMove is set if we found *any* valid move that improved bestEval (even if it didn't beat alpha)
    // Though usually, we only care about outMove if it beat alpha (PV) or beta (Cutoff).
    if (intermmediateMove.isNull() == false && outMove.isNull()) {
        outMove = intermmediateMove;
    }

    return bestEval;
}