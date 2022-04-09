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

enum class PieceType : byte
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

enum class Set : byte
{
	WHITE = 0,
	BLACK = 1,
	NR_OF_SETS = 2
};

class ChessPieceDef
{
public:
	static byte			 	MoveCount(byte pIndex);
	static bool 			Slides(byte pIndex);
	static signed short 	Moves0x88(byte pIndex, byte mIndex);
	static signed short 	Attacks0x88(byte pIndex, byte mIndex);

private:
	static byte m_moveCount[6];
	static bool m_slides[6];
	static signed short m_moves0x88[6][8];
	static signed short m_attacks0x88[6][8];
};


struct ChessPiece
{
public:
	static Set FlipSet(Set source);
	static byte FlipSet(byte source);

public:
	ChessPiece();
	ChessPiece(Set _set, PieceType _type);

	char toString() const;
	bool fromString(char piece);

	bool operator==(const ChessPiece& rhs) const;
	bool operator!=(const ChessPiece& rhs) const;
	bool operator<(const ChessPiece& rhs) const;

	PieceType getType() const	{ return static_cast<PieceType>(m_internalState & 0x07); }
	Set getSet() const 	{ return static_cast<Set>(m_internalState >> 7); }
	byte type() const		{ return m_internalState & 0x07; }
	byte set() const 		{ return m_internalState >> 7; }

private:	
	// [set][moved flag][not used][not used][not used][piece t][piece t][piece t]
	byte m_internalState;
};