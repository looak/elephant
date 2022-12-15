#include "game_context.h"
#include "move.h"
#include <iostream>
#include <sstream>
#include <array>

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
    std::cout << " > castling state: " << PrintCastlingState(board) << "\n";
    std::cout << " > prev move: " << Notation::toString(move.SourceSquare) << Notation::toString(move.TargetSquare) << "\n";

    return true;
}

void GameContext::PlayMoves(const Move& move, bool print)
{
	const Move* mv = &move;
	while (mv != nullptr)
	{
		Move madeMove = m_board.MakeMove(*mv);
		if (print)
            PrintBoard(*this, madeMove);

        m_plyCount++;

        // increase moveCount after black has moved.
        if (mv->Piece.getSet() == Set::BLACK)
            m_moveCount++;

        mv = mv->NextMove;
	}
}