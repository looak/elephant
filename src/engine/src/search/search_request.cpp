#include <search/search_request.hpp>

#include <core/game_context.hpp>
#include <diagnostics/logger.hpp>
#include <move/generation/move_ordering_view.hpp>
#include <search/core/search_move_source.hpp>
#include <search/state/thread_context.hpp>

namespace {
template<Set us>
std::vector<PackedMove> legalRootMoves(PositionReader position, const std::vector<PackedMove>& requested) {
    std::vector<PackedMove> rootMoves;
    MoveOrderingView ordering;
    SearchMoveSource<us> legalMoves(position);
    legalMoves.start(&ordering, MoveTypes::ALL);
    for (PrioritizedMove legal = legalMoves.next(); legal.move.isNull() == false; legal = legalMoves.next()) {
        for (PackedMove move : requested) {
            if (ThreadContext::sameUciMove(move, legal.move)) {
                rootMoves.push_back(legal.move);
                break;
            }
        }
    }
    return rootMoves;
}
} // namespace

SearchRequest makeSearchRequest(const GameContext& game, SearchLimits limits) {
    SearchRequest request;
    request.root = game.readChessPosition().copy();
    request.toPlay = game.readToPlay();
    request.rootHalfmoveClock = game.readPly();

    const auto& undoUnits = game.readGameHistory().moveUndoUnits;
    request.gameHashes.reserve(undoUnits.size());
    for (const auto& undoUnit : undoUnits)
        request.gameHashes.push_back(undoUnit.hash);

    // go searchmoves, with none of them legal here the root isn't restricted, the search must still return a move.
    if (limits.SearchMoves.empty() == false) {
        const PositionReader position = request.root.read();
        request.rootMoves = request.toPlay == Set::WHITE ? legalRootMoves<Set::WHITE>(position, limits.SearchMoves)
                                                         : legalRootMoves<Set::BLACK>(position, limits.SearchMoves);
        if (request.rootMoves.empty())
            LOG_WARN("go searchmoves: none of the given moves are legal, searching all moves.");
    }

    request.limits = std::move(limits);
    return request;
}
