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
	explicit Notation(const Notation& other) = default;


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

	const ChessPiece& getPiece() const { return m_piece; };
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
	friend class Iterator;
public:
	Chessboard();
	Chessboard(const Chessboard& other);
	Chessboard(const Chessboard&& other) = delete;
	~Chessboard() = default;

	void Clear();
	void Setup();

	bool PlacePiece(ChessPiece p, const Notation& tile);

	const ChessboardTile& readTile(const Notation& position) const;
	ChessboardTile& editTile(const Notation& position);
	
	template<typename T, bool isConst = false>
	class ChessboardIterator
	{
		friend class Chessboard;
		using reference = typename std::conditional_t<isConst, const ChessboardTile&, ChessboardTile&>;
	public:
		ChessboardIterator(const ChessboardIterator& other) :
			m_chessboard(other.m_chessboard),
			m_position(other.m_position)
		{

		}

		ChessboardIterator(T& board) :
			m_chessboard(board)
		{}

		ChessboardIterator(T& board, Notation pos) :
			m_chessboard(board),
			m_position(std::move(pos))
		{}

		bool operator==(const ChessboardIterator& rhs) const;
		bool operator!=(const ChessboardIterator& rhs) const;
		ChessboardIterator& operator++()
		{
			if (end()) return *this;

			m_position.file++;
			if (m_position.file > 7)
			{
				m_position.file = 0;
				m_position.rank++;
			}

			return *this;
		}

		ChessboardIterator operator++(int)
		{
			ChessboardIterator itr(*this);
			++(*this);
			return itr;
		}

		bool end() const;
		byte file() const { return m_position.file; }
		byte rank() const { return m_position.rank; }
		byte index() const { return m_position.getIndex(); }
					
		reference operator*() const
		{
			return m_chessboard.get(m_position);
		}

	private:
		T& m_chessboard;
		mutable Notation m_position;
	};

	using Iterator = ChessboardIterator<Chessboard, false>;
	using ConstIterator = ChessboardIterator<const Chessboard, true>;

	Iterator begin();
	Iterator end();
	ConstIterator begin() const;
	ConstIterator end() const;

private:
	int getTileIndex(byte file, byte rank);

	ChessboardTile& get(const Notation& position) { return editTile(position); }
	const ChessboardTile& get(const Notation& position) const { return readTile(position); }

	ChessboardTile m_tiles[64];
};

template<typename T, bool isConst>
bool Chessboard::ChessboardIterator<T, isConst>::operator==(const ChessboardIterator<T, isConst>& rhs) const
{
	return &m_chessboard == &rhs.m_chessboard && m_position == rhs.m_position;
}

template<typename T, bool isConst>
bool Chessboard::ChessboardIterator<T, isConst>::operator!=(const ChessboardIterator<T, isConst>& rhs) const
{
	return !(*this == rhs);
}

template<typename T, bool isConst>
bool Chessboard::ChessboardIterator<T, isConst>::end() const
{
	return m_position.rank >= 8;
}