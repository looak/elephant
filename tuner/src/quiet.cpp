#include <quiet.hpp>

#include <cstdlib>

#include <move/move_executor.hpp>
#include <search/search.hpp>
#include <search/search_constants.hpp>

namespace tuner {

namespace {

template<Set us>
PVLine resolve(ThreadSearchContext& context, bool& decided) {
    PVLine line;
    const i16 score = Search::quiescenceLine<us>(context, line);
    decided = std::abs(score) >= c_checkmateMinScore;
    if (decided)
        return PVLine{};

    // play the line out, the undo units are dropped, context is reset for the next position.
    u16 fiftyMoveCounter = 0;
    for (u32 i = 0; i < line.length; ++i) {
        MoveUndoUnit undoState;
        context.executor().makeMove(line.moves[i], undoState, fiftyMoveCounter);
    }
    return line;
}

} // namespace

PVLine resolveToQuiet(ThreadSearchContext& context, bool& decided) {
    return context.gameState.whiteToMove ? resolve<Set::WHITE>(context, decided) : resolve<Set::BLACK>(context, decided);
}

} // namespace tuner
