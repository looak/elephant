#pragma once

template<Set us>
u16 Search::mostValuablePieceInPosition(PositionReader pos) {
    const auto& material = pos.material();
    for (i8 pieceIndx = queenId; pieceIndx >= pawnId; --pieceIndx) {
        if (material.read<us>(pieceIndx).empty() == false) {
            return piece_constants::value[pieceIndx];
        }
    }
    return 0; // no pieces found
}

template<Set us>
i16 Search::quiescence(ThreadSearchContext& context, u16 depth, i16 alpha, i16 beta, u16 ply) {
    Evaluator evaluator(context.position.read());
    i16 perspective = 1 - (int)us * 2;
    i16 standPat = evaluator.Evaluate() * perspective;

    // Stand-pat beta cutoff
    if (standPat >= beta)
        return beta;

    // Leaf node - return stand-pat
    if (depth <= 0 || ply >= c_maxSearchDepth)
        return standPat;

    // Futility pruning
    u16 mvValue = mostValuablePieceInPosition<opposing_set<us>()>(context.position.read());
    if (search_policies::QuiescencePolicy::futile(depth, standPat + mvValue, alpha))
        return standPat;

    // Update alpha
    if (standPat > alpha)
        alpha = standPat;

    // Generate captures
    MoveGenParams genParams = MoveGenParams{ .moveFilter = MoveTypes::CAPTURES_ONLY };
    MoveGenerator<us> generator(context.position.read(), genParams);

    i16 bestEval = standPat;
    PrioritizedMove ordered = generator.pop();
    
    while (!ordered.move.isNull()) {
        if (context.clock.shouldStop())
            break;

        PackedMove move = ordered.move;
        
        // Skip bad captures (SEE < 0)
        // if (SEE(move) < 0) {
        //     ordered = generator.pop();
        //     continue;
        // }

        MoveExecutor executor(context.position.edit());
        MoveUndoUnit undoState;
        executor.makeMove(move, undoState, ply);
        
        i16 qEval = -quiescence<opposing_set<us>()>(context, depth - 1, -beta, -alpha, ply + 1);
        context.qNodeCount++;
        
        executor.unmakeMove(undoState);

        if (qEval > bestEval) {            
            bestEval = qEval;
            if (qEval > alpha) {
                alpha = qEval;
                if (alpha >= beta)
                    return alpha; // Beta cutoff
            }
        }

        ordered = generator.pop();
    }

    return bestEval;
}