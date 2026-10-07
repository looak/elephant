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
    // mate scores are +-(checkmate - matePly) with the root at ply 1, so plies from the root position is distance - 1.
    const i32 checkmateDistance = c_checkmateConstant - abs(searchResult.score);

    if (checkmateDistance <= static_cast<i32>(c_maxSearchDepth)) {
        // found checkmate within depth.
        searchResult.ForcedMate = true;
        // UCI reports moves, not plies. Mating takes (plies + 1) / 2 of our moves, being mated plies / 2 of theirs.
        const i32 plies = checkmateDistance - 1;
        const i32 mateInMoves = searchResult.score > 0 ? (plies + 1) / 2 : -(plies / 2);
        io::printer::uci("info score mate {} depth {} nodes {} time {} pv {}",
            mateInMoves,
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