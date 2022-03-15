#include "chess_piece.h"
#include <iostream>

ChessPiece::ChessPiece() :
	m_internalState(0x00)
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

bool ChessPiece::fromString(char piece)
{
	PieceSet set = PieceSet::BLACK;
	if (std::isupper(piece))
	{
		set = PieceSet::WHITE;
	}

	char lower = std::tolower(piece);
	PieceType type;
	switch(lower)
	{
		case 'p':
			type = PieceType::PAWN;
			break;
		case 'n':
			type = PieceType::KNIGHT;
			break;
		case 'b':
			type = PieceType::BISHOP;
			break;
		case 'r':
			type = PieceType::ROOK;
			break;
		case 'q':
			type = PieceType::QUEEN;
			break;
		case 'k':
			type = PieceType::KING;
			break;
		default:
			m_internalState = 0;
			return false;
	}

	*this = ChessPiece(set, type);
	return true;
}

bool ChessPiece::operator==(const ChessPiece& rhs) const
{
	return m_internalState == rhs.m_internalState;
}