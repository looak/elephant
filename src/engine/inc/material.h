// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021-2022  Alexander Loodin Ek

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
#include "chess_piece.h"
#include "notation.h"
#include <vector>
#include <array>

class Material
{
public:
	Material();
	Material(const Material& other);

	void Clear();

	Material& operator=(const Material& other);

	void AddPiece(ChessPiece piece, Notation position);
	void RemovePiece(ChessPiece piece, Notation position);
	void MovePiece(ChessPiece piece, Notation source, Notation target);
	void PromotePiece(ChessPiece piece, Notation position);
	
	/**
	* @brief
	* Unmakes a chess piece move by removing the piece from the target location and adding it back to the source location.
	* Note: In this function, the "source" and "target" arguments are backwards compared to other methods, since this function is used to undo a previous move.
	* Note: pieceToAdd and pieceToRemove can be different in the case where we are unmaking a promotion
	*
	* @param pieceToAdd		The chess piece to add to the source location.
	* @param pieceToRemove	The chess piece to remove from the target location.
	* @param source			The source location on the chess board where the pieceToAdd should be placed.
	* @param target			The target location on the chess board where the pieceToRemove should be removed from.
	*/
	void UnmakePieceMove(ChessPiece pieceToAdd, ChessPiece pieceToRemove, Notation source, Notation target);
		
	const std::vector<Notation>& getPlacementsOfPiece(ChessPiece piece) const;

	u32 getValue() const;
	size_t getCount() const;
	size_t getPieceCount(ChessPiece piece) const;
	size_t getPieceCount(PieceType pieceType) const;
	
private:
	typedef std::array<std::vector<Notation>, (size_t)PieceType::KING> MaterialGrid;

	MaterialGrid m_material;
};