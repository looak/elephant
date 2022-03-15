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
		return result;

	result.file = corrFile;	
	result.rank = corrRank;
	return result;
}

Notation& Notation::operator=(Notation&& other)
{
	file = other.file;
	rank = other.rank;
	return *this;
}

bool Notation::operator==(const Notation& rhs) const
{
	bool result = rank == rhs.rank;
	result &= file == rhs.file;
	return result;
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

void Chessboard::Clear()
{
	std::memset(m_tiles, 0, sizeof(m_tiles));
}

bool Chessboard::PlacePiece(ChessPiece p, const Notation& tile)
{
	m_tiles[tile.getIndex()].editPiece() = std::move(p);
	return false;
}

const ChessboardTile&
Chessboard::readTile(const Notation& position) const
{
	return m_tiles[position.getIndex()];
}

ChessboardTile& 
Chessboard::editTile(const Notation& position)
{
	return m_tiles[position.getIndex()];
}

const Notation s_beginPos = Notation::BuildPosition('a', 1);
const Notation s_endPos = Notation(0, 8);

Chessboard::Iterator
Chessboard::begin()
{
	return Chessboard::Iterator(*this, Notation(s_beginPos));
}

Chessboard::Iterator 
Chessboard::end()
{
	return Chessboard::Iterator(*this, Notation(s_endPos));
}
Chessboard::ConstIterator 
Chessboard::begin() const
{
	return Chessboard::ConstIterator(*this, Notation(s_beginPos));
}
Chessboard::ConstIterator
Chessboard::end() const
{
	return Chessboard::ConstIterator(*this, Notation(s_endPos));
}