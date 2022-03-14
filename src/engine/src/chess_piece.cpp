#include "chess_piece.h"
#include <iostream>

ChessPiece::ChessPiece() :
	m_internalState(0x00)
{}

ChessPiece::ChessPiece(byte value) :
	m_internalState(value)
{}

ChessPiece::ChessPiece(PieceSet _set, PieceType _type) :
	m_internalState(0x00)
{
	m_internalState |= (byte)_set << 7;
	m_internalState |= (byte)_type;
}

char ChessPiece::toString() const
{
	return toString(*this);
}

char ChessPiece::toString(const ChessPiece& piece)
{
	char retValue = ' ';
	switch(piece.getType())
	{
		case PieceType::PAWN:
			retValue = 'p';
			break;
		case PieceType::BISHOP:
			retValue = 'b';
			break;
		case PieceType::KNIGHT:
			retValue = 'n';
			break;
		case PieceType::ROOK:
			retValue = 'r';
			break;
		case PieceType::QUEEN:
			retValue = 'q';
			break;
		case PieceType::KING:
			retValue = 'k';
			break;
	}

	if (piece.getSet() == PieceSet::WHITE)
		retValue = std::toupper(retValue);

	return retValue;
}

bool ChessPiece::operator==(const ChessPiece& rhs) const
{
	return m_internalState == rhs.m_internalState;
}