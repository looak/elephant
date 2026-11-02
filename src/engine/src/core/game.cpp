#include <core/game.hpp>
#include <elephant_gambit.hpp>
#include <position/position_accessors.hpp>
#include <eval/evaluator.hpp>
#include <io/fen_parser.hpp>
#include <position/hash_zobrist.hpp>
#include <move/move.hpp>
#include <search/search.hpp>

#include <algorithm>
#include <array>
#include <iostream>
#include <sstream>

void
Game::Reset()
{
    chess::ClearBoard(m_board);
    m_history.moveUndoUnits.clear();
}

void
Game::NewGame()
{
    m_history.moveUndoUnits.clear();
    chess::NewGame(m_board);
}

template<bool validation>
void Game::makeMove(const PackedMove move)
{
    m_board.makeMove<validation>(move, m_history.moveUndoUnits.emplace_back());
}

template void Game::makeMove<false>(const PackedMove move);
template void Game::makeMove<true>(const PackedMove move);

bool Game::unmakeMove()
{
    if (m_history.moveUndoUnits.empty()) return false;

    const bool result = m_board.unmakeMove(m_history.moveUndoUnits.back());
    m_history.moveUndoUnits.pop_back();

    return result;
}
