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
}

void GameContext::NewGame()
{
    Reset();
    FENParser::deserialize(c_startPositionFen.c_str(), *this);
    m_toPlay = Set::WHITE;
}

bool GameContext::MakeMove(Move& move)
{
    auto actualMove = m_board.PlayMove(move);

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

void GameContext::PlayMoves(const Move& move, bool print)
{
    const Move* mv = &move;
    while (mv != nullptr)
    {
        Move madeMove = m_board.PlayMove(*mv);
        if (print)
            PrintBoard(*this, madeMove);

        m_plyCount++;

        // increase moveCount after black has moved.
        if (mv->Piece.getSet() == Set::BLACK)
            m_moveCount++;

        mv = mv->NextMove;
    }
}

std::pair<u64, Move> 
GameContext::concurrentBestMove(int depth, Chessboard& board, Set toPlay)
{
    MoveGenerator generator;
    Evaluator evaluator;
    auto moves = generator.GeneratePossibleMoves(board, toPlay);

    if (moves.empty()) // no moves
        return { -99, Move() };

    std::vector<std::pair<u64, Move>> scoredMoves;

    if (depth == 1)
    {    
        for (auto& move : moves)
        {
            auto& itr = scoredMoves.emplace_back(0, move);
            board.PlayMove(move);
            itr.first = evaluator.Evaluate(board) * -1;
            board.UnmakeMove(move);
        }
    }
    else
    {
        int multiplier = (depth & 1) == 0 ? -1 : 1;
        for (auto& move : moves)
        {            
            board.PlayMove(move);            
            auto& itr = scoredMoves.emplace_back(concurrentBestMove(depth - 1, board, ChessPiece::FlipSet(toPlay)));
            itr.first += evaluator.Evaluate(board) * multiplier;
            board.UnmakeMove(move);
        }        
    }

    std::sort(scoredMoves.begin(), scoredMoves.end(), [](const std::pair<u64, Move>& a, const std::pair<u64, Move>& b) { return a.first > b.first; });
    return scoredMoves[0];
}

Move GameContext::CalculateBestMove()
{
    MoveGenerator generator;
    Evaluator evaluator;
    auto moves = generator.GeneratePossibleMoves(*this);

    std::vector<std::pair<u64, Move>> scoredMoves;

    for(auto& move : moves)
    {
        auto& itr = scoredMoves.emplace_back(0, move);

        auto boardCpy = copyChessboard();
        boardCpy.MakeMove(move);
        u64& refValue = itr.first;
        refValue = evaluator.Evaluate(readChessboard());
        auto bestResponse = concurrentBestMove(3, boardCpy, ChessPiece::FlipSet(readToPlay()));
        refValue -= bestResponse.first;
        boardCpy.UnmakeMove(move);
    }

    std::sort(scoredMoves.begin(), scoredMoves.end(), [](const std::pair<u64, Move>& a, const std::pair<u64, Move>& b) { return a.first > b.first; });
    return scoredMoves[0].second;
}

bool GameContext::isGameOver() const
{
    return m_board.isCheckmated(Set::WHITE) || m_board.isCheckmated(Set::BLACK) || m_board.isStalemated(Set::WHITE) || m_board.isStalemated(Set::BLACK);
}