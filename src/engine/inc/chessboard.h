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
#include <array>
#include <string>
#include <utility>
#include <vector>
#include "bitboard.h"
#include "chess_piece.h"
#include "material.h"
#include "notation.h"

struct Move;

// enum ChessboardPrint
// {
//     NONE = 0x00,
//     MOVE = 0x01,
//     CASTLING = 0x02,
//     EN_PASSANT = 0x04,
//     HASH = 0x08,
//     FEN = 0x10,
//     ALL = MOVE | CASTLING | EN_PASSANT | HASH | FEN
// }

// 0x01 == K, 0x02 == Q, 0x04 == k, 0x08 == q
enum CastlingState {
    NONE = 0x00,
    WHITE_KINGSIDE = 0x01,
    WHITE_QUEENSIDE = 0x02,
    WHITE_ALL = WHITE_KINGSIDE | WHITE_QUEENSIDE,
    BLACK_KINGSIDE = 0x04,
    BLACK_QUEENSIDE = 0x08,
    BLACK_ALL = BLACK_KINGSIDE | BLACK_QUEENSIDE,
    ALL = WHITE_ALL | BLACK_ALL
};

struct CastlingStateInfo {
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
    byte m_innerState;
};

struct ChessboardTile {
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
 * It provides functions for moving and placing chess pieces, as well as
 * tracking the state of the game.
 *
 * @author Alexander Loodin Ek
 */
class Chessboard {
public:
    Chessboard();
    ~Chessboard() = default;
    Chessboard(const Chessboard& other);

    void Clear();
    bool PlacePiece(ChessPiece piece, Notation target, bool overwrite = false);

    bool MakeMove(Move& move);
    bool MakeMoveUnchecked(Move& move);
    bool UnmakeMove(const Move& move);

    bool MakeNullMove(Move& move);
    bool UnmakeNullMove(const Move& move);

    /**
     * @brief Takes a move and serializes it to a unambigous Portable Game
     * Notation (PGN) string for this board position.
     * @param move The move to serialize.
     * @return The serialized move.	*/
    std::string SerializeMoveToPGN(const Move& move) const;
    /**
     * @brief Takes a short algebraic notation (SAN) string and deserializes it
     * to a move for this board position.
     * @param sanMove The SAN string to deserialize.
     * @param isWhiteMove Whether the move is a white move or not.
     * @return The deserialized move.	*/
    Move DeserializeMoveFromPGN(const std::string& pgnMove, bool isWhiteMove) const;

    std::tuple<int, KingMask> calcualteCheckedCount(Set set) const;

    /**
     * @brief Calculates the end game coeficient.
     * Used for tapered evaluation.
     * @return a float between 0 and 1 where 1 is endgame and 0 is midgame.     */
    float calculateEndGameCoeficient() const;

    std::vector<Move> GetAvailableMoves(Notation source,
                                        ChessPiece piece,
                                        u64 threatenedMask,
                                        KingMask checkedMask,
                                        KingMask kingMask,
                                        bool captureMoves = false) const;

    /**
     * Calculates the available legal moves for the specified set.
     * @param currentSet The set to calculate the available moves for.
     * @param captureMoves if true, will only return capture moves, by default
     * false.
     * @return A vector of all the moves for the specified set - might not be legal moves.	 */
    std::vector<Move> GetAvailableMoves(Set currentSet, bool captureMoves = false) const;

    /**
     * @brief Calculates a bitboard which shows opponents available moves, i.e.
     * threatened squares.
     * @param set The set to calculate the threat against.     */
    u64 calculateThreatenedMask(Set set) const;

    /**
     * @brief Calculates a mask from the kings point of view, figuring out if the king is threatened
     * or in check.
     * @param set The set to of the kin we are interested in.
     * @tparam useCache Whether to use the cache or not, default is true
     * @return The king mask    */
    template<bool useCache = true>
    KingMask calcKingMask(Set set) const;

    template<typename T, bool isConst = false>
    class ChessboardIterator {
        friend class Chessboard;
        using reference =
            typename std::conditional_t<isConst, const ChessboardTile&, ChessboardTile&>;

    public:
        ChessboardIterator(const ChessboardIterator& other) :
            m_chessboard(other.m_chessboard),
            m_index(other.m_index),
            m_position(other.m_position)
        {
        }

        ChessboardIterator(T& board) :
            m_chessboard(board),
            m_index(0)
        {
        }

        ChessboardIterator(T& board, Notation pos) :
            m_chessboard(board),
            m_index(0),
            m_position(std::move(pos))
        {
        }

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

