#include <search/search.hpp>
#include <core/chessboard.hpp>
#include <core/game_context.hpp>
#include <search/state/transposition_table.hpp>

Search::Search(SearchContext& searchContext, GameContext& context, SearchListener& listener)
    : m_transpositionTable(context.editTranspositionTable())
    , m_searchContext(searchContext)
    , m_gameContext(context)
    , m_listener(listener)
    , m_originPosition(context.readChessPosition())
{
}
