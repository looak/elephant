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
#include <queue>
#include <string>
#include <utility>
#include <vector>
#include "chess_piece.h"
#include "move.h"
#include "notation.h"
#include "position.hpp"

struct Move;
struct PrioratizedMove;
struct PrioratizedMoveComparator;

struct MoveUndoUnit {
    PackedMove move;
    ChessPiece capturedPiece;
    CastlingStateInfo castlingState;
    EnPassantStateInfo enPassantState;
    u64 hash;
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
 * It provides functions for moving and placing chess pieces, and updates
 * all underlying state accordingly.
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

    template<bool validation>
    MoveUndoUnit MakeMove(const PackedMove move, ChessPiece pieceToPromoteTo = ChessPiece::None());

    template<typename... placementpairs>
    bool PlacePieces(placementpairs... placements);

    // bool MakeMove(Move& move);
    // bool MakeMoveUnchecked(Move& move);
    // bool UnmakeMove(const Move& move);

    // bool MakeNullMove(Move& move);
    // bool UnmakeNullMove(const Move& move);

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

    std::vector<Move> GetAvailableMoves(Notation source, ChessPiece piece, u64 threatenedMask, KingMask checkedMask,
                                        KingMask kingMask, bool captureMoves = false) const;

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
     * @param set The set to calculate the threat against.
     * Should be removed!*/
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
        using reference = typename std::conditional_t<isConst, const ChessboardTile&, ChessboardTile&>;

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
    ChessboardTile& editTile(Notation position);
    const Position& readPosition() const { return m_position; }

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
    CastlingStateInfo readCastlingState() const { return m_position.readCastling(); }
    u64 readHash() const { return m_hash; }
    const ChessboardTile& readTile(Notation position) const;
    ChessPiece readPieceAt(Notation notation) const;
    Notation readKingPosition(Set set) const;

    bool isChecked(Set set) const;
    bool isCheckmated(Set set) const;
    bool isStalemated(Set set) const;
    std::string toString() const;

private:
    template<typename piece, typename square, typename... placements>
    bool InternalProcessEvenPlacementPairs(const piece& p, const square& sqr, const placements&... _placements);

    bool InternalProcessEvenPlacementPairs() { return true; }

    /**
     * Internal helper function for handling the movement of a pawn chess piece.
     *
     * @param move The move being made.
     * @return The updated target location for the pawn, in case we double moved the piece and
     * target differ.*/
    Square InternalHandlePawnMove(const PackedMove move, MoveUndoUnit& undoState);
    void InternalHandleRookMove(const ChessPiece piece, const PackedMove move, Notation targetRook, Notation rookMove,
                                MoveUndoUnit& undoState);
    void InternalHandleRookMovedOrCaptured(const PackedMove move, Notation rookSquare, MoveUndoUnit& undoState);
    void InternalUpdateCastlingState(const PackedMove move, byte mask, MoveUndoUnit& undoState);

    /**
     * Internal helper function for handling the movement of a king chess piece.
     *
     * @param move The move being made.
     * @param set The set of the king being moved.
     * @param targetRook The position of the rook that will be involved in the castle move (if any).
     * @param rookMove The position that the rook will move to during the castle move (if any).
     * @return True if the move is a castle move, false otherwise. */
    bool InternalHandleKingMove(const PackedMove move, Set set, Notation& targetRook, Notation& rookMove,
                                MoveUndoUnit& undoState);
    void InternalHandleKingRookMove(const ChessPiece piece, const PackedMove move, MoveUndoUnit& undoState);
    void InternalHandleCapture(const PackedMove move, const Notation pieceTarget, MoveUndoUnit& undoState);

    bool InternalUpdateEnPassant(Notation source, Notation target);
    void InternalMakeMove(Notation source, Notation target);
    void InternalUnmakeMove(Notation source, Notation target, ChessPiece pieceToRmv, ChessPiece pieceToAdd);

    std::vector<Move> concurrentCalculateAvailableMovesForPiece(ChessPiece piece, u64 threatenedMask, KingMask kingMask,
                                                                KingMask checkedMask, bool captureMoves) const;

    ChessboardTile& get(Notation position) { return editTile(position); }
    const ChessboardTile& get(Notation position) const { return readTile(position); }

    bool InternalIsMoveCheck(Move& move) const;
    int IsMoveCastling(const Move& move) const;
    bool IsPromoting(const Move& move) const;
    bool VerifyMove(const Move& move) const;

    u64 m_hash;

    ChessboardTile m_tiles[64];

    Position m_position;

    // caching kings and their locations
    std::pair<ChessPiece, Notation> m_kings[2];

    mutable std::array<std::tuple<bool, KingMask>, 2> m_cachedKingMask{};
};

template<typename T, bool isConst>
bool
Chessboard::ChessboardIterator<T, isConst>::operator==(const ChessboardIterator<T, isConst>& rhs) const
{
    return &m_chessboard == &rhs.m_chessboard && m_position == rhs.m_position;
}

template<typename T, bool isConst>
bool
Chessboard::ChessboardIterator<T, isConst>::operator!=(const ChessboardIterator<T, isConst>& rhs) const
{
    return !(*this == rhs);
}

template<typename T, bool isConst>
bool
Chessboard::ChessboardIterator<T, isConst>::end() const
{
    return m_index >= 64;
}

template<typename T, bool isConst>
Chessboard::ChessboardIterator<T, isConst>&
Chessboard::ChessboardIterator<T, isConst>::operator++()
{
    if (end())
        return *this;
    ++m_index;
    m_position = Notation(m_index);
    return *this;
}

template<typename T, bool isConst>
Chessboard::ChessboardIterator<T, isConst>
Chessboard::ChessboardIterator<T, isConst>::operator++(int)
{
    ChessboardIterator itr(*this);
    ++(*this);
    return itr;
}

template<typename T, bool isConst>
Chessboard::ChessboardIterator<T, isConst>&
Chessboard::ChessboardIterator<T, isConst>::operator+=(int incre)
{
    int temp_index = static_cast<int>(m_index);
    int result = temp_index + incre;
    if (result < 0 || result > 63)
        result = 64;

    m_index = static_cast<unsigned char>(result);
    m_position = Notation(m_index);
    return *this;
}

template<typename piece, typename square, typename... placements>
bool
Chessboard::InternalProcessEvenPlacementPairs(const piece& p, const square& sqr, const placements&... _placements)
{
    if (PlacePiece(p, sqr) == false)
        return false;

    return InternalProcessEvenPlacementPairs(_placements...);
}

template<typename... placements>
bool
Chessboard::PlacePieces(placements... _placement)
{
    static_assert(sizeof...(_placement) % 2 == 0, "Number of arguments must be even");
    return InternalProcessEvenPlacementPairs(_placement...);
}