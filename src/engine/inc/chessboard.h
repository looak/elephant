// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021-2023  Alexander Loodin Ek

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
#include <utility>

struct Move;

// 0x01 == K, 0x02 == Q, 0x04 == k, 0x08 == q
enum CastlingState
{
	NONE = 0x00,
	WHITE_KINGSIDE = 0x01,
	WHITE_QUEENSIDE = 0x02,
	WHITE_ALL = WHITE_KINGSIDE | WHITE_QUEENSIDE,
	BLACK_KINGSIDE = 0x04,
	BLACK_QUEENSIDE = 0x08,
	BLACK_ALL = BLACK_KINGSIDE | BLACK_QUEENSIDE,
	ALL = WHITE_ALL | BLACK_ALL
};

struct CastlingStateInfo
{
public:
	CastlingStateInfo() = default;
	
	bool hasAll() const { return m_innerState == ALL; }
	bool hasAny() const { return m_innerState != NONE; }
	bool hasWhite() const { return m_innerState & WHITE_ALL; }
	bool hasBlack() const { return m_innerState & BLACK_ALL; }
	bool hasWhiteKingSide() const { return m_innerState & WHITE_KINGSIDE; }
	bool hasWhiteQueenSide() const { return m_innerState & WHITE_QUEENSIDE; }
	bool hasBlackKingSide() const { return m_innerState & BLACK_KINGSIDE; }
	bool hasBlackQueenSide() const { return m_innerState & BLACK_QUEENSIDE; }

	void clear() { m_innerState = NONE; }
	void unsetWhite() { m_innerState &= ~WHITE_ALL; }
	void unsetBlack() { m_innerState &= ~BLACK_ALL; }
	void unsetWhiteKingSide() { m_innerState &= ~WHITE_KINGSIDE; }
	void unsetWhiteQueenSide() { m_innerState &= ~WHITE_QUEENSIDE; }
	void unsetBlackKingSide() { m_innerState &= ~BLACK_KINGSIDE; }
	void unsetBlackQueenSide() { m_innerState &= ~BLACK_QUEENSIDE; }
	
	void setAll() { m_innerState = ALL; }
	void setWhite() { m_innerState |= WHITE_ALL; }
	void setBlack() { m_innerState |= BLACK_ALL; }
	void setWhiteKingSide() { m_innerState |= WHITE_KINGSIDE; }
	void setWhiteQueenSide() { m_innerState |= WHITE_QUEENSIDE; }
	void setBlackKingSide() { m_innerState |= BLACK_KINGSIDE; }
	void setBlackQueenSide() { m_innerState |= BLACK_QUEENSIDE; }

	std::string toString() const;
		
private:
	union {
		byte m_innerState;
		struct
		{
			bool m_whiteKingSide : 1;
			bool m_whiteQueenSide : 1;
			bool m_blackKingSide : 1;
			bool m_blackQueenSide : 1;
		};
	};	
};

struct ChessboardTile
{
	friend class Chessboard;
public:
	ChessboardTile();
	ChessboardTile(Notation&& notation);
	~ChessboardTile() = default;
	
	ChessPiece readPiece() const { return m_piece; };
	ChessPiece& editPiece() { return m_piece; };

	Notation readPosition() const { return m_position; };

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

	void Clear();
	bool PlacePiece(ChessPiece piece, Notation target, bool overwrite = false);

	bool MakeMove(Move& move);
	bool MakeMoveUnchecked(Move& move);
	bool UnmakeMove(const Move& move);

	/**
	 * @brief Takes a move and serializes it to a unambigous Portable Game Notation (PGN) string for this board position.
	 * @param move The move to serialize.
	 * @return The serialized move.	*/
	std::string SerializeMoveToPGN(const Move& move) const;
	/**
	 * @brief Takes a short algebraic notation (SAN) string and deserializes it to a move for this board position.
	 * @param sanMove The SAN string to deserialize.
	 * @param isWhiteMove Whether the move is a white move or not.
	 * @return The deserialized move.	*/
	Move DeserializeMoveFromPGN(const std::string& pgnMove, bool isWhiteMove) const;
	
	std::tuple<bool, int, u64> calcualteCheckedCount(Set set) const;
	bool isChecked(Set set) const;
	bool isCheckmated(Set set) const;
	bool isStalemated(Set set) const;
	
