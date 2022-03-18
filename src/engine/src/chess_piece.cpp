#include "chess_piece.h"
#include "log.h"
#include <iostream>

signed short ChessPieceDef::m_moveCount[6] = {
	2, 8, 4, 4, 8, 8
};

bool ChessPieceDef::m_slides[6] = {
	false, false, true, true, true, false,
};

signed short ChessPieceDef::m_moves0x88[6][8] = {
	{ -16, -32, 0, 0, 0, 0, 0, 0 },
	{ -33, -31, -18, -14, 14, 18, 31, 33 },
	{ -17, -15, 15, 17, 0, 0, 0, 0 },
	{ -16, -1, 1, 16, 0, 0, 0, 0 },
	{ -17, -16, -15, -1, 1, 15, 16, 17 },
	{ -17, -16, -15, -1, 1, 15, 16, 17 }
};

signed short ChessPieceDef::m_attacks0x88[6][8] = {
	{ -15, -17, 0, 0, 0, 0, 0, 0 },
	{ -33, -31, -18, -14, 14, 18, 31, 33 },
	{ -17, -15, 15, 17, 0, 0, 0, 0 },
	{ -16, -1, 1, 16, 0, 0, 0, 0 },
	{ -17, -16, -15, -1, 1, 15, 16, 17 },
	{ -17, -16, -15, -1, 1, 15, 16, 17 }
};

signed short ChessPieceDef::MoveCount(byte pIndex)
{
	return m_moveCount[pIndex];
}

bool ChessPieceDef::Slides(byte pIndex)
{
	return m_slides[pIndex];
}

signed short ChessPieceDef::Moves0x88(byte pIndex, byte mIndex)
{
	return m_moves0x88[pIndex][mIndex];
}

signed short ChessPieceDef::Attacks0x88(byte pIndex, byte mIndex)
{
	return m_attacks0x88[pIndex][mIndex];
}

PieceSet ChessPiece::FlipSet(PieceSet source)
{
	int retValue = !(int)source;
	return (PieceSet)retValue;
}

byte ChessPiece::FlipSet(byte source)
{
	byte retValue = !source;
	return retValue;
}


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
	char retValue = ' ';
	switch (getType())
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
		case PieceType::NON:
		retValue = ' ';
		break;
	default:
		LOG_ERROR() << "Invalid Chess Piece;\n";
	}

	if (getSet() == PieceSet::WHITE)
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