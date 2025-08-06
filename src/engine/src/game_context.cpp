#include "game_context.h"
#include <elephant_gambit.h>
#include <position/position_accessors.hpp>
#include "evaluator.h"
#include <serializing/fen_parser.hpp>
#include <position/hash_zorbist.hpp>
#include <move/move.hpp>
#include <move_generation/move_generator.hpp>
#include <move/move_executor.hpp>
#include "search.hpp"

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

bool
GameContext::MakeMove(const PackedMove move)
{
    MoveExecutor executor(m_board.editPosition());
    executor.makeMove<false>(move, m_board.editState(), m_history);
    return true;
}

bool
GameContext::TryMakeMove(Move move)
{
    PackedMove found = PackedMove::NullMove();
    PositionReader position = m_board.readPosition();

    if (move.isAmbiguous()) {
        MoveGenerator generator(position, m_board.readToPlay(), move.Piece.getType());
        generator.generate();

        generator.forEachMove([&](const PrioratizedMove& pm) {
            if (pm.move.targetSqr() == move.TargetSquare.toSquare()) {
                found = pm.move;
                return;
            }
            });

        if (found == PackedMove::NullMove())
            return false;
    }
    else {
        // set capture if the target square is occupied.
        ChessPiece piece = position.pieceAt(move.TargetSquare.toSquare());
        if (piece.isValid()) {
            move.setCapture(true);
        }
        found = move.readPackedMove();
    }

    return MakeMove(found);
}

bool
GameContext::UnmakeMove()
{
    // if (m_undoUnits.empty())
    //     return false;

    // auto undoUnit = m_undoUnits.back();
    // m_board.UnmakeMove(undoUnit);
    // m_undoUnits.pop_back();

    return true;
}

SearchResult
GameContext::CalculateBestMove(SearchParameters params)
{
    Search search;
    return search.CalculateBestMove(*this, params);
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