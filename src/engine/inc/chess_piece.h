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

constexpr signed short pieceValues[6] = {
	100, 350, 350, 525, 1000, 10000
};
constexpr byte moveCount[6] = {
	2, 8, 4, 4, 8, 8
};

constexpr bool slides[6] = {
	false, false, true, true, true, false,
};

constexpr signed short moves0x88[6][8] = {
	{ -16, -32, 0, 0, 0, 0, 0, 0 },
	{ -33, -31, -18, -14, 14, 18, 31, 33 },
	{ -17, -15, 15, 17, 0, 0, 0, 0 },
	{ -16, -1, 1, 16, 0, 0, 0, 0 },
	{ -17, -16, -15, -1, 1, 15, 16, 17 },
	{ -17, -16, -15, -1, 1, 15, 16, 17 }
};

constexpr signed short attacks0x88[6][8] = {
	{ -15, -17, 0, 0, 0, 0, 0, 0 },
	{ -33, -31, -18, -14, 14, 18, 31, 33 },
	{ -17, -15, 15, 17, 0, 0, 0, 0 },
	{ -16, -1, 1, 16, 0, 0, 0, 0 },
	{ -17, -16, -15, -1, 1, 15, 16, 17 },
	{ -17, -16, -15, -1, 1, 15, 16, 17 }
};

const PieceType m_slidingPieceTypes[3] = {
	PieceType::BISHOP, PieceType::ROOK, PieceType::QUEEN
};

class ChessPieceDef
{
public:
	static inline constexpr byte	 			MoveCount(byte pIndex) { return moveCount[pIndex]; };
	static inline constexpr bool 				Slides(byte pIndex) { return slides[pIndex]; }
	static inline constexpr signed short 		Moves0x88(byte pIndex, byte mIndex) { return moves0x88[pIndex][mIndex]; }
	static inline constexpr signed short 		Attacks0x88(byte pIndex, byte mIndex) { return attacks0x88[pIndex][mIndex]; }
	static inline constexpr signed short		Value(byte pIndex) { return pieceValues[pIndex]; }
	static inline const PieceType*              SlidingTypes() { return &m_slidingPieceTypes[0]; }
	static inline constexpr bool				IsDiagonalMove(signed short mvValue) { return (mvValue == -17 || mvValue == -15 || mvValue == 15 || mvValue == 17); }
};


struct ChessPiece
{
public:
	static Set FlipSet(Set source);
	static byte FlipSet(byte source);
	static ChessPiece None() { return ChessPiece(); }

public:
	ChessPiece();
	ChessPiece(Set _set, PieceType _type);

	char toString() const;
	bool fromString(char piece);

	bool isPawn() const { return type() == 1; }
	bool isKnight() const { return type() == 2; }
	bool isBishop() const { return type() == 3; }
	bool isRook() const { return type() == 4; }
	bool isQueen() const { return type() == 5; }
	bool isKing() const { return type() == 6; }
	bool isWhite() const { return set() == 0; }

	bool operator==(const ChessPiece& rhs) const;
	bool operator!=(const ChessPiece& rhs) const;
	bool operator<(const ChessPiece& rhs) const;

	PieceType getType() const	{ return static_cast<PieceType>(m_internalState & 0x07); }
	Set getSet() const 	{ return static_cast<Set>(m_internalState >> 7); }
	byte type() const		{ return m_internalState & 0x07; }
	byte set() const 		{ return m_internalState >> 7; }
	byte index() const		{ return type() - 1; }
	byte raw() const		{ return m_internalState; }

	bool isValid() const { return type() > 0; }
	bool isSliding() const { return ChessPieceDef::Slides(index()); }

private:	
	// [set][not used][not used][not used][not used][piece t][piece t][piece t]
	union
	{
		byte m_internalState;
		// struct
		// {
		// 	// technically we only use 3 bits to identify the piece type but to get the correct offset to the
		// 	// set bit we have set this to 7.
		// 	PieceType m_type : 7;				
		// 	Set m_set : 1;
		// };
		
		// struct
		// {
		// 	byte m_typeValue : 7;
		// 	byte m_setValue : 1;
		// };
	};
	//byte m_internalState;
};