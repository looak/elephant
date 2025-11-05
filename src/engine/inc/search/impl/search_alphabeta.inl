#pragma once

template<Set us, typename config>
i16 Search::alphaBeta(ThreadSearchContext& context, u16 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv) {
    THROW_EXPR(depth >= 0, ephant::search_exception, "Depth cannot be negative in alphaBeta.");   

    PositionReader pos = context.position.read();
    pv->length = 0;

    // --- Transposition Table Probe ---    
    if (std::optional<i16> ttProbeResult = config::TT_Policy::probe(pos.hash(), depth, alpha, beta, ply))
        return ttProbeResult.value();

    // --- No-Moves Check (Mate/Stalemate) ---
    MoveGenParams genParams;
    MoveOrderingView orderingView;
    
    // if(pv->length > 0) {
    //     orderingView.pvMove = pv->moves[0];
    //     genParams.ordering = &orderingView;
    // }

    MoveGenerator<us> generator(pos, genParams);   

    // --- terminal node ---
    if (generator.peek().isNull()) {
        if (generator.isChecked())
            return -c_checkmateConstant + (i16)ply; // Mate score adjusted by ply
        return -c_drawConstant; // Stalemate
    }

    // --- Leaf Node Check ---
    if (depth <= 0) {
        pv->length = 0;
        if constexpr (config::QSearch_Policy::enabled) {
            // Start Q-Search with its *own* depth limit, configured with search params.
            return quiescence<us, config>(context, config::QSearch_Policy::maxDepth, alpha, beta, ply);
        } else {
            int perspective = 1 - (int)us * 2;            
            Evaluator evaluator(context.position.read());
            return evaluator.Evaluate() * perspective;
        }
    }

    // --- Null Move Pruning ---
    if constexpr (config::NMP_Policy::enabled) {
        if (generator.isChecked() == false && tryNullMovePrune<us, config>(context, depth, alpha, beta, ply)) {
            return beta;
        }
    }

    // --- Main Search Loop ---
    PackedMove bestMove = PackedMove::NullMove();
    auto flag = TranspositionFlag::TTF_CUT_ALPHA; // Assume we'll fail-low
    i16 eval = searchMoves<us, config>(generator, context, depth, alpha, beta, ply, pv, flag, bestMove);

    // --- 6. Store to TT ---
    // All moves searched, no cutoff.
    config::TT_Policy::update(        
        pos.hash(),
        bestMove, // Store the best move found
        context.gameHistory.age,
        eval, // Store the best score (which is alpha if it was a PV node)
        ply,
        depth,
        flag); // Flag is either TTF_CUT_ALPHA or TTF_CUT_EXACT

    return eval;
}


template<Set us, typename config>
i16 Search::searchMoves(MoveGenerator<us>& gen, ThreadSearchContext& context, u16 depth, i16 alpha, i16 beta, u16 ply, PVLine* pv, TranspositionFlag& flag, PackedMove& outMove)
{
    // --- Main Search Loop ---
    PositionReader pos = context.position.read();

    i16 bestEval = -c_infinity; // Start at -infinity
    PVLine childPv;
    u16 index = 0;

    MoveExecutor executor(context.position.edit());
    PackedMove move = gen.pop();
    
    do {
        u16 modifiedDepth = depth;
        // --- Late Move Reduction ---
        if constexpr (config::LMR_Policy::enabled) {
            if (config::LMR_Policy::shouldReduce(depth, move, index, gen.isChecked())) {
                modifiedDepth -= config::LMR_Policy::getReduction(depth);
            }
        }

        MoveUndoUnit undoState;
        u16 movingPly = ply; // this will become important once we start caring about 50-move rule.
        executor.makeMove(move, undoState, movingPly);

        i16 eval;        
        if (context.gameHistory.IsRepetition(pos.hash()) == true) {
            eval = -c_drawConstant;
        } else {
            eval = -alphaBeta<opposing_set<us>(), config>(context, modifiedDepth - 1, -beta, -alpha, ply + 1, &childPv);
        }

        if constexpr (config::LMR_Policy::enabled) {
            if (eval > alpha && modifiedDepth < depth) {
                // Re-search at full depth
                eval = -alphaBeta<opposing_set<us>(), config>(context, depth - 1, -beta, -alpha, ply + 1, &childPv);
            }
        }

        executor.unmakeMove(undoState);
        context.nodeCount++;

        // if (context.cancel()) return 0; // Handle search cancellation

        // --- 5. Alpha-Beta Logic (Fail-Soft) ---
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
            }

            // --- Beta Cutoff ---
            if (alpha >= beta) {
                flag = TranspositionFlag::TTF_CUT_BETA; // It's a fail-high                
                // (pushKillerMove logic would go here)                
                return bestEval; 
            }
        }

        move = gen.pop();
        index++;
    } while (move.isNull() == false);

    return bestEval;
}