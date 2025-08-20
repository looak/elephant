#include <core/game_context.hpp>
#include <elephant_gambit.hpp>
#include <position/position_accessors.hpp>
#include <eval/evaluator.hpp>
#include <io/fen_parser.hpp>
#include <position/hash_zobrist.hpp>
#include <move/move.hpp>
#include <move/generation/move_generator.hpp>
#include <move/move_executor.hpp>
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

bool
GameContext::GameOver() const
{
    // if (m_fiftyMoveRule >= 50) // not fully implemented yet
    //     return true;
    // if (readChessboard().isCheckmated(m_toPlay))
    //     return true;
    // if (readChessboard().isStalemated(m_toPlay))
    //     return true;

    // Set opSet = ChessPiece::FlipSet(m_toPlay);
    // if (readChessboard().isCheckmated(opSet))
    //     return true;
    // if (readChessboard().isStalemated(opSet))
    //     return true;

    return false;
}

void
GameContext::MakeMove(const PackedMove move)
{
    MoveExecutor executor(m_board.editPosition(), m_board.editState(), m_history);
    executor.makeMove<false>(move);
}

bool
GameContext::isGameOver() const
{
    return false;
}

bool GameContext::IsRepetition(u64 hashKey) const {

    if (m_history.moveUndoUnits.size() < 4)
        return false;

    int count = 0;

    auto itr = m_history.moveUndoUnits.rbegin() + 1;
    while (itr != m_history.moveUndoUnits.rend()) {
        if (itr->hash == hashKey)
            count++;
        itr++;
    }
    return count >= 2;
}