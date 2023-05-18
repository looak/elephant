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

/**
 * @file bitboard.h
 * @brief Backend of this chess engine. Represents the chessboard as a 64bit
 * integer where each bit represents a square on the board. By using this
 * representation we can optimize the move generation and evaluation of the
 * board. The board is built up by 12 bitboards, one for each piece type.
 *
 * @author Alexander Loodin Ek    */
#pragma once
#include <functional>
#include "bitboard_constants.h"
#include "chess_piece.h"
#include "defines.h"

struct Notation;

struct MaterialMask {
    u64 material[6]{};

    inline constexpr u64 combine() const
    {
        return material[pawnId] | material[knightId] | material[bishopId] | material[rookId] | material[queenId] |
               material[kingId];
    }
};

struct MaterialSlidingMask {
    u64 orthogonal;
    u64 diagonal;
};

/**
 * @brief KingMask, used to figure out pinned pieces and if king is in check. Each bitboard is a
 * direction of where the threat comes from. */
struct KingMask {
    KingMask()
    {
        for (int i = 0; i < 8; ++i) {
            threats[i] = 0;
            checked[i] = false;
        }
        knightsAndPawns = 0;
        knightOrPawnCheck = false;
        pawnMask = false;
    }

    u64 threats[8];
    bool checked[8];
    u64 knightsAndPawns;
    bool knightOrPawnCheck;
    bool pawnMask;

    KingMask checkedMask(int& checkedCount) const
    {
        KingMask result;
        for (int i = 0; i < 8; ++i) {
            if (checked[i]) {
                result.threats[i] = threats[i];
                result.checked[i] = true;
                ++checkedCount;
            }
        }

        if (knightOrPawnCheck) {
            result.knightsAndPawns = knightsAndPawns;
            result.knightOrPawnCheck = true;
            ++checkedCount;
        }
        return result;
    }
    bool zero() const
    {
        for (int i = 0; i < 8; ++i) {
            if (threats[i] != 0) {
                return false;
            }
        }
        return knightsAndPawns == 0;
    }

    u64 combined() const
    {
        u64 result = 0;
        result |= combinedPins();
        result |= knightsAndPawns;
        return result;
    }

    constexpr u64 combinedPins() const
    {
        u64 result = 0;
        for (int i = 0; i < 8; ++i) {
            result |= threats[i];
        }
        return result;
    }

    void operator^=(const KingMask& rhs)
    {
        for (int i = 0; i < 8; ++i) {
            threats[i] ^= rhs.threats[i];
        }
        knightsAndPawns ^= rhs.knightsAndPawns;
    }
};

inline KingMask
operator&(const KingMask& lhs, const u64& rhs)
{
    KingMask result;
    for (int i = 0; i < 8; ++i) {
        result.threats[i] = lhs.threats[i] & rhs;
    }
    result.knightsAndPawns = lhs.knightsAndPawns & rhs;
    return result;
}

inline KingMask
operator&(const KingMask& lhs, const KingMask& rhs)
{
    KingMask result;
    for (int i = 0; i < 8; ++i) {
        result.threats[i] = lhs.threats[i] & rhs.threats[i];
    }
    result.knightsAndPawns = lhs.knightsAndPawns & rhs.knightsAndPawns;
    return result;
}

inline KingMask
operator^(const KingMask& lhs, const KingMask& rhs)
{
    KingMask result;
    for (int i = 0; i < 8; ++i) {
        result.threats[i] = lhs.threats[i] ^ rhs.threats[i];
    }
    result.knightsAndPawns = lhs.knightsAndPawns ^ rhs.knightsAndPawns;
    return result;
}

class Bitboard {
public:
    static bool IsValidSquare(signed short currSqr);
    static bool IsValidSquare(Notation source);

public:
    Bitboard();
    Bitboard& operator=(const Bitboard& other);

    void Clear();

    bool PlacePiece(ChessPiece piece, Notation target);
    bool ClearPiece(ChessPiece piece, Notation target);
    bool IsValidMove(Notation source, ChessPiece piece, Notation target, byte castling, Notation enPassant,
                     u64 threatenedMask) const;

    /**
     * @brief Calculate the available moves for a given chess piece on the bitboard.
     * This function calculates the legal moves for the specified chess piece on the bitboard,
     * taking into account the current game state, including castling rights, en passant capture,
     * and pinned or checked status of the piece.
     * @param source The source square of the chess piece in algebraic notation.
     * @param piece The chess piece for which to calculate the available moves.
     * @param castling Castling rights represented as a byte.
     * @param enPassant The en passant target square represented as a byte.
     * @param threatened A bitmask representing the threatened squares on the board.
     * @param checked Boolean flag indicating if the piece is checked.
     * @param kingMask A bitmask representing the king's potential threats and pins.
     * @return A bitmask representing the available moves for the given chess piece.  */
    u64 calcAvailableMoves(Notation source, ChessPiece piece, byte castling, Notation enPassant, u64 threatenedMask = 0,
                           KingMask checkedMask = KingMask(), KingMask kingMask = KingMask()) const;
    u64 calcAttackedSquares(Notation source, ChessPiece piece) const;

