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
#include <functional>
#include "defines.h"
#include "chess_piece.h"

struct Notation;

class Bitboard
{
public:
	static bool IsValidSquare(signed short currSqr);
	static bool IsValidSquare(const Notation& source);
public:
	Bitboard();
		
	bool PlacePiece(const ChessPiece& piece, const Notation& target);
	bool ClearPiece(const ChessPiece& piece, const Notation& target);
	bool IsValidMove(const Notation& source, const ChessPiece& piece, const Notation& target, byte castling, byte enPassant, u64 threatenedMask) const;

	u64 GetAvailableMoves(const Notation& source, const ChessPiece& piece, byte castling = 0x0, byte enPassant = 0xff, u64 threatenedMask = 0) const;
	u64 GetAttackedSquares(const Notation& source, const ChessPiece& piece) const;
	u64 GetThreatenedSquares(const Notation& source, const ChessPiece& piece) const;
	u64 GetAttackedSquares(Set set);

private:
typedef std::function<bool(u64 sqrMask)> ResolveMask;
	u64 MaterialCombined() const;
	u64 MaterialCombined(byte set) const;
	u64 Castling(byte set, byte castling) const;
	bool IsValidPawnMove(byte srcSqr, byte trgSqr, byte set);
	u64 GetAvailableMovesForPawn(u64 mat, u64 opMat, const Notation& source, const ChessPiece& piece, byte enPassant) const;
	u64 GetAvailableMovesForKing(u64 mat, u64 threatenedMask, const Notation& source, const ChessPiece& piece, byte castling) const;

	u64 InternalGenerateMask(byte curSqr, signed short dir, bool& sliding, ResolveMask func) const;

	u64 m_material[2][6];
};