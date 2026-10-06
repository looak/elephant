#include <search/search.hpp>

#include <eval/evaluator.hpp>

#include <move/generation/move_generator.hpp>
#include <move/move_executor.hpp>

#include <search/search_policies.hpp>
#include <search/search_threadcontext.hpp>
#include <system/time_manager.hpp>

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

template u16 Search::mostValuablePieceInPosition<Set::WHITE>(PositionReader);
template u16 Search::mostValuablePieceInPosition<Set::BLACK>(PositionReader);

template<Set us>
i16 Search::quiescence(ThreadSearchContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, bool checked) {
    ASSERT_MSG(depth >= 0, "Depth cannot be negative in alphaBeta.");
    ASSERT_MSG(ply < c_maxSearchDepth, "Ply exceeds maximum search depth in alphaBeta.");    
    ASSERT_MSG(alpha >= -c_infinity && beta <= c_infinity, "Alpha and Beta must be within valid bounds in alphaBeta.");

    i16 standPat = -c_infinity;
    bool checkExtension = false;

    if (checked == false) {
        standPat = context.evaluate<us>();

        // Stand-pat beta cutoff
        if (standPat >= beta)
            return standPat;

        if (standPat > alpha)
            alpha = standPat;

        // Leaf node - return stand-pat
        if (depth <= 0 || ply >= c_maxSearchDepth)
            return standPat;
    }
    else if (depth <= 0 || ply >= c_maxSearchDepth) {
        // If in check and at leaf node, extend search by 1 ply
        // BUG: this can lead to infinite extensions if not handled properly
        checkExtension = true;
        depth += 1;
    }

    // Futility pruning
    // u16 mvValue = mostValuablePieceInPosition<opposing_set<us>()>(context.position.read());
    // if (search_policies::QuiescencePolicy::futile(depth, standPat + mvValue, alpha))
    //     return standPat;

    // Generate captures
    MoveTypes filter = checkExtension ? MoveTypes::ALL : MoveTypes::CAPTURES_ONLY;
    MoveGenParams genParams = MoveGenParams{ .moveFilter =  filter };
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

    // If we were in check, generated moves, but found NO legal moves -> Checkmate.
    if (checked && bestEval == -c_infinity) {
        return checked_cast<i16>(-c_checkmateConstant + ply); // Mate score
    }

    return bestEval;
}

template i16 Search::quiescence<Set::WHITE>(ThreadSearchContext&, u8, i16, i16, u16, bool);
template i16 Search::quiescence<Set::BLACK>(ThreadSearchContext&, u8, i16, i16, u16, bool);