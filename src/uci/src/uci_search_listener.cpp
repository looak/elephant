#include <uci/uci_search_listener.hpp>
#include <uci/uci_printer.hpp>

#include <diagnostics/logger.hpp>
#include <io/printer.hpp>
#include <search/core/search_constants.hpp>

#include <cstdlib>
#include <string>

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

void UciSearchListener::onBestMove(const SearchResult& result, u64 elapsedMs) {
    // the iteration lines only count completed iterations, this one counts every node searched, all threads together,
    // like Stockfish's last line. Test runners & GUIs read the total from it.
    io::printer::uci("info nodes {} time {} nps {}", result.count, elapsedMs,
                     elapsedMs > 0 ? result.count * 1000 / elapsedMs : result.count);

    // no legal move at the root (mate or stalemate) is a null move, UCI writes that as 0000.
    io::printer::uci("bestmove {}", result.move().isNull() ? std::string("0000") : result.move().toString());
    io::printer::uci_flush();
}

void UciSearchListener::onSearchFailed(const std::exception& e) {
    LOG_WARN("{}", e.what());
    io::printer::uci("info string error {}", e.what());
    io::printer::uci("bestmove 0000");
    io::printer::uci_flush();
}
