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
#include <vector>
#include <array>

struct Move;

struct ChessboardTile
{
	friend class Chessboard;
public:
	ChessboardTile();
	ChessboardTile(Notation&& notation);
	~ChessboardTile() = default;
	
	const ChessPiece& readPiece() const { return m_piece; };
	ChessPiece& editPiece() { return m_piece; };

	const Notation& readPosition() const { return m_position; };

	bool operator==(const ChessboardTile& rhs) const;
	ChessboardTile& operator=(const ChessboardTile& rhs);

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

	Chessboard(const Chessboard& other);

	bool PlacePiece(const ChessPiece& piece, const Notation& target, bool overwrite = false);
	bool MakeMove(Move& move);
	Move MakeMove(const Move& move);
	bool UnmakeMove(const Move& move);
	bool Checked(Set set) const;

	

	std::vector<Move> GetAvailableMoves(const Notation& source, const ChessPiece& piece, u64 threatenedMask, bool checked, u64 kingMask) const;
	std::vector<Move> GetAvailableMoves(Set currentSet) const;
	u64 GetThreatenedMask(Set set) const;
	u64 GetKingMask(Set set) const;
	u64 GetSlidingMask(Set set) const;

	const ChessboardTile& readTile(const Notation& position) const;
	ChessboardTile& editTile(const Notation& position);
	ChessPiece readPieceAt(Notation notation) const;

	bool setEnPassant(const Notation& notation);
	bool setCastlingState(u8 castlingState);

	const Notation& readEnPassant() const { return m_enPassant; }
	byte readCastlingState() const { return m_castlingState; }

	u64 readHash() const { return m_hash; }

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
		ChessboardIterator& operator+=(int incre);

		reference operator*() const { return get(); }
		reference get() const { return m_chessboard.get(m_position); }

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
	Notation InternalHandlePawnMove(Move& move);
	void InternalHandleRookMove(Move& move, const Notation& targetRook, const Notation& rookMove);
	void InternalHandleKingMove(Move& move, Notation& targetRook, Notation& rookMove);
	void InternalHandleKingRookMove(Move& move);
	bool UpdateEnPassant(const Notation& source, const Notation& target);
	void InternalMakeMove(const Notation& source, const Notation& target);
	void InternalUnmakeMove(const Notation& source, const Notation& target, const ChessPiece& pieceToRmv, const ChessPiece& pieceToAdd);
	int getTileIndex(byte file, byte rank);

	ChessboardTile& get(const Notation& position) { return editTile(position); }
	const ChessboardTile& get(const Notation& position) const { return readTile(position); }

	bool IsMoveCastling(const Move& move) const;
	bool IsPromoting(const Move& move) const;
	bool IsCheck(const Move& move) const;
	bool VerifyMove(const Move& move) const;

	u64 m_hash;
	
	union {
		ChessboardTile m_tiles[64];
		ChessboardTile m_tiles8x8[8][8];
		struct 
		{
			ChessboardTile m_a1, m_b1, m_c1, m_d1, m_e1, m_f1, m_g1, m_h1;
			ChessboardTile m_a2, m_b2, m_c2, m_d2, m_e2, m_f2, m_g2, m_h2;
			ChessboardTile m_a3, m_b3, m_c3, m_d3, m_e3, m_f3, m_g3, m_h3;
			ChessboardTile m_a4, m_b4, m_c4, m_d4, m_e4, m_f4, m_g4, m_h4;
			ChessboardTile m_a5, m_b5, m_c5, m_d5, m_e5, m_f5, m_g5, m_h5;
			ChessboardTile m_a6, m_b6, m_c6, m_d6, m_e6, m_f6, m_g6, m_h6;
			ChessboardTile m_a7, m_b7, m_c7, m_d7, m_e7, m_f7, m_g7, m_h7;
			ChessboardTile m_a8, m_b8, m_c8, m_d8, m_e8, m_f8, m_g8, m_h8;
		} m_individualTiles;
	};
	
	Bitboard m_bitboard;

	// caching kings and their locations
	std::pair<ChessPiece, Notation> m_kings[2];

	// 0x01 == K, 0x02 == Q, 0x04 == k, 0x08 == q
	byte m_castlingState;
	Notation m_enPassant;
	Notation m_enPassantTarget;
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
Chessboard::ChessboardIterator<T, isConst>& Chessboard::ChessboardIterator<T, isConst>::operator+=(int incre)
{
	int temp_index = static_cast<int>(m_index);	
	int result = temp_index + incre;
	if (result < 0 || result > 63)
		result = 64;

	m_index = static_cast<unsigned char>(result);
	m_position = Notation(m_index);
	return *this;
}