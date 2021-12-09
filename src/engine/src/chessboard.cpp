#include "chessboard.h"
#include <cstring>

Chessboard::Chessboard()
{
}

Chessboard::Chessboard(const Chessboard& other)
{
	std::memcpy(m_tiles, other.m_tiles, sizeof(ChessboardTile) * 64);
}

void Chessboard::Clear()
{
	std::memset(m_tiles, 0, sizeof(m_tiles));
}