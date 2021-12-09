// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021  Alexander Loodin Ek

// This program is free software : you can redistribute it and /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.If not, see < http://www.gnu.org/licenses/>.
#pragma once
#include "defines.h"

enum PieceType : byte
{
	NON = 0,
	PAWN = 1,
	KNIGHT = 2,
	BISHOP = 3,
	ROOK = 4,
	QUEEN = 5,
	KING = 6,
	NR_OF_PIECES = 7,
};

enum PieceSet : byte
{
	WHITE = 0,
	BLACK = 1,
	NR_OF_SETS = 2
};

struct ChessPiece
{
public:
	ChessPiece() :
		m_internalState(0x00)
	{}

	ChessPiece(byte value) :
		m_internalState(value)
	{}

	ChessPiece(PieceSet _set, PieceType _type) :
		m_internalState(0x00)
	{
		m_internalState |= _set << 7;
		m_internalState |= _type;
	}

	PieceType getType()	{ return static_cast<PieceType>(m_internalState & 0x07); }
	PieceSet getSet()	{ return static_cast<PieceSet>(m_internalState >> 7); }

private:	
	// [set][moved flag][not used][not used][not used][piece t][piece t][piece t]
	byte m_internalState;
};