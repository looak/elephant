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
#include "chess_piece.h"

enum Column { A, B, C, D, E, F, G, H };

struct ChessboardTile
{
	ChessPiece Piece;
};

class Chessboard
{
public:
	Chessboard();
	Chessboard(const Chessboard& other);
	Chessboard(const Chessboard&& other) = delete;
	~Chessboard() = default;

	void Clear();
	void Setup();

	void getTile(char annoation[2]);

private:
	ChessboardTile m_tiles[64];
};