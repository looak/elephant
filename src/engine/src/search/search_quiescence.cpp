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
i16 Search::quiescence(ThreadSearchContext& context, u8 depth, i16 alpha, i16 beta, u16 ply) {
    ASSERT_MSG(ply < c_maxSearchDepth, "Ply exceeds maximum search depth in quiescence.");
    ASSERT_MSG(alpha >= -c_infinity && beta <= c_infinity, "Alpha and Beta must be within valid bounds in quiescence.");

    // hard ply limit, the static eval is the best we can do even when in check.
    if (ply >= c_maxSearchDepth - 1)
        return context.evaluate<us>();

    // The generator knows reliably whether we're in check (move check flags don't), the filter is read lazily on the
    // first pop so it can be decided after construction.
    MoveGenParams genParams;
    MoveGenerator<us> generator(context.position.read(), genParams);
    const bool inCheck = generator.isChecked();

    i16 bestEval = -c_infinity;
    if (inCheck) {
        // no stand pat while in check, every evasion has to be searched. Captures alone could miss the only escapes.
        genParams.moveFilter = MoveTypes::ALL;
    }
    else {
        const i16 standPat = context.evaluate<us>();

        // Stand-pat beta cutoff
        if (standPat >= beta)
            return standPat;

        if (standPat > alpha)
            alpha = standPat;

        // Leaf node - return stand-pat
        if (depth == 0)
            return standPat;

        bestEval = standPat;
        genParams.moveFilter = MoveTypes::CAPTURES_ONLY;
    }

    // Futility pruning
    // u16 mvValue = mostValuablePieceInPosition<opposing_set<us>()>(context.position.read());
    // if (search_policies::QuiescencePolicy::futile(depth, standPat + mvValue, alpha))
    //     return standPat;

    // evasions don't consume quiescence depth, the line stays bounded since only captures are searched once out of
    // check, and the ply limit above.
    const u8 childDepth = (inCheck || depth == 0) ? depth : static_cast<u8>(depth - 1);
    u32 movesSearched = 0;
    PrioritizedMove ordered = generator.pop();

    while (!ordered.move.isNull()) {
        if (context.shouldStop())
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
        
        i16 qEval = -quiescence<opposing_set<us>()>(context, childDepth, -beta, -alpha, ply + 1);
        context.qNodeCount++;

        executor.unmakeMove(undoState);

        // the child was aborted, its score is meaningless.
        if (context.stopped)
            return 0;

        ++movesSearched;

        if (qEval > bestEval)
            bestEval = qEval;

        if (bestEval >= beta)
            return bestEval;
        
        if (bestEval > alpha)
            alpha = bestEval;

        ordered = generator.pop();
    }

    if (context.stopped)
        return 0;

    // In check with every legal move generated and none found -> Checkmate.
    if (inCheck && movesSearched == 0) {
        return checked_cast<i16>(-c_checkmateConstant + ply); // Mate score
    }

    return bestEval;
}

template i16 Search::quiescence<Set::WHITE>(ThreadSearchContext&, u8, i16, i16, u16);
template i16 Search::quiescence<Set::BLACK>(ThreadSearchContext&, u8, i16, i16, u16);