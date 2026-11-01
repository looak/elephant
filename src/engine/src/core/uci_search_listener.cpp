#include <core/uci_search_listener.hpp>

#include <io/printer.hpp>
#include <search/core/search_constants.hpp>

#include <cstdlib>

void UciSearchListener::onIteration(const IterationInfo& info) {
    if (info.threadId != 0) {
        if (m_reportHelpers)
            io::printer::uci("info string thread {} depth {} score {} nodes {} pv {}", info.threadId, info.depth,
                info.score, info.nodes, info.pv.toString());
        return;
    }

    // mate scores are +-(checkmate - matePly) with the root at ply 1, so plies from the root position is distance - 1.
    const i32 checkmateDistance = c_checkmateConstant - std::abs(info.score);

    if (checkmateDistance <= static_cast<i32>(c_maxSearchDepth)) {
        // UCI reports moves, not plies. Mating takes (plies + 1) / 2 of our moves, being mated plies / 2 of theirs.
        const i32 plies = checkmateDistance - 1;
        const i32 mateInMoves = info.score > 0 ? (plies + 1) / 2 : -(plies / 2);
        io::printer::uci("info score mate {} depth {} nodes {} time {} hashfull {} pv {}", mateInMoves, info.depth,
            info.nodes, info.timeMs, info.hashfull, info.pv.toString());
        return;
    }

    io::printer::uci("info score cp {} depth {} nodes {} time {} hashfull {} pv {}", info.score, info.depth, info.nodes,
        info.timeMs, info.hashfull, info.pv.toString());
}
