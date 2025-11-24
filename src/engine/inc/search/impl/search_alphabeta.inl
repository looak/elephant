#pragma once

template<Set us>
i16 Search::alphaBeta(ThreadSearchContext& context, u16 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv) {
    THROW_EXPR(depth >= 0, ephant::search_exception, "Depth cannot be negative in alphaBeta.");   
    
    PositionReader pos = context.position.read();
    if (context.history.isRepetition(pos.hash()) == true) {
        return -c_drawConstant;
    }

    pv->length = 0;
    PackedMove bestMove = PackedMove::NullMove();

    // --- Transposition Table Probe ---
    TranspositionFlag flag = TranspositionFlag::TTF_NONE;
    if constexpr (search_policies::TT::enabled) {
        std::optional<i16> ttProbeResult = search_policies::TT::probe(pos.hash(), depth, alpha, beta, ply, flag, bestMove);
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
            int perspective = 1 - (int)us * 2;
            Evaluator evaluator(context.position.read());
            return evaluator.Evaluate() * perspective;
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
            ply,
            depth,
            flag); // Flag is either TTF_CUT_ALPHA or TTF_CUT_EXACT
    }
    return eval;
}

template<Set us>
i16 Search::searchMoves(MoveGenerator<us>& gen, ThreadSearchContext& context, u16 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv, TranspositionFlag& flag, PackedMove& outMove)
{
    // --- Main Search Loop ---
    PositionReader pos = context.position.read();

    i16 bestEval = -c_infinity; // Start at -infinity
    PVLine childPv;
    u16 index = 0;

    MoveExecutor executor(context.position.edit());
    PrioritizedMove ordered = gen.pop();
    
    do {
        if (context.clock.shouldStop() == true) 
            break;

        PackedMove move = ordered.move;

        u16 adjustedDepth = depth;
        // --- Checking Search Extensions ---
        adjustedDepth += static_cast<u8>(ordered.isCheck()); // Extend by 1 if the move gives check
        u16 movingPly = ply; // this will become important once we start caring about 50-move rule.
        i16 eval = bestEval;

        MoveUndoUnit undoState;
        executor.makeMove(move, undoState, movingPly);
        context.history.push(pos.hash());
        
        if (index == 0) {
            // First move: full window, full depth (no PVS, no LMR)
            eval = -alphaBeta<opposing_set<us>()>(context, adjustedDepth - 1, -beta, -alpha, ply + 1, &childPv);
    
        } else {
            // All other moves: start with zero-window (PVS)
            u16 searchDepth = adjustedDepth - 1;
            
            // Apply LMR if appropriate
            // if constexpr (search_policies::LMR::enabled) {
            //     if (search_policies::LMR::shouldReduce(depth, move, index, gen.isChecked(), ordered.isCheck())) {
            //         searchDepth -= search_policies::LMR::getReduction(depth);
            //     }
            // }
            
            // --- Scouting with zero-window ---
            this->scout_search_count++;
            eval = -alphaBeta<opposing_set<us>()>(context, searchDepth, -alpha - 1, -alpha, ply + 1, &childPv);
            
            // --- Scouting failed high: Re-search with full window ---
            if (eval > alpha && eval < beta) {
                this->scout_re_search_count++;
                eval = -alphaBeta<opposing_set<us>()>(context, searchDepth, -beta, -alpha, ply + 1, &childPv);
            }
        }
        
        context.history.pop();
        executor.unmakeMove(undoState);
        context.nodeCount++;

        // --- Alpha-Beta Logic (Fail-Soft) ---
        if (eval > bestEval) {
            bestEval = eval;
            outMove = move;

            if (bestEval > alpha) {
                alpha = bestEval;
                flag = TranspositionFlag::TTF_CUT_EXACT; // This is now a PV-Node

                // Update the Principal Variation
                pv->moves[0] = outMove;
                memcpy(pv->moves + 1, childPv.moves, childPv.length * sizeof(PackedMove));
                pv->length = childPv.length + 1;

                // --- Beta Cutoff ---
                if (alpha >= beta) {
                    flag = TranspositionFlag::TTF_CUT_BETA; // It's a fail-high
                    search_policies::MoveOrdering::push(context.moveOrdering.killers, move, ply);
                    return bestEval; 
                }
            }
        }

        ordered = gen.pop();
        index++;
    } while (ordered.move.isNull() == false);

    return bestEval;
}