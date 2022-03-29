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
#include "bitboard.h"
#include "chess_piece.h"
#include "notation.h"
#include <string>

struct Move;

struct ChessboardTile
{
	friend class Chessboard;
public:
	ChessboardTile() = default;
	ChessboardTile(Notation&& notation);
	~ChessboardTile() = default;

	const ChessPiece& readPiece() const { return m_piece; };
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
	~Chessboard() = default;

	bool PlacePiece(const ChessPiece& piece, const Notation& target);
	bool MakeMove(Move& move);
	

	const ChessboardTile& readTile(const Notation& position) const;
	ChessboardTile& editTile(const Notation& position);

	Notation& editEnPassant() { return m_enPassant; }
	const Notation& readEnPassant() const { return m_enPassant; }
	byte& editCastlingState() { return m_castlingState; }
	byte readCastlingState() const { return m_castlingState; }

	template<typename T, bool isConst = false>
	class ChessboardIterator
	{
		friend class Chessboard;
		using reference = typename std::conditional_t<isConst, const ChessboardTile&, ChessboardTile&>;
	public:
		ChessboardIterator(const ChessboardIterator& other) :
			m_chessboard(other.m_chessboard),
			m_index(other.m_index),
			m_position(other.m_position)
		{}

		ChessboardIterator(T& board) :
			m_chessboard(board),
			m_index(0)
		{}

		ChessboardIterator(T& board, Notation pos) :
			m_chessboard(board),
			m_index(0),
			m_position(std::move(pos))
		{}

		bool operator==(const ChessboardIterator& rhs) const;
		bool operator!=(const ChessboardIterator& rhs) const;

		ChessboardIterator& operator++();
		ChessboardIterator operator++(int);
		ChessboardIterator& operator+=(byte incre);

		reference operator*() const { return m_chessboard.get(m_position); }

		bool end() const;
		byte file() const { return m_position.file; }
		byte rank() const { return m_position.rank; }
		byte index() const { return m_index; }


	private:
		T& m_chessboard;
		mutable byte m_index;
		mutable Notation m_position;
	};

	using Iterator = ChessboardIterator<Chessboard, false>;
	using ConstIterator = ChessboardIterator<const Chessboard, true>;

	Iterator begin();
	Iterator end();
	ConstIterator begin() const;
	ConstIterator end() const;

private:
	bool UpdateEnPassant(const Notation& source, const Notation& target, bool wasPawnMove);
	void InternalMakeMove(const Notation& source, const Notation& target);
	int getTileIndex(byte file, byte rank);

	ChessboardTile& get(const Notation& position) { return editTile(position); }
	const ChessboardTile& get(const Notation& position) const { return readTile(position); }

	ChessboardTile m_tiles[64];
	Bitboard m_bitboard;

	// 0x01 == K, 0x02 == Q, 0x04 == k, 0x08 == q
	byte m_castlingState;
	Notation m_enPassant;
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
	return m_index >= 64;
}

template<typename T, bool isConst>
Chessboard::ChessboardIterator<T, isConst>& Chessboard::ChessboardIterator<T, isConst>::operator++()
{
	if (end()) return *this;
	++m_index;
	m_position = Notation(m_index);
	return *this;
}

template<typename T, bool isConst>
Chessboard::ChessboardIterator<T, isConst> Chessboard::ChessboardIterator<T, isConst>::operator++(int)
{
	ChessboardIterator itr(*this);
	++(*this);
	return itr;
}

template<typename T, bool isConst>
Chessboard::ChessboardIterator<T, isConst>& Chessboard::ChessboardIterator<T, isConst>::operator+=(byte incre)
{
	signed char result = m_index + incre;
	if (result < 0 || result > 63)
		result = 64;

	m_index = result;
	m_position = Notation(m_index);

	return *this;
}