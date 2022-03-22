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
#include <defines.h>

struct Notation;
struct ChessPiece;

class Bitboard
{
public:
	static bool IsValidSquare(signed short currSqr);
	static bool IsValidSquare(const Notation& source);
	bool PlacePiece(const ChessPiece& piece, const Notation& target);
	bool IsValidMove(const Notation& source, const ChessPiece& piece, const Notation& target);

	u64 GetAvailableMoves(const Notation& source, const ChessPiece& piece);
	u64 GetAttackedSquares(const Notation& source, const ChessPiece& piece);

private:
	bool IsValidPawnMove(byte srcSqr, byte trgSqr, byte set);

	u64 m_material[2][6];
};