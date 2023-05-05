#include "game_context.h"
#include "evaluator.h"
#include "fen_parser.h"
#include "hash_zorbist.h"
#include "move_generator.h"
#include "move.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <sstream>

std::string PrintCastlingState(const Chessboard& board)
{
    std::string ret = "";

    if (board.readCastlingState() & 1)
        ret += "K";
    if (board.readCastlingState() & 2)
        ret += "Q";
    if (board.readCastlingState() & 4)
        ret += "k";
    if (board.readCastlingState() & 8)
        ret += "q";

    return ret;
}

bool PrintBoard(const GameContext& context, const Move& move)
{
    const Chessboard& board = context.readChessboard();
    auto boardItr = board.begin();
    std::array<std::stringstream, 8> ranks;

    byte prevRank = -1;
    do
    {
        if (prevRank != boardItr.rank())
        {
            ranks[boardItr.rank()] << "\n > " << (int)(boardItr.rank() + 1) << "  ";
        }

        ranks[boardItr.rank()] << '[' << (*boardItr).readPiece().toString() << ']';
        prevRank = boardItr.rank();
        ++boardItr;

    } while (boardItr != board.end());

    auto rankItr = ranks.rbegin();
    while (rankItr != ranks.rend())
    {
        std::cout << (*rankItr).str();
        rankItr++;
    }

    std::cout << "\n >\n >     A  B  C  D  E  F  G  H\n";
    std::cout << " > move: " << std::dec << (int)context.readMoveCount() << "\tply: " << (int)context.readPly() << "\n";
    std::cout << " > hash: 0x" << std::hex << board.readHash() << "\n";
    std::cout << " > hash: 0x" << ZorbistHash::Instance().HashBoard(board) << "\n";
    std::cout << " > castling state: " << PrintCastlingState(board) << "\n";
    std::cout << " > prev move: " << Notation::toString(move.SourceSquare) << Notation::toString(move.TargetSquare) << "\n";
    std::string output;
    FENParser::serialize(context, output);
    std::cout << " > fen: " << output << "\n";

    return true;
}

void GameContext::Reset()
{
    m_board.Clear();
    m_moveCount = 0;
    m_plyCount = 0;
    m_fiftyMoveRule = 0;
    m_moveHistory.clear();
}

void GameContext::NewGame()
{
    Reset();
    FENParser::deserialize(c_startPositionFen.c_str(), *this);
    m_toPlay = Set::WHITE;
}

bool GameContext::GameOver() const
{
    // if (m_fiftyMoveRule >= 50) // not fully implemented yet
    //     return true;
    if (readChessboard().isCheckmated(m_toPlay))
        return true;
    if (readChessboard().isStalemated(m_toPlay))
        return true;

    // Set opSet = ChessPiece::FlipSet(m_toPlay);
    // if (readChessboard().isCheckmated(opSet))
    //     return true;
    // if (readChessboard().isStalemated(opSet))
    //     return true;
    
    return false;    
}

bool GameContext::PlayMove(Move& move)
{
    std::string pgn = m_board.SerializeMoveToPGN(move);

    if(!MakeMove(move))
        return false;
        
    MoveHistory entry = { m_board.readHash(), m_plyCount, m_moveCount, m_fiftyMoveRule, pgn };
    m_moveHistory.emplace_back(entry);

    return true;
}

bool GameContext::MakeLegalMove(Move& move)
{
    m_board.MakeMoveUnchecked(move);

    if (move.Piece.getSet() == Set::BLACK)
        m_moveCount++;

    if (move.isCapture() || move.Piece.getType() == PieceType::PAWN)
        m_fiftyMoveRule = 0;
    else
        m_fiftyMoveRule++;

    m_plyCount++;

    m_toPlay = m_toPlay == Set::WHITE ? Set::BLACK : Set::WHITE;

    return true;
}

bool GameContext::MakeMove(Move& move)
{
    Move actualMove = move;
    
    if (!m_board.MakeMove(actualMove))
        return false;

    if (actualMove.isInvalid())
        return false;

    if (actualMove.Piece.getSet() == Set::BLACK)
        m_moveCount++;

    if (actualMove.isCapture() || actualMove.Piece.getType() == PieceType::PAWN)
        m_fiftyMoveRule = 0;
    else
        m_fiftyMoveRule++;

    m_plyCount++;

    m_toPlay = m_toPlay == Set::WHITE ? Set::BLACK : Set::WHITE;
    
    move = actualMove;
    return true;
}

bool GameContext::UnmakeMove(const Move& move)
{
    if (m_board.UnmakeMove(move))
    {
        if (move.Piece.getSet() == Set::BLACK)
            m_moveCount--;

        if (move.isCapture() || move.Piece.getType() == PieceType::PAWN)
            m_fiftyMoveRule = 0;
        else
            m_fiftyMoveRule--;

        m_plyCount--;

        m_toPlay = m_toPlay == Set::WHITE ? Set::BLACK : Set::WHITE;

        return true;
    }

    return false;
}

SearchResult GameContext::CalculateBestMove(SearchParameters params)
{
    MoveGenerator generator;
    return generator.CalculateBestMove(*this, params);
}

bool GameContext::isGameOver() const
{
    return m_board.isCheckmated(Set::WHITE) || m_board.isCheckmated(Set::BLACK) || m_board.isStalemated(Set::WHITE) || m_board.isStalemated(Set::BLACK);
}