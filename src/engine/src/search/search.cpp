#include <search/search.hpp>
#include <search/search_constants.hpp>

#include <core/chessboard.hpp>
#include <core/game_context.hpp>
#include <eval/evaluator.hpp>
#include <io/fen_parser.hpp>
#include <move/generation/move_generator.hpp>
#include <util/clock.hpp>
#include <debug/log.hpp>
#include <future>
#include <limits>
#include <sstream>
#include <thread>
#include <utility>

void Search::reportResult(SearchResult& searchResult, u32 itrDepth, u64 nodes, u64 elapsedTime) const {
    u32 checkmateDistance = static_cast<u32>(c_checkmateConstant - abs(searchResult.score));

    if (checkmateDistance <= c_maxSearchDepth) {
        // found checkmate within depth.
        searchResult.ForcedMate = true;
        checkmateDistance /= 2;
        std::cout << "info mate " << checkmateDistance << " depth " << itrDepth << " nodes " << nodes
            << " time " << elapsedTime << " pv " << searchResult.pvLine.toString() << "\n";

        return;
    }

    i32 centipawn = searchResult.score;
    std::cout << "info score cp " << centipawn << " depth " << itrDepth
        << " nodes " << nodes << " time " << elapsedTime << " pv " << searchResult.pvLine.toString() << "\n";
}