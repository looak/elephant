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

template<bool validation>
void GameContext::MakeMove(const PackedMove move)
{
    auto& gameState = m_board.editState();
    auto& undoUnit = m_history.moveUndoUnits.emplace_back();
    
    MoveExecutor executor(m_board.editPosition());
    executor.makeMove<validation>(move, undoUnit, gameState.plyCount);

    // flip the bool and if we're back at white turn we assume we just made a black turn and hence we increment the move count.
    gameState.whiteToMove = !gameState.whiteToMove;
    gameState.moveCount += (short)gameState.whiteToMove;
    m_history.age++;
}

template void GameContext::MakeMove<false>(const PackedMove move);
template void GameContext::MakeMove<true>(const PackedMove move);

bool GameContext::UnmakeMove()
{
    MoveExecutor executor(m_board.editPosition());
    if (m_history.moveUndoUnits.empty()) return false;

    MoveUndoUnit undoState = m_history.moveUndoUnits.back();
    m_history.moveUndoUnits.pop_back();

    bool result = executor.unmakeMove(undoState);
    
    auto& gameState = m_board.editState();
    gameState.moveCount -= (short)gameState.whiteToMove;
    gameState.whiteToMove = !gameState.whiteToMove;
    gameState.plyCount = undoState.plyCount;
    m_history.age--;

    return result;
}

bool GameContext::isGameOver() const
{
    return false;
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