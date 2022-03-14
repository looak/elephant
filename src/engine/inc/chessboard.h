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
#include <string>

struct Notation
{
	static Notation BuildPosition(byte file, byte rank);
	// Should Notation validate rank & file?
	// Should Notation validate format of rank & file?
	Notation() :
		file(0xF),
		rank(0xF)
	{}

	Notation(byte _file, byte _rank) :
		file(_file),
		rank(_rank)
	{}

	Notation(Notation&& other) = default;


	byte file : 4;
	byte rank : 4;
	

	byte getIndex() const { return (rank * 8) + file; }
	bool operator==(const Notation& rhs) const;
	Notation& operator=(Notation&& other);
};

struct ChessboardTile
{
	friend class Chessboard;
public:
	ChessboardTile() = default;
	ChessboardTile(Notation&& notation);
	~ChessboardTile() = default;

	const ChessPiece& getPiece() { return m_piece; };
	ChessPiece& editPiece() { return m_piece; };

	const Notation& getPosition() { return m_position; };

	bool operator==(const ChessboardTile& rhs) const;

private:
	Notation& editPosition() { return m_position; };

	Notation m_position;
	ChessPiece m_piece;
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

	bool PlacePiece(ChessPiece p, const Notation& tile);

	const ChessPiece& readTile(const Notation& position) const;
	ChessboardTile& editTile(const Notation& position);
	//const ChessPiece& operator[](byte index) const;
	//const ChessboardTile& getTile(const Notation& position) const;
	//ChessboardTile& editTile(const Notation& position);
	class Iterator
	{
		Iterator(Chessboard& chessboard);
		void operator++();
	//	ChessboardTile& operator->();
		ChessboardTile& operator*();

		// iterator comparison
		// iterator end
		// const itr?

	private:
		ChessboardTile& get();
		Chessboard& m_chessboard;
		Notation m_position;
		bool m_end;
	};

private:
	int getTileIndex(byte file, byte rank);

	ChessboardTile m_tiles[64];
};