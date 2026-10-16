#include <quiet.hpp>

#include <algorithm>
#include <cstdlib>

#include <move/move_executor.hpp>
#include <search/search_constants.hpp>
#include <search/search_move_source.hpp>
#include <search/search_policies.hpp>
#include <search/static_exchange.hpp>

namespace tuner {

namespace {

// Search::quiescence (search_quiescence.cpp) with a principal variation: pv is set to the best line from here when a
// move raises alpha, and left empty when standing pat is best.
template<Set us>
i16 quiescence(ThreadSearchContext& context, u8 depth, i16 alpha, i16 beta, u16 ply, QuietLine& pv) {
    pv.length = 0;

    if (ply >= c_maxSearchDepth - 1 || ply >= c_maxQuietLine)
        return context.evaluate<us>();

    SearchMoveSource<us> moves(context.position.read());
    const bool inCheck = moves.isChecked();

    i16 bestEval = -c_infinity;
    i16 standPat = -c_infinity;
    if (inCheck) {
        moves.start(nullptr, MoveTypes::ALL);
    }
    else {
        standPat = context.evaluate<us>();
        if (standPat >= beta)
            return standPat;
        if (standPat > alpha)
            alpha = standPat;
        if (depth == 0)
            return standPat;

        bestEval = standPat;

        if constexpr (search_policies::QuiescencePolicy::deltaPruning) {
            if (search_policies::QuiescencePolicy::deltaPruneNode(context.position.read().material(), us, standPat, alpha))
                return standPat;
        }

        moves.start(nullptr, MoveTypes::CAPTURES_ONLY);
    }

    const u8 childDepth = (inCheck || depth == 0) ? depth : static_cast<u8>(depth - 1);
    u32 movesSearched = 0;
    QuietLine childPv;

    for (PrioritizedMove ordered = moves.next(); !ordered.move.isNull(); ordered = moves.next()) {
        const PackedMove move = ordered.move;

        if constexpr (search_policies::QuiescencePolicy::deltaPruning) {
            if (!inCheck && search_policies::QuiescencePolicy::deltaPruneMove(context.position.read().material(), move, standPat, alpha))
                continue;
        }
        if constexpr (search_policies::QuiescencePolicy::seePruning) {
            if (!inCheck && !see::ge(context.position.read().material(), move, 0))
                continue;
        }

        MoveExecutor executor = context.executor();
        MoveUndoUnit undoState;
        u16 fiftyMoveCounter = ply;
        executor.makeMove(move, undoState, fiftyMoveCounter);
        const i16 qEval = static_cast<i16>(-quiescence<opposing_set<us>()>(context, childDepth, static_cast<i16>(-beta),
                                                                          static_cast<i16>(-alpha),
                                                                          static_cast<u16>(ply + 1), childPv));
        executor.unmakeMove(undoState);
        ++movesSearched;

        if (qEval > bestEval)
            bestEval = qEval;
        if (bestEval >= beta)
            return bestEval;
        if (bestEval > alpha) {
            alpha = bestEval;
            pv.moves[0] = move;
            for (u32 i = 0; i < childPv.length && i + 1 < c_maxQuietLine; ++i)
                pv.moves[i + 1] = childPv.moves[i];
            pv.length = std::min(childPv.length + 1, c_maxQuietLine);
        }
    }

    if (inCheck && movesSearched == 0)
        return static_cast<i16>(-c_checkmateConstant + ply);

    return bestEval;
}

template<Set us>
QuietLine resolve(ThreadSearchContext& context, bool& decided) {
    QuietLine pv;
    const i16 score = quiescence<us>(context, search_policies::QuiescencePolicy::maxDepth,
                                     static_cast<i16>(-c_infinity), static_cast<i16>(c_infinity), 1, pv);
    decided = std::abs(score) >= c_checkmateMinScore;
    if (decided)
        return QuietLine{};

    // play the line out, the undo units are dropped, context is reset for the next position.
    u16 fiftyMoveCounter = 0;
    for (u32 i = 0; i < pv.length; ++i) {
        MoveUndoUnit undoState;
        context.executor().makeMove(pv.moves[i], undoState, fiftyMoveCounter);
    }
    return pv;
}

} // namespace

QuietLine resolveToQuiet(ThreadSearchContext& context, bool& decided) {
    return context.gameState.whiteToMove ? resolve<Set::WHITE>(context, decided) : resolve<Set::BLACK>(context, decided);
}

} // namespace tuner
