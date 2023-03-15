#include "game_context.h"
#include "move.h"
#include <iostream>
#include <sstream>
#include <array>
#include "fen_parser.h"
#include "hash_zorbist.h"

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

bool GameContext::MakeMove(Move& move)
{	
	auto actualMove = m_board.PlayMove(move);
    
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

bool GameContext::endOfGame() const
{
    return m_board.isCheckmated(Set::WHITE) || m_board.isCheckmated(Set::BLACK) || m_board.isStalemated(Set::WHITE) || m_board.isStalemated(Set::BLACK);
}