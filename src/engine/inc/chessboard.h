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
#include <position/position.hpp>

struct Move;
struct PrioratizedMove;
struct PrioratizedMoveComparator;

struct MoveUndoUnit {
    MoveUndoUnit() :
        move(),
        movedPiece(),
        capturedPiece(),
        castlingState(),
        enPassantState(),
        hash(0),
        plyCount(0)
    {
    }
    PackedMove move;
    ChessPiece movedPiece;
    ChessPiece capturedPiece;
    CastlingStateInfo castlingState;
    EnPassantStateInfo enPassantState;
    u64 hash;
    short plyCount;
};

/**
 * The Chessboard class represents a chess board and its current state.
 * It provides functions for moving and placing chess pieces, and updates
 * all underlying state accordingly.
 *
 * @author Alexander Loodin Ek  */
class Chessboard {
public:
    Chessboard();
    ~Chessboard() = default;
    Chessboard(const Chessboard& other);

    void Clear();
    bool PlacePiece(ChessPiece piece, Notation target, bool overwrite = false);

    template<bool validation>
    MoveUndoUnit MakeMove(const PackedMove move);

    bool UnmakeMove(const MoveUndoUnit& undoState);

    template<typename... placementpairs>
    bool PlacePieces(placementpairs... placements);

    template<typename... moves>
    std::vector<MoveUndoUnit> MakeMoves(const moves&... _moves);

    /**
     * @brief Calculates the end game coeficient.
     * Used for tapered evaluation.
     * @return a float between 0 and 1 where 1 is endgame and 0 is midgame.     */
    float calculateEndGameCoeficient() const;

    /**
     * @brief Calculates a bitboard which shows opponents available moves, i.e.
     * threatened squares.
     * @param set The set to calculate the threat against.
     * Should be removed!*/
    u64 calculateThreatenedMask(Set set) const;

    template<typename T, bool isConst = false>
    class ChessboardIterator {
        friend class Chessboard;

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
        const ChessboardIterator& operator*() const { return *this; }

        ChessboardIterator& operator++();
        ChessboardIterator operator++(int);
        ChessboardIterator& operator+=(int incre);

        bool end() const;
        byte file() const { return m_position.file; }
        byte rank() const { return m_position.rank; }
        byte index() const { return m_index; }
        Square square() const { return m_position.toSquare(); }
        ChessPiece get() const { return m_chessboard.readPieceAt(m_position.toSquare()); }

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
    CastlingStateInfo readCastlingState() const { return m_position.readCastling(); }
    const Position& readPosition() const { return m_position; }
    Position& editPosition() { return m_position; }
    ChessPiece readPieceAt(Square sqr) const { return m_position.readPieceAt(sqr); }

    u64 readHash() const { return m_hash; }
    short readMoveCount() const { return m_moveCount; }
    short readPlyCount() const { return m_plyCount; }
    void setPlyAndMoveCount(short ply, short moveCount)
    {
        m_plyCount = ply;
        m_moveCount = moveCount;
    }
    Set readToPlay() const { return m_isWhiteTurn ? Set::WHITE : Set::BLACK; }
    void setToPlay(Set set);
    short readAge() const { return m_age; }

    std::string toString() const;

private:
    template<typename piece, typename square, typename... placements>
    bool InternalProcessEvenPlacementPairs(const piece& p, const square& sqr, const placements&... _placements);
    bool InternalProcessEvenPlacementPairs() { return true; }

    template<typename move, typename... moves>
    std::vector<MoveUndoUnit> InternalUnrollMoves(const move& m, const moves&... _moves);
    std::vector<MoveUndoUnit> InternalUnrollMoves() { return {}; }

    MoveUndoUnit InternalMakeMove(const std::string& moveString);

    std::tuple<Square, ChessPiece> InternalHandlePawnMove(const PackedMove move, Set set, MutableMaterialProxy& materialEditor, MoveUndoUnit& undoState);
    void InternalHandleRookMove(const ChessPiece piece, const PackedMove move, Square targetRook, Square rookMove, MoveUndoUnit& undoState);
    void InternalHandleRookMovedOrCaptured(Notation rookSquare, MoveUndoUnit& undoState);
    void InternalUpdateCastlingState(byte mask, MoveUndoUnit& undoState);

    /**
     * Internal helper function for handling the movement of a king chess piece.
     *
     * @param move The move being made.
     * @param set The set of the king being moved.
     * @param targetRook The position of the rook that will be involved in the castle move (if any).
     * @param rookMove The position that the rook will move to during the castle move (if any).
     * @return True if the move is a castle move, false otherwise. */
    bool InternalHandleKingMove(const PackedMove move, Set set, Square& targetRook, Square& rookMove, MoveUndoUnit& undoState);
    void InternalHandleKingRookMove(const ChessPiece piece, const PackedMove move, MoveUndoUnit& undoState);
    void InternalHandleCapture(const PackedMove move, const Square pieceTarget, MoveUndoUnit& undoState);

    bool InternalUpdateEnPassant(Notation source, Notation target);
    void InternalMakeMove(ChessPiece piece, Square source, Square target, MutableMaterialProxy materialEditor);

    u64 m_hash;
    Position m_position;

    bool m_isWhiteTurn;
    short m_moveCount;
    short m_plyCount;
    short m_age;
    mutable float m_endGameCoeficient;

    // caching kings and their locations
    std::pair<ChessPiece, Notation> m_kings[2];
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

template<typename... moves>
std::vector<MoveUndoUnit>
Chessboard::MakeMoves(const moves&... _moves)
{
    return InternalUnrollMoves(_moves...);
}

template<typename move, typename... moves>
std::vector<MoveUndoUnit>
Chessboard::InternalUnrollMoves(const move& m, const moves&... _moves)
{
    std::vector<MoveUndoUnit> result;
    result.push_back(InternalMakeMove(m));
    auto temp = InternalUnrollMoves(_moves...);
    result.insert(result.end(), temp.begin(), temp.end());
    return result;
}