    /**
     * @brief Sets the en passant square.
     * Sets the en passant square, updates hash and calculates the en passant target square.
     * @param notation The notation of the en passant square.
     * @return true if the en passant square was set */
    bool setEnPassant(Notation notation);
    bool setCastlingState(u8 castlingState);
    CastlingStateInfo& editCastlingState() { return m_castlingInfo; }
    ChessboardTile& editTile(Notation position);
    const Bitboard& readBitboard() const { return m_bitboard; }
    const Material& readMaterial(Set set) const { return m_material[(size_t)set]; }

    /**
     * @brief the sliding material of given set represented ini a bitboard.
     *
     * Computes and returns two bitboards that represent all the squares that are threatened by the
     * sliding pieces (rooks, bishops, and queens) of a specified set (black or white) on the
     * current board, taking into account the current material on the board. The first bitboard
     * represents squares that are threatened by sliding pieces moving orthogonally, and the second
     * represents squares that are threatened by sliding pieces moving diagonally.
     *
     * @param set The set of pieces (black or white) to consider.
     * @return A pair of bitboards representing the squares that are threatened by sliding pieces
     * moving orthogonally and diagonally, respectively.     */
    MaterialSlidingMask readSlidingMaterialMask(Set set) const;
    Notation readEnPassant() const { return m_enPassant; }
    byte readCastlingState() const { return m_castlingState; }
    const CastlingStateInfo& readCastlingStateInfo() const { return m_castlingInfo; }
    u64 readHash() const { return m_hash; }
    const ChessboardTile& readTile(Notation position) const;
    ChessPiece readPieceAt(Notation notation) const;
    Notation readKingPosition(Set set) const;

    bool isChecked(Set set) const;
    bool isCheckmated(Set set) const;
    bool isStalemated(Set set) const;
    std::string toString() const;

private:
    /**
     * Internal helper function for handling the movement of a pawn chess piece.
     *
     * @param move The move being made.
     * @return The updated target location for the pawn, in case we double moved the piece and
     * target differ.*/
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
    void InternalUnmakeMove(Notation source,
                            Notation target,
                            ChessPiece pieceToRmv,
                            ChessPiece pieceToAdd);

    std::vector<Move> concurrentCalculateAvailableMovesForPiece(ChessPiece piece,
                                                                u64 threatenedMask,
                                                                KingMask kingMask,
                                                                KingMask checkedMask,
                                                                bool captureMoves) const;

    ChessboardTile& get(Notation position) { return editTile(position); }
    const ChessboardTile& get(Notation position) const { return readTile(position); }

    int IsMoveCastling(const Move& move) const;
    bool IsPromoting(const Move& move) const;
    bool VerifyMove(const Move& move) const;

    u64 m_hash;

    ChessboardTile m_tiles[64];

    mutable Bitboard m_bitboard;

    // caching kings and their locations
    std::pair<ChessPiece, Notation> m_kings[2];

    union {
        // 0x01 == K, 0x02 == Q, 0x04 == k, 0x08 == q
        byte m_castlingState;
        CastlingStateInfo m_castlingInfo;
    };

    // when a pawn moves 2 squares, this is the square it can be captured on
    Notation m_enPassant;
    // the square the pawn moved to when it moved 2 squares
    Notation m_enPassantTarget;

    std::array<Material, 2> m_material;

    mutable std::array<std::tuple<bool, KingMask>, 2> m_cachedKingMask{};
};

template<typename T, bool isConst>
bool Chessboard::ChessboardIterator<T, isConst>::operator==(
    const ChessboardIterator<T, isConst>& rhs) const
{
    return &m_chessboard == &rhs.m_chessboard && m_position == rhs.m_position;
}

template<typename T, bool isConst>
bool Chessboard::ChessboardIterator<T, isConst>::operator!=(
    const ChessboardIterator<T, isConst>& rhs) const
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
    if (end())
        return *this;
    ++m_index;
    m_position = Notation(m_index);
    return *this;
}

template<typename T, bool isConst>
Chessboard::ChessboardIterator<T, isConst> Chessboard::ChessboardIterator<T, isConst>::operator++(
    int)
{
    ChessboardIterator itr(*this);
    ++(*this);
    return itr;
}

template<typename T, bool isConst>
Chessboard::ChessboardIterator<T, isConst>& Chessboard::ChessboardIterator<T, isConst>::operator+=(
    int incre)
{
    int temp_index = static_cast<int>(m_index);
    int result = temp_index + incre;
    if (result < 0 || result > 63)
        result = 64;

    m_index = static_cast<unsigned char>(result);
    m_position = Notation(m_index);
    return *this;
}