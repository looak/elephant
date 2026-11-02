#include <core/game_context.hpp>
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
GameContext::Reset()
{
    chess::ClearBoard(m_board);
    m_history.moveUndoUnits.clear();
    // keeping transposition table
}

void
GameContext::NewGame()
{
    m_history.moveUndoUnits.clear();
    chess::NewGame(m_board);
}

template<bool validation>
void GameContext::MakeMove(const PackedMove move)
{
    m_board.makeMove<validation>(move, m_history.moveUndoUnits.emplace_back());
    m_history.age++;
}

template void GameContext::MakeMove<false>(const PackedMove move);
template void GameContext::MakeMove<true>(const PackedMove move);

bool GameContext::UnmakeMove()
{
    if (m_history.moveUndoUnits.empty()) return false;

    const bool result = m_board.unmakeMove(m_history.moveUndoUnits.back());
    m_history.moveUndoUnits.pop_back();
    m_history.age--;

    return result;
}

bool GameHistory::IsRepetition(u64 hashKey) const {

    if (moveUndoUnits.size() < 4)
        return false;

    int count = 0;

    auto itr = moveUndoUnits.rbegin() + 1;
    while (itr != moveUndoUnits.rend()) {
        if (itr->hash == hashKey)
            count++;
        itr++;
    }
    return count >= 2;
}