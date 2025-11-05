#include <search/search.hpp>
#include <search/search_constants.hpp>

#include <core/chessboard.hpp>
#include <core/game_context.hpp>
#include <eval/evaluator.hpp>
#include <io/fen_parser.hpp>
#include <move/generation/move_generator.hpp>
#include <util/clock.hpp>

#include <future>
#include <limits>
#include <sstream>
#include <thread>
#include <utility>


void Search::reportResult(SearchResult& searchResult, u32 searchDepth, u32 itrDepth, u64 nodes, const Clock& clock) const {
    
    i64 et = clock.getElapsedTime();

    i32 checkmateDistance = c_checkmateConstant - abs((int)searchResult.score);
    checkmateDistance = abs(checkmateDistance);
    if ((u32)checkmateDistance <= searchDepth) {
        // found checkmate within depth.
        searchResult.ForcedMate = true;
        checkmateDistance /= 2;
        std::cout << "info mate " << checkmateDistance << " depth " << itrDepth << " nodes " << nodes
            << " time " << et << " pv " << searchResult.pvLine.toString() << "\n";

        return;
    }

    i32 centipawn = searchResult.score;
    std::cout << "info score cp " << centipawn << " depth " << itrDepth
        << " nodes " << nodes << " time " << et << " pv " << searchResult.pvLine.toString() << "\n";
}



// bool Search::TimeManagement(i64 elapsedTime, i64 timeleft, i32 timeInc, u16 depth) {
//     // should return false if we want to abort our search.
//     // how do we manage time?
//     // lots of magic numbers in here.
//     const i64 c_maxTimeAllowed = (timeInc * .75f) + (timeleft / 32);  // at 5min this is 9 seconds.
//     if (elapsedTime > c_maxTimeAllowed) {
//         return false;
//     }
//     else {
//         i64 avrgTime = elapsedTime / depth;
//         avrgTime *= avrgTime;  // assume exponential time increase per depth.
//         avrgTime /= 2;         // give some credit to the alpha beta search.
//         if (avrgTime > c_maxTimeAllowed)
//             return false;
//         else
//             return true;
//     }

//     return false;
// }

CancelSearchCondition Search::buildCancellationFunction(Set perspective, const SearchParameters& params, const Clock& clock) const {
    if (params.BlackTimelimit == 0 && params.WhiteTimelimit == 0 && params.MoveTime == 0) {
        // we will never cancel a search based on time.
        return [&]() {
            return false;
            };
    }

    if (params.MoveTime > 0) {
        i64 moveTime = params.MoveTime;
        return [&clock, moveTime]() {
            return clock.getElapsedTime() > moveTime;
            };
    }

    u32 timelimit = perspective == Set::WHITE ? params.WhiteTimelimit : params.BlackTimelimit;
    i32 timeInc = perspective == Set::WHITE ? params.WhiteTimeIncrement : params.BlackTimeIncrement;

    if (timelimit > 0) {
        const i64 c_maxTimeAllowed = timeInc + (timelimit / 24);  // at 5min this is 12 seconds, at 1min this is 2.5 seconds
        return [&clock, c_maxTimeAllowed]() {
            return clock.getElapsedTime() > c_maxTimeAllowed;
            };
    }

    return [&]() {
        return true;
        };
}

// i32 Search::Extension(const Chessboard&, const PrioritizedMove& prioratized, u16 ply) const {
//     if (ply >= c_maxSearchDepth)
//         return 0;
//     //    board.readPosition().calcKingMask<Set::WHITE>().isChecked();
//     if (prioratized.move.isCapture() || prioratized.move.isPromotion() || prioratized.isCheck()) {
//         return 1;
//     }

//     return 0;
// }

void Search::clear() {
    for (u32 i = 0; i < 2; ++i) {
        for (u32 j = 0; j < 64; ++j) {
            for (u32 k = 0; k < 64; ++k) {
                m_historyHeuristic[i][j][k] = 0;
            }
        }
    }
    for (u32 i = 0; i < 4; ++i) {
        for (u32 j = 0; j < 64; ++j) {
            m_killerMoves[i][j] = PackedMove::NullMove();
        }
    }
}

bool Search::isKillerMove(PackedMove move, u16 ply) const {
    const PackedMove* movesAtPly = &m_killerMoves[0][ply];
    return movesAtPly[0] == move || movesAtPly[1] == move || movesAtPly[2] == move || movesAtPly[3] == move;
}

u32 Search::getHistoryHeuristic(u8 set, u8 src, u8 dst) const {
    return m_historyHeuristic[set][src][dst];
}

void Search::pushKillerMove(PackedMove mv, u16 ply) {
    PackedMove* movesAtPly = &m_killerMoves[0][ply];

    // don't fill the killer move list with the same move.
    if (movesAtPly[0] == mv || movesAtPly[1] == mv || movesAtPly[2] == mv || movesAtPly[3] == mv)
        return;

    movesAtPly[3] = movesAtPly[2];
    movesAtPly[2] = movesAtPly[1];
    movesAtPly[1] = movesAtPly[0];
    movesAtPly[0] = mv;
}

void Search::putHistoryHeuristic(u8 set, u8 src, u8 dst, u32 depth) {
    m_historyHeuristic[set][src][dst] += depth * depth;
}