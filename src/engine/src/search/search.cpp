#include <search/search.hpp>
#include <core/chessboard.hpp>
#include <core/game_context.hpp>
#include <io/printer.hpp>

Search::Search(GameContext& context)
    : m_transpositionTable(context.editTranspositionTable())
    , m_gameContext(context)
    , m_originPosition(context.readChessPosition())
{
    if constexpr (search_policies::TT::enabled) {
        search_policies::TT::assign(m_transpositionTable);
    }
}

void Search::reportResult(SearchResult& searchResult, u32 itrDepth, u64 nodes, u64 elapsedTime) const {
    u32 checkmateDistance = static_cast<u32>(c_checkmateConstant - abs(searchResult.score));

    if (checkmateDistance <= c_maxSearchDepth) {
        // found checkmate within depth.
        searchResult.ForcedMate = true;
        checkmateDistance /= 2;
        io::printer::uci("info score mate {} depth {} nodes {} time {} pv {}",
            (searchResult.score > 0 ? checkmateDistance : -checkmateDistance),
            itrDepth,
            nodes,
            elapsedTime,
            searchResult.pvLine.toString());
        return;
    }

    i32 centipawn = searchResult.score;
    io::printer::uci("info score cp {} depth {} nodes {} time {} pv {}",
        centipawn,
        itrDepth,
        nodes,
        elapsedTime,
        searchResult.pvLine.toString());
}