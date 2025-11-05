#pragma once

template<Set us, typename config>
i16 Search::quiescence(ThreadSearchContext& context, u16 depth, i16 alpha, i16 beta, u16 ply) {
    MoveGenParams genParams = MoveGenParams{ .moveFilter = MoveTypes::CAPTURES_ONLY };
    MoveGenerator<us> generator(context.position.read(), genParams);
    Evaluator evaluator(context.position.read());

    i16 perspective = 1 - (int)us * 2; // WHITE = 1, BLACK = -1
    i16 eval = evaluator.Evaluate() * perspective;

    if (eval >= beta)
        return beta;
    
    constexpr i16 c_evalMargin = 100; // 1 pawn margin
    
    // margin is too large, cut-off
    i16 margin = eval + c_evalMargin + piece_constants::value[queenId];
    if (alpha > margin)
        return alpha; // even the best capture can not raise eval above alpha.

    if (eval > alpha)
        alpha = eval;

    PackedMove move = generator.pop();

    // context.cancel() == true
    // && generator.isChecked() == false // continue as long as we're in check?

    if (move.isNull() || ply >= c_maxSearchDepth || (depth <= 0))
        return eval;

    i16 maxEval = -c_infinity;
    do {
        MoveExecutor executor(context.position.edit());
        MoveUndoUnit undoState;
        executor.makeMove(move, undoState, ply);
        i16 eval = -quiescence<opposing_set<us>(), config>(context, depth - 1, -beta, -alpha, ply + 1);        
        context.qNodeCount++;
        executor.unmakeMove(undoState);

        maxEval = std::max(maxEval, eval);
        alpha = std::max(alpha, eval);

        if (beta <= alpha)
            return beta;

        move = generator.pop();
        // skip bad captures - static exchange evaluation

    } while (move.isNull() == false);

    return maxEval;
}