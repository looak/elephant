#include "game_context.h"
#include "move.h"
#include <iostream>
#include <sstream>
#include <array>

bool PrintBoard(const Chessboard& board)
{
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

    return true;
}

void GameContext::PlayMoves(const Move& move)
{
	const Move* mv = &move;
	while (mv != nullptr)
	{
		m_board.MakeMove(*mv);
		mv = mv->NextMove;

        PrintBoard(m_board);
	}
}