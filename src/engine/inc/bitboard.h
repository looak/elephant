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
 * @author Alexander Loodin Ek
 */
#pragma once
#include <functional>
#include "defines.h"
#include "chess_piece.h"

struct Notation;

struct MaterialMask
{
    u64 orthogonal;
    u64 diagonal;
};

struct KingMask
{
    KingMask()
    {
        for (int i = 0; i < 8; ++i)
        {
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
        for (int i = 0; i < 8; ++i)
        {
            if (checked[i])
            {
                result.threats[i] = threats[i];
                result.checked[i] = true;
                ++checkedCount;
            }
        }

        if (knightOrPawnCheck)
        {
            result.knightsAndPawns = knightsAndPawns;
            result.knightOrPawnCheck = true;
            ++checkedCount;
        }
        return result;

    }
    bool zero() const
    {
        for (int i = 0; i < 8; ++i)
        {
            if (threats[i] != 0)
            {
                return false;
            }
        }
        return knightsAndPawns == 0;
    }

    u64 combined() const
    {
        u64 result = 0;
        for (int i = 0; i < 8; ++i)
        {
            result |= threats[i];
        }
        result |= knightsAndPawns;
        return result;
    }

    void operator^=(const KingMask& rhs)
    {
        for (int i = 0; i < 8; ++i)
        {
            threats[i] ^= rhs.threats[i];
        }
        knightsAndPawns ^= rhs.knightsAndPawns;
    }

    //u8 checkedCount() const { return std::count_if(checked, checked + 8, [](bool b) { return b; }); }
};

inline KingMask operator&(const KingMask& lhs, const u64& rhs)
{
    KingMask result;
    for (int i = 0; i < 8; ++i)
    {
        result.threats[i] = lhs.threats[i] & rhs;
    }
    result.knightsAndPawns = lhs.knightsAndPawns & rhs;
    return result;
}

inline KingMask operator&(const KingMask& lhs, const KingMask& rhs)
{
    KingMask result;
    for (int i = 0; i < 8; ++i)
    {
        result.threats[i] = lhs.threats[i] & rhs.threats[i];
    }
    result.knightsAndPawns = lhs.knightsAndPawns & rhs.knightsAndPawns;
    return result;
}

inline KingMask operator^(const KingMask& lhs, const KingMask& rhs)
{
    KingMask result;
    for (int i = 0; i < 8; ++i)
    {
        result.threats[i] = lhs.threats[i] ^ rhs.threats[i];
    }
    result.knightsAndPawns = lhs.knightsAndPawns ^ rhs.knightsAndPawns;
    return result;
}

class Bitboard
{
public:
    static bool IsValidSquare(signed short currSqr);
    static bool IsValidSquare(Notation source);
public:
    Bitboard();
    Bitboard& operator=(const Bitboard& other);

    void Clear();

    bool PlacePiece(ChessPiece piece, Notation target);
    bool ClearPiece(ChessPiece piece, Notation target);
    bool IsValidMove(Notation source, ChessPiece piece, Notation target, byte castling, byte enPassant, u64 threatenedMask) const;

    /**
     * @brief Calculate the available moves for a given chess piece on the bitboard.
     * This function calculates the legal moves for the specified chess piece on the bitboard, taking into account the current game state,
     * including castling rights, en passant capture, and pinned or checked status of the piece.    
     * @param source The source square of the chess piece in algebraic notation.
     * @param piece The chess piece for which to calculate the available moves.
     * @param castling Castling rights represented as a byte.
     * @param enPassant The en passant target square represented as a byte.
     * @param threatened A bitmask representing the threatened squares on the board.
     * @param checked Boolean flag indicating if the piece is checked.
     * @param kingMask A bitmask representing the king's potential threats and pins.
     * @return A bitmask representing the available moves for the given chess piece.  */
    u64 calcAvailableMoves(Notation source, ChessPiece piece, byte castling = 0x0, byte enPassant = 0xff, u64 threatenedMask = 0, KingMask checkedMask = KingMask(), KingMask kingMask = KingMask()) const;
    u64 calcAttackedSquares(Notation source, ChessPiece piece) const;

    u64 calcThreatenedSquares(Notation source, ChessPiece piece, bool pierceKing = false) const;
    u64 GetThreatenedSquaresWithMaterial(Notation source, ChessPiece piece, bool pierceKing = false) const;

    /**
     * @brief Calculate the king's potential threats and pins.
     * This function calculates the king's potential threats and pins, taking into account the current game state.
     * From Kings position, we look at all directions until we hit end of board and see if we run into a sliding piece which is threatening the king
     * and taking into account same set pieces in between to figure out if they are pinned.
     * @param king ChessPiece representing the king we are using as source.
     * @param source Position on board of the king.
     * @param opponentSlidingMask A mask struct that contains opponents sliding masks.
     * @return A mask struct containing a seperate mask for each direction.  */
    KingMask calcKingMask(ChessPiece king, Notation source, const MaterialMask& opponentSlidingMask) const;
    u64 GetMaterialCombined(Set set) const;
    u64 GetMaterial(ChessPiece piece) const;

    int BitScanFowrward(u64 bitboard) const;

private:
    typedef std::function<bool(u64 sqrMask)> ResolveMask;
    typedef std::function<u64(u64 sqrMask)> Validate;

    u64 MaterialCombined(byte set) const;
    u64 MaterialCombined() const;
    u64 SlidingMaterialCombined(byte set) const;
    u64 Castling(byte set, byte castling, u64 threatenedMask) const;
    u64 calcAvailableMovesForPawn(u64 mat, u64 opMat, Notation source, ChessPiece piece, byte enPassant, u64 threatenedMask, KingMask checkedMask, KingMask kingMask) const;
    u64 calcAvailableMovesForKing(u64 mat, u64 threatenedMask, Notation source, ChessPiece piece, byte castling) const;

    u64 internalGenerateMask(byte curSqr, signed short dir, bool& sliding, ResolveMask func, Validate valid = [](u64 sqrMask) { return sqrMask; }) const;

    u64 m_material[2][6];
};