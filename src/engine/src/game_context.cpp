#include "game_context.h"
#include "evaluator.h"
#include "fen_parser.h"
#include "hash_zorbist.h"
#include "move.h"
#include "move_generator.hpp"
#include "search.h"

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

// bool
// PrintBoard(const GameContext& context, const Move& move)
// {
//     const Chessboard& board = context.readChessboard();
//     auto boardItr = board.begin();
//     std::array<std::stringstream, 8> ranks;

//     byte prevRank = -1;
//     do {
//         if (prevRank != boardItr.rank()) {
//             ranks[boardItr.rank()] << "\n > " << (int)(boardItr.rank() + 1) << "  ";
//         }

//         ranks[boardItr.rank()] << '[' << boardItr.get().toString() << ']';
//         prevRank = boardItr.rank();
//         ++boardItr;

//     } while (boardItr != board.end());

//     auto rankItr = ranks.rbegin();
//     while (rankItr != ranks.rend()) {
//         std::cout << (*rankItr).str();
//         rankItr++;
//     }

//     std::cout << "\n >\n >     A  B  C  D  E  F  G  H\n";
//     std::cout << " > move: " << std::dec << (int)context.readMoveCount() << "\tply: " << (int)context.readPly() << "\n";
//     std::cout << " > hash: 0x" << std::hex << board.readHash() << "\n";
//     std::cout << " > hash: 0x" << ZorbistHash::Instance().HashBoard(board) << "\n";
//     std::cout << " > castling state: " << PrintCastlingState(board) << "\n";
//     std::cout << " > prev move: " << Notation::toString(move.SourceSquare) << Notation::toString(move.TargetSquare) << "\n";
//     std::string output;
//     FENParser::serialize(context, output);
//     std::cout << " > fen: " << output << "\n";

//     return true;
// }

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
    m_toPlay = Set::WHITE;
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

    // m_moveHistory.push_back(MoveHistory());
    // m_moveHistory.back().HashKey = m_board.readHash();
    // m_moveHistory.back().PlyCount = m_board.readPlyCount();
    // m_moveHistory.back().MoveCount = m_board.readMoveCount();
    // m_moveHistory.back().FiftyMoveRule = m_board.readFiftyMoveRule();
    // m_moveHistory.back().SAN = Notation::toString(m);

    m_toPlay = ChessPiece::FlipSet(m_toPlay);
    return true;
}

bool
GameContext::TryMakeMove(Move move)
{
    PackedMove found = PackedMove::NullMove();
    if (move.isAmbiguous()) {
        MoveGenerator generator(m_board.readPosition(), m_toPlay, move.Piece.getType());
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

    // m_board.readHash() = m_moveHistory.back().HashKey;
    // m_board.readPlyCount() = m_moveHistory.back().PlyCount;
    // m_board.readMoveCount() = m_moveHistory.back().MoveCount;
    // m_board.readFiftyMoveRule() = m_moveHistory.back().FiftyMoveRule;
    // m_moveHistory.pop_back();

    m_toPlay = ChessPiece::FlipSet(m_toPlay);
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