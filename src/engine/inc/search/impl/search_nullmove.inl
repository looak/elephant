#pragma once

template<Set us, typename Config>
bool Search::tryNullMovePrune(ThreadSearchContext& ctx, u16 depth, i16 alpha, i16 beta, u16 ply) {
    PositionReader pos = ctx.position.read();

    // Safety check: Don't prune near mate scores
    if (beta >= c_checkmateConstant - 100 || beta <= -c_checkmateConstant + 100) {
        return false;
    }
    
    // Check if we have any pieces besides the king and pawns - trying to identify zugzwang positions to avoid pruning them.
    const auto& mat = pos.material();
    Bitboard pieces = mat.knights<us>() | mat.bishops<us>() | mat.rooks<us>() | mat.queens<us>();
    
    if (!Config::NMP_Policy::shouldPrune(depth, false, !pieces.empty())) {
        return false;
    }

    // TODO: consider adding makeNullMove to move executor.
    u64 originalHash = ctx.position.read().hash();
    ctx.position.edit().hash() = zobrist::updateBlackToMoveHash(originalHash);

    u16 R = Config::NMP_Policy::getReduction(depth);
    i16 nullScore = -nullmove<opposing_set<us>(), Config>(ctx, depth - 1 - R, -beta, -beta + 1, ply + 1);

    ctx.nodeCount++;
    ctx.position.edit().hash() = originalHash;
    return (nullScore >= beta);
}

template<Set us, typename config>
i16 Search::nullmove(ThreadSearchContext& context, u16 depth, i16 alpha, i16 beta, u16 ply) {
    THROW_EXPR(depth >= 0, ephant::search_exception, "Depth cannot be negative in recursiveAlphaBetaNegamax.");   

    PositionReader currentPos = context.position.read();

    // --- No-Moves Check (Mate/Stalemate) ---
    MoveGenParams genParams;
    MoveGenerator<us> generator(currentPos, genParams);
    PrioritizedMove ordered = generator.pop();
    PackedMove move = ordered.move;

    if (move.isNull()) {
        if (generator.isChecked())
            return -c_checkmateConstant + c_nullMoveOffset;
        return -c_drawConstant; // Stalemate
    }

    // --- Leaf Node Check ---
    if (depth <= 0) {
        if constexpr (config::QSearch_Policy::enabled) {
            // Start Q-Search with its *own* depth limit, configured with search params.
            return quiescence<us, config>(context, config::QSearch_Policy::maxDepth, alpha, beta, ply);
        } else {
            Evaluator evaluator(context.position.read());
            i16 perspective = (us == Set::WHITE) ? 1 : -1;
            return evaluator.Evaluate() * perspective;
        }
    }

    // --- Main Search Loop ---
    i16 bestEval = -c_infinity; // Start at -infinity
    MoveExecutor executor(context.position.edit());
    u16 index = 0;

    do {
        if (context.clock.shouldStop() == true) 
            break;

        u16 modifiedDepth = depth;
        // --- Late Move Reduction if Enabled ---
        if constexpr (config::LMR_Policy::enabled) {
            if (config::LMR_Policy::shouldReduce(depth, move, index, generator.isChecked(), ordered.isCheck())) {
                modifiedDepth -= config::LMR_Policy::getReduction(depth);                
            }
        }

        MoveUndoUnit undoState;
        u16 movingPly = ply;
        executor.makeMove(move, undoState, movingPly);
        context.history.push(currentPos.hash());

        i16 eval;
        if (context.history.isRepetition(currentPos.hash()) == true) {
            eval = -c_drawConstant;
        } else {
            eval = -nullmove<opposing_set<us>(), config>(context, modifiedDepth - 1, -beta, -alpha, ply + 1);
        }

        // TODO: Evaluate if we need to re-search within the NullMoveReduction because of LMR.

        context.history.pop();
        executor.unmakeMove(undoState);
        context.nodeCount++;

        // --- Alpha-Beta Evaluation ---
        if (eval > bestEval) {
            bestEval = eval;            

            if (bestEval > alpha) 
                alpha = bestEval;

            if (alpha >= beta)                
                return bestEval;
        }

        ordered = generator.pop();
        move = ordered.move;
        index++;
    } while (move.isNull() == false);

    return bestEval;
}