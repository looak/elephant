#include "game_context.h"
#include "evaluator.h"
#include "fen_parser.h"
#include "hash_zorbist.h"
#include "move.h"
#include "move_generator.hpp"
#include "search.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <sstream>

std::string
PrintCastlingState(const Chessboard& board)
{
    std::string ret = "";

    if (board.readCastlingState().hasWhiteKingSide())
        ret += "K";
    if (board.readCastlingState().hasWhiteQueenSide())
        ret += "Q";
    if (board.readCastlingState().hasBlackKingSide())
        ret += "k";
    if (board.readCastlingState().hasBlackQueenSide())
        ret += "q";

    return ret;
}

void
GameContext::Reset()
{
    m_board.Clear();
    m_moveHistory.clear();
}

void
GameContext::NewGame()
{
    Reset();
    FENParser::deserialize(c_startPositionFen.c_str(), *this);
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
    auto undoUnit = m_board.MakeMove<false>(move);
    m_undoUnits.push(undoUnit);
    return true;
}

bool
GameContext::TryMakeMove(Move move)
{
    PackedMove found = PackedMove::NullMove();
    if (move.isAmbiguous()) {
        MoveGenerator generator(m_board.readPosition(), m_board.readToPlay(), move.Piece.getType());
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
        if (m_board.readPieceAt(move.TargetSquare.toSquare()).isValid()) {
            move.setCapture(true);
        }
        found = move.readPackedMove();
    }

    return MakeMove(found);
}

bool
GameContext::UnmakeMove()
{
    if (m_undoUnits.empty())
        return false;

    auto undoUnit = m_undoUnits.top();
    m_undoUnits.pop();
    m_board.UnmakeMove(undoUnit);

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