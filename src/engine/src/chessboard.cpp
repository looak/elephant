#include "chessboard.h"
#include <cstring>
#include <utility>

Notation Notation::BuildPosition(byte file, byte rank)
{
	Notation result(0xF, 0xF);
	byte corrFile = (byte)(tolower(file) - 'a');
	byte corrRank = rank - 1;

	// validate placement is inside the board.
	if (corrFile > 7 || corrRank > 7)
		return std::move(result);

	result.file = corrFile;	
	result.rank = corrRank;
	return std::move(result);
}

Notation& Notation::operator=(Notation&& other)
{
	file = other.file;
	rank = other.rank;
	return *this;
}

ChessboardTile::ChessboardTile(Notation&& notation) :
	m_position(std::move(notation))
{
}

bool ChessboardTile::operator==(const ChessboardTile& rhs) const
{
	bool result = m_position == rhs.m_position;
	result |= m_piece == rhs.m_piece;
	return result;
}

bool Notation::operator==(const Notation& rhs) const
{
	bool result = rank == rhs.rank;
	result |= file == rhs.file;	
	return result;
}

Chessboard::Chessboard()
{
	for (byte r = 0; r < 8; r++)
	{
		for (byte f = 0; f < 8; f++)
		{
			Notation pos(r,f);
			m_tiles[pos.getIndex()].editPosition() = std::move(pos);
		}
	}
}

Chessboard::Chessboard(const Chessboard& other)
{
	std::memcpy(m_tiles, other.m_tiles, sizeof(m_tiles));
}

//const ChessboardTile& Chessboard::getTile(const Notation& position) const
//{
//	return m_tiles[position.getIndex()];
//}
//
//ChessboardTile& Chessboard::editTile(const Notation& position)
//{
//	return m_tiles[position.getIndex()];
//}

void Chessboard::Clear()
{
	std::memset(m_tiles, 0, sizeof(m_tiles));
}

bool Chessboard::PlacePiece(ChessPiece p, const Notation& tile)
{
	m_tiles[tile.getIndex()].editPiece() = std::move(p);
	return false;
}

// const ChessPiece&
// Chessboard::readTile(const Notation& position) const
// {
// 	return m_tiles[position.getIndex()].m_piece;
// }

const ChessPiece&
Chessboard::readTile(const Notation& position) const
{
	return m_tiles[position.getIndex()].m_piece;
}

ChessboardTile& 
Chessboard::editTile(const Notation& position)
{
	return m_tiles[position.getIndex()];
}

Chessboard::Iterator::Iterator(Chessboard& chessboard) :
	m_chessboard(chessboard),
	m_end(false)
{ }

ChessboardTile&
Chessboard::Iterator::get()
{
	return m_chessboard.editTile(m_position);
}

ChessboardTile&
Chessboard::Iterator::operator*()
{
	return get();
}

void Chessboard::Iterator::operator++()
{
	m_position.file++;
	if (m_position.file >= 7)
	{
		m_position.file = 0;
		m_position.rank++;

		if (m_position.rank >= 8)
			m_end = true;
	}
}