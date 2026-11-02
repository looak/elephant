#include <search/search.hpp>
#include <search/state/transposition_table.hpp>

Search::Search(SearchContext& searchContext, TranspositionTable& transpositionTable, SearchListener& listener)
    : m_transpositionTable(transpositionTable)
    , m_searchContext(searchContext)
    , m_listener(listener)
{
}
