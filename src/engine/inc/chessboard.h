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
#include "bitboard.h"
#include "chess_piece.h"
#include "notation.h"
#include "material.h"
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
/**
 * The Chessboard class represents a chess board and its current state.
 * It provides functions for moving and placing chess pieces, as well as tracking the state of the game.
 *
 * @author Alexander Loodin Ek
 */
class Chessboard
{
public:
	Chessboard();
	~Chessboard() = default;

	Chessboard(const Chessboard& other);

	bool PlacePiece(const ChessPiece& piece, const Notation& target, bool overwrite = false);
	bool MakeMove(Move& move);

	/**
	* Makes the specified move on the chessboard. The move may be a partial move generated from a Portable Game Notation (PGN) string,
	* in which case it may not include the full move and we need the context of the chessboard to disambiguate the move.
	*
	* @param move The move to make.
	* @return The actual move that was made, which may be different from the input move if the input move was ambiguous.
	*         If the move could not be made, an invalid move is returned.
	*/
	Move PlayMove(const Move& move);
	bool UnmakeMove(const Move& move);
	
	std::tuple<bool, int> IsInCheck(Set set) const;
	bool IsInCheckmate(Set set) const;
	bool IsInStalemate(Set set) const;
	
	std::vector<Move> GetAvailableMoves(const Notation& source, const ChessPiece& piece, u64 threatenedMask, bool checked, u64 kingMask) const;
	std::vector<Move> GetAvailableMoves(Set currentSet) const;
	
	u64 GetThreatenedMask(Set set) const;	
	u64 GetKingMask(Set set) const;
	u64 GetSlidingMask(Set set) const;

	const Notation& readKingPosition(Set set) const;

	const ChessboardTile& readTile(const Notation& position) const;
	ChessboardTile& editTile(const Notation& position);
	ChessPiece readPieceAt(Notation notation) const;

	bool setEnPassant(const Notation& notation);
	bool setCastlingState(u8 castlingState);

	const Notation& readEnPassant() const { return m_enPassant; }
	byte readCastlingState() const { return m_castlingState; }

	u64 readHash() const { return m_hash; }

	const Material& readMaterial(Set set) const { return m_material[(size_t)set]; }

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
	/**
	* Internal helper function for handling the movement of a pawn chess piece.
	*
	* @param move The move being made.
	* @return The updated target location for the pawn, in case we double moved the piece and target differ.*/	
	Notation InternalHandlePawnMove(Move& move);
	void InternalHandleRookMove(Move& move, const Notation& targetRook, const Notation& rookMove);
	
	/**
	* Internal helper function for handling the movement of a king chess piece.
	*
	* @param move The move being made.
	* @param targetRook The position of the rook that will be involved in the castle move (if any).
	* @param rookMove The position that the rook will move to during the castle move (if any).
	* @return True if the move is a castle move, false otherwise. */
	bool InternalHandleKingMove(Move& move, Notation& targetRook, Notation& rookMove);
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
			ChessboardTile A1, B1, C1, D1, E1, F1, G1, H1;
			ChessboardTile A2, B2, C2, D2, E2, F2, G2, H2;
			ChessboardTile A3, B3, C3, D3, E3, F3, G3, H3;
			ChessboardTile A4, B4, C4, D4, E4, F4, G4, H4;
			ChessboardTile A5, B5, C5, D5, E5, F5, G5, H5;
			ChessboardTile A6, B6, C6, D6, E6, F6, G6, H6;
			ChessboardTile A7, B7, C7, D7, E7, F7, G7, H7;
			ChessboardTile A8, B8, C8, D8, E8, F8, G8, H8;
		} m_tilesNamed;
	};
	
	Bitboard m_bitboard;

	// caching kings and their locations
	std::pair<ChessPiece, Notation> m_kings[2];

	// 0x01 == K, 0x02 == Q, 0x04 == k, 0x08 == q
	byte m_castlingState;
	Notation m_enPassant;
	Notation m_enPassantTarget;

	Material m_material[(size_t)Set::NR_OF_SETS];
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