    u64 calcThreatenedSquares(Notation source, ChessPiece piece, bool pierceKing = false) const;
    u64 GetThreatenedSquaresWithMaterial(Notation source, ChessPiece piece, bool pierceKing = false) const;

    template<Set s>
    u64 calcAvailableMovesPawnsBulk() const;

    template<Set s>
    u64 calcAvailableAttacksPawnsBulk() const;

    template<Set s>
    u64 calcThreatenedSquaresPawnsBulk() const;

    template<Set s>
    u64 calcPinnedPiecesBulk(KingMask kingMask) const;

    /**
     * @brief Calculate the king's potential threats and pins.
     * This function calculates the king's potential threats and pins, taking into account the
     * current game state. From Kings position, we look at all directions until we hit end of board
     * and see if we run into a sliding piece which is threatening the king and taking into account
     * same set pieces in between to figure out if they are pinned.
     * @param king ChessPiece representing the king we are using as source.
     * @param source Position on board of the king.
     * @param opponentSlidingMask A mask struct that contains opponents sliding masks.
     * @return A mask struct containing a seperate mask for each direction.  */
    KingMask calcKingMask(ChessPiece king, Notation source, const MaterialSlidingMask& opponentSlidingMask) const;
    u64 GetMaterialCombined(Set set) const;
    u64 GetMaterial(ChessPiece piece) const;
    MaterialMask GetMaterial(Set set) const;

private:
    u64 MaterialCombined(byte set) const;
    u64 MaterialCombined() const;
    u64 SlidingMaterialCombined(byte set) const;
    u64 Castling(byte set, byte castling, u64 threatenedMask) const;
    u64 calcAvailableMovesForPawn(u64 mat, u64 opMat, Notation source, ChessPiece piece, Notation enPassant, u64 threatenedMask,
                                  KingMask checkedMask, KingMask kingMask) const;
    u64 calcAvailableMovesForKing(u64 mat, u64 threatenedMask, Notation source, ChessPiece piece, byte castling) const;
    MaterialMask m_material[2];
};

template<Set s>
[[nodiscard]] u64
shiftForwardRelative(u64 bb)
{
    if constexpr (s == Set::WHITE) {
        return bb << shifts::vertical;
    }
    else {
        return bb >> shifts::vertical;
    }
}

template<Set s>
[[nodiscard]] u64
shiftForwardLeftRelative(u64 bb)
{
    if constexpr (s == Set::WHITE) {
        return bb << shifts::backward_diagonal;
    }
    else {
        return bb >> shifts::forward_diagonal;
    }
}

template<Set s>
[[nodiscard]] u64
shiftForwardRightRelative(u64 bb)
{
    if constexpr (s == Set::WHITE) {
        return bb << shifts::forward_diagonal;
    }
    else {
        return bb >> shifts::backward_diagonal;
    }
}

template<Set s>
u64
Bitboard::calcAvailableMovesPawnsBulk() const
{
    u64 unoccupied = ~(m_material[0].combine() | m_material[1].combine());
    u64 piecebb = m_material[(size_t)s].material[pawnId];
    u64 mvsbb = ~universe;

    mvsbb = shiftForwardRelative<s>(piecebb);
    u64 doublePush = mvsbb & pawn_constants::baseRank[(size_t)s] & unoccupied;
    mvsbb |= shiftForwardRelative<s>(doublePush);

    return mvsbb & unoccupied;
}

template<Set s>
u64
Bitboard::calcAvailableAttacksPawnsBulk() const
{
    Set opSet = ChessPiece::FlipSet<s>();

    u64 threats = calcThreatenedSquaresPawnsBulk<s>();
    u64 opMaterial = m_material[(size_t)opSet].combine();

    return threats & opMaterial;
}

template<Set s>
u64
Bitboard::calcThreatenedSquaresPawnsBulk() const
{
    u64 piecebb = m_material[(size_t)s].material[pawnId];

    // special case for a file & h file
    u64 afilePawns = piecebb & board_constants::fileaMask;
    piecebb &= ~afilePawns;
    afilePawns = shiftForwardRightRelative<s>(afilePawns);

    u64 hfilePawns = piecebb & board_constants::filehMask;
    piecebb &= ~hfilePawns;
    hfilePawns = shiftForwardLeftRelative<s>(hfilePawns);

    u64 threatbb = afilePawns | hfilePawns;
    threatbb |= shiftForwardLeftRelative<s>(piecebb);
    threatbb |= shiftForwardRightRelative<s>(piecebb);

    return threatbb;
}

template<Set s>
u64
Bitboard::calcPinnedPiecesBulk(KingMask kingMask) const
{
    return kingMask
}