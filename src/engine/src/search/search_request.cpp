#include <search/search_request.hpp>

#include <core/game.hpp>
#include <diagnostics/logger.hpp>
#include <move/generation/move_ordering_view.hpp>
#include <search/core/search_move_source.hpp>

namespace {
// A move parsed from UCI notation carries no capture or special flags, compare what the notation holds.
bool sameUciMove(PackedMove lhs, PackedMove rhs) {
    return lhs.sourceSqr() == rhs.sourceSqr() && lhs.targetSqr() == rhs.targetSqr()
        && lhs.isPromotion() == rhs.isPromotion()
        && (!lhs.isPromotion() || lhs.readPromoteToPieceType() == rhs.readPromoteToPieceType());
}

template<Set us>
std::vector<PackedMove> legalRootMoves(PositionReader position, const std::vector<PackedMove>& requested) {
    std::vector<PackedMove> rootMoves;
    MoveOrderingView ordering;
    SearchMoveSource<us> legalMoves(position);
    legalMoves.start(&ordering, MoveTypes::ALL);
    for (PrioritizedMove legal = legalMoves.next(); legal.move.isNull() == false; legal = legalMoves.next()) {
        for (PackedMove move : requested) {
            if (sameUciMove(move, legal.move)) {
                rootMoves.push_back(legal.move);
                break;
            }
        }
    }
    return rootMoves;
}
} // namespace

SearchRequest makeSearchRequest(const Game& game, SearchLimits limits) {
    SearchRequest request;
    request.root = game.readPosition().copy();
    request.toPlay = game.readToPlay();
    request.rootHalfmoveClock = game.readHalfmoveClock();

    const auto undoUnits = game.moves();
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