	std::vector<Move> GetAvailableMoves(Notation source, ChessPiece piece, u64 threatenedMask, u64 checkedMask, u64 kingMask) const;

	/**
	 * Calculates the available moves for the specified set. Does not generate 100% legal moves. For legal moves refere to the MoveGenerator class.
	 * 
	 * @param currentSet The set to calculate the available moves for.
	 * @return A vector of all the moves for the specified set - might not be legal moves.	 */
	std::vector<Move> GetAvailableMoves(Set currentSet) const;
	
	/**
	 * @brief Calculates a bitboard which shows opponents available moves, i.e. threatened squares.
	 * @param set The set to calculate the threat against.
	*/
	u64 CalcThreatenedMask(Set set) const;
	u64 GetKingMask(Set set) const;
	/**
	 * Computes and returns two bitboards that represent all the squares that are threatened by the sliding pieces
	 * (rooks, bishops, and queens) of a specified set (black or white) on the current board, taking into account the
	 * current material on the board. The first bitboard represents squares that are threatened by sliding pieces moving
	 * orthogonally, and the second represents squares that are threatened by sliding pieces moving diagonally.
	 *
	 * @param set The set of pieces (black or white) to consider.
	 * @return A pair of bitboards representing the squares that are threatened by sliding pieces moving orthogonally and
	 *         diagonally, respectively.	 */
	MaterialMask GetSlidingMaskWithMaterial(Set set) const;
	MaterialMask GetMaterialMask(Set set) const;

	Notation readKingPosition(Set set) const;

	const ChessboardTile& readTile(Notation position) const;
	ChessboardTile& editTile(Notation position);
	ChessPiece readPieceAt(Notation notation) const;

	bool setEnPassant(Notation notation);

	Notation readEnPassant() const { return m_enPassant; }
	
	bool setCastlingState(u8 castlingState);
	byte readCastlingState() const { return m_castlingState; }
	const CastlingStateInfo& readCastlingStateInfo() const { return m_castlingInfo; }
	CastlingStateInfo& editCastlingState() { return m_castlingInfo; }
	
	u64 readHash() const { return m_hash; }

	const Material& readMaterial(Set set) const { return m_material[(size_t)set]; }
	const Bitboard& readBitboard() const { return m_bitboard; }

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
	void InternalHandleRookMove(Move& move, Notation targetRook, Notation rookMove);
	void InternalHandleRookMovedOrCaptured(Move& move, Notation rookSquare);
	void UpdateCastlingState(Move& move, byte mask);
	
	/**
	* Internal helper function for handling the movement of a king chess piece.
	*
	* @param move The move being made.
	* @param targetRook The position of the rook that will be involved in the castle move (if any).
	* @param rookMove The position that the rook will move to during the castle move (if any).
	* @return True if the move is a castle move, false otherwise. */
	bool InternalHandleKingMove(Move& move, Notation& targetRook, Notation& rookMove);
	void InternalHandleKingRookMove(Move& move);
	void InternalHandleCapture(Move& move, Notation pieceTarget);

	bool InternalIsMoveCheck(Move& move) const;

	bool UpdateEnPassant(Notation source, Notation target);
	void InternalMakeMove(Notation source, Notation target);
	void InternalUnmakeMove(Notation source, Notation target, ChessPiece pieceToRmv, ChessPiece pieceToAdd);

	std::vector<Move> concurrentCalculateAvailableMovesForPiece(ChessPiece piece, u64 threatenedMask, u64 kingMask, u64 checkedMask) const;

	ChessboardTile& get(Notation position) { return editTile(position); }
	const ChessboardTile& get(Notation position) const { return readTile(position); }

	bool IsMoveCastling(const Move& move) const;
	bool IsPromoting(const Move& move) const;
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
	
	mutable Bitboard m_bitboard;

	// caching kings and their locations
	std::pair<ChessPiece, Notation> m_kings[2];

	union
	{
		// 0x01 == K, 0x02 == Q, 0x04 == k, 0x08 == q
		byte m_castlingState;
		CastlingStateInfo m_castlingInfo;
	};
	
	// when a pawn moves 2 squares, this is the square it can be captured on
	Notation m_enPassant;
	// the square the pawn moved to when it moved 2 squares
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