#pragma once

template<Set us>
u16 Search::mostValuablePieceInPosition(PositionReader pos) {
    const auto& material = pos.material();
    for (u8 pieceIndx = queenId; pieceIndx >= pawnId; --pieceIndx) {
        if (material.read<us>(pieceIndx).empty() == false) {
            return piece_constants::value[pieceIndx];
        }
    }
    return 0; // no pieces found
}

template<Set us>
i16 Search::quiescence(ThreadSearchContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, bool checked) {
    ASSERT_MSG(depth >= 0, "Depth cannot be negative in alphaBeta.");
    ASSERT_MSG(ply < c_maxSearchDepth, "Ply exceeds maximum search depth in alphaBeta.");    
    ASSERT_MSG(alpha >= -c_infinity && beta <= c_infinity, "Alpha and Beta must be within valid bounds in alphaBeta.");

    Evaluator evaluator(context.position.read());
    i16 perspective = 1 - (int)us * 2;
    i16 standPat = evaluator.Evaluate() * perspective;

    // Stand-pat beta cutoff
    if (standPat >= beta && !checked)
        return standPat;

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
    MoveGenParams genParams = MoveGenParams{ .moveFilter = checked ? MoveTypes::ALL : MoveTypes::CAPTURES_ONLY };
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
        
        i16 qEval = -quiescence<opposing_set<us>()>(context, depth - 1, -beta, -alpha, ply + 1, ordered.isCheck());
        context.qNodeCount++;
        
        executor.unmakeMove(undoState);

        if (qEval > bestEval) 
            bestEval = qEval;

        if (bestEval >= beta)
            return bestEval;
        
        if (bestEval > alpha)
            alpha = bestEval;

        ordered = generator.pop();
    }

    return bestEval;
}