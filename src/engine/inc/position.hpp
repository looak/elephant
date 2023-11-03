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
 * @file position.hpp
 * @brief Backend of this chess engine. Represents the chessboard as a few 64bit
 * integers where each bit represents a square on the board. By using this
 * representation we can optimize the move generation and evaluation of the
 * board. The board is built up by 12 bitboards, one for each piece type.
 *
 * @author Alexander Loodin Ek    */
#pragma once
#include <functional>
#include "bitboard.hpp"
#include "bitboard_constants.h"
#include "chess_piece.h"
#include "defines.h"
#include "intrinsics.hpp"

struct Notation;

struct MaterialMask {
    Bitboard material[6]{};

    inline constexpr Bitboard combine() const
    {
        return material[pawnId] | material[knightId] | material[bishopId] | material[rookId] | material[queenId] |
               material[kingId];
    }

    [[nodiscard]] constexpr Bitboard operator[](i32 indx) { return material[indx]; }
    [[nodiscard]] constexpr Bitboard operator[](i32 indx) const { return material[indx]; }
};

// this would be 8 x 64bits, i.e. 64 bytes per position rather than the currently used
// 12 x 64bits, i.e. 96 bytes per position.
// struct MaterialPositionMask {
//     Bitboard set[2];
//     Bitboard material[6];

//     template<Set us>
//     [[nodiscard]] constexpr Bitboard operator[](i32 pieceId)
//     {
//         if constexpr (us == Set::WHITE) {
//             return material[pieceId] & set[0];
//         }
//         else {
//             return material[pieceId] & set[1];
//         }
//     }
//     template<Set us>
//     [[nodiscard]] constexpr Bitboard operator[](i32 pieceId) const
//     {
//         if constexpr (us == Set::WHITE) {
//             return material[pieceId] & set[0];
//         }
//         else {
//             return material[pieceId] & set[1];
//         }
//     }

//     [[nodiscard]] constexpr Bitboard operator[](i32 indx) { return material[indx]; }
//     [[nodiscard]] constexpr Bitboard operator[](i32 indx) const { return material[indx]; }

//     template<Set us>
//     [[nodiscard]] constexpr Bitboard combine() const
//     {
//         if constexpr (us == Set::WHITE) {
//             return set[0];
//         }
//         return set[1];
//     }
// };

struct MaterialSlidingMask {
    Bitboard orthogonal;
    Bitboard diagonal;
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

class Position {
public:
    static bool IsValidSquare(signed short currSqr);
    static bool IsValidSquare(Notation source);

public:
    Position();
    Position& operator=(const Position& other);

    /* Material Manpulators and readers */

    void Clear();

    bool PlacePiece(ChessPiece piece, Notation target);
    bool ClearPiece(ChessPiece piece, Notation target);
    bool IsValidMove(Notation source, ChessPiece piece, Notation target, byte castling, Notation enPassant,
                     u64 threatenedMask) const;

    template<Set us>
    MaterialMask readMaterial() const;

    template<Set us>
    MaterialMask& writeMaterial();

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

    template<Set us>
    Bitboard calcAvailableMovesPawnBulk() const;
    template<Set us>
    Bitboard calcAvailableMovesKnightBulk() const;
    template<Set us, u8 pieceId = rookId>
    Bitboard calcAvailableMovesRookBulk() const;
    template<Set us, u8 pieceId = bishopId>
    Bitboard calcAvailableMovesBishopBulk() const;
    template<Set us>
    Bitboard calcAvailableMovesQueenBulk() const;
    template<Set us, Set op = opposing_set<us>()>
    Bitboard calcAvailableMovesKing(byte castlingRights) const;

    template<Set us>
    MaterialSlidingMask calcMaterialSlidingMasksBulk() const;

    template<Set us>
    Bitboard calcAvailableAttacksPawnBulk() const;

    template<Set us>
    Bitboard calcThreatenedSquaresPawnBulk() const;
    template<Set us>
    Bitboard calcThreatenedSquaresKnightBulk() const;
    template<Set us, u8 pieceId = bishopId>
    Bitboard calcThreatenedSquaresBishopBulk() const;
    template<Set us, u8 pieceId = rookId>
    Bitboard calcThreatenedSquaresRookBulk() const;
    template<Set us>
    Bitboard calcThreatenedSquaresQueenBulk() const;
    template<Set us>
    Bitboard calcThreatenedSquaresKing() const;

    template<Set us, bool includeMaterial, bool pierceKing = false>
    Bitboard calcThreatenedSquares() const;
    template<Set us, bool includeMaterial, bool pierceKing = false>
    Bitboard calcThreatenedSquaresDiagonal() const;
    template<Set us, bool includeMaterial, bool pierceKing = false>
    Bitboard calcThreatenedSquaresOrthogonal() const;

    template<Set us, u8 pieceId, u8 direction>
    Bitboard calcPinnedPiecesBulk(KingMask kingMask) const;

    template<Set us>
    std::tuple<Bitboard, Bitboard> isolatePiece(u8 pieceId, Notation source, Bitboard movesbb) const;

    template<Set us, u8 pieceId>
    std::tuple<Bitboard, Bitboard> isolatePiece(Notation source, Bitboard movesbb) const;

    i32 diffWestEast(Notation a, Notation b) const;

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
    Bitboard GetMaterialCombined(Set uset) const;
    Bitboard GetMaterial(ChessPiece piece) const;
    MaterialMask GetMaterial(Set set) const;

private:
    u64 calcThreatenedSquares(Notation source, ChessPiece piece, bool pierceKing = false) const;
    template<Set us, u8 direction, u8 pieceId>
    Bitboard internalCalcAvailableMoves(Bitboard bounds) const;

    /**
     * @brief Isolate a given pawn from the moves bitboard.
     * The following functions all do the same thing, but for different pieces. They take a bitboard representing all
     * available moves for a given piece type, and isolate the moves that are valid for the given piece at source square.
     * @param source The source square of the pawn.
     * @param movesbb The moves bitboard.    */
    template<Set us>
    std::tuple<Bitboard, Bitboard> internalIsolatePawn(Notation source, Bitboard movesbb) const;
    template<Set us>
    std::tuple<Bitboard, Bitboard> internalIsolateKnightMoves(Notation source, Bitboard movesbb) const;
    std::tuple<Bitboard, Bitboard> internalIsolateBishop(Set uset, Notation source, Bitboard movesbb) const;
    std::tuple<Bitboard, Bitboard> internalIsolateRook(Set uset, Notation source, Bitboard movesbb) const;

    Bitboard MaterialCombined(byte set) const;
    Bitboard MaterialCombined() const;
    Bitboard SlidingMaterialCombined(byte set) const;
    u64 Castling(byte set, byte castling, u64 threatenedMask) const;
    u64 calcAvailableMovesForPawn(u64 mat, u64 opMat, Notation source, ChessPiece piece, Notation enPassant, u64 threatenedMask,
                                  KingMask checkedMask, KingMask kingMask) const;
    u64 calcAvailableMovesForKing(u64 mat, u64 threatenedMask, Notation source, ChessPiece piece, byte castling) const;
    mutable MaterialMask m_material[2];
};

template<Set us>
MaterialMask
Position::readMaterial() const
{
    if constexpr (us == Set::WHITE) {
        return m_material[0];
    }
    else {
        return m_material[1];
    }
}

template<Set us>
MaterialMask&
Position::writeMaterial()
{
    if constexpr (us == Set::WHITE) {
        return m_material[0];
    }
    else {
        return m_material[1];
    }
}

template<Set us, u8 direction, u8 pieceId>
Bitboard
Position::internalCalcAvailableMoves(Bitboard bounds) const
{
    const Bitboard piecebb = readMaterial<us>()[pieceId];
    const Bitboard materialbb = readMaterial<us>().combine();
    const Bitboard opMaterial = readMaterial<opposing_set<us>()>().combine();

    bounds |= opMaterial;

    Bitboard bbCopy = piecebb;
    Bitboard moves = 0;
    do {
        Bitboard purge = bbCopy & bounds;
        bbCopy &= ~purge;

        // // bbCopy = shiftRelative<s, direction>(bbCopy);
        moves |= bbCopy;
        bbCopy ^= (materialbb & bbCopy);

    } while (bbCopy.empty() == false);

    return moves;
}

template<Set us>
Bitboard
Position::calcAvailableMovesKnightBulk() const
{
    Bitboard result = 0;
    const Bitboard knights = readMaterial<us>()[knightId];
    if (knights == 0)
        return result;  // early out

    const u8 moveCount = ChessPieceDef::MoveCount(knightId);

    for (u8 moveIndx = 0; moveIndx < moveCount; ++moveIndx) {
        const u16 dir = ChessPieceDef::Attacks0x88(knightId, moveIndx);

        Bitboard knightsCopy = knights;
        while (knightsCopy.empty() == false) {
            byte curSqr = knightsCopy.popLsb();
            // build a 0x88 square out of current square.
            signed char sq0x88 = to0x88(curSqr);
            // do move
            sq0x88 += dir;
            if (sq0x88 & 0x88)  // validate move, are we still on the board?
                continue;

            // convert our sqr0x88 back to a square index
            curSqr = fr0x88(sq0x88);
            // build a square mask from current square
            u64 sqrMask = squareMaskTable[curSqr];
            result |= sqrMask;
        }
    }

    return result;
}

template<Set us, u8 pieceId>
Bitboard
Position::calcAvailableMovesBishopBulk() const
{
    const Bitboard materialbb = readMaterial<us>().combine();

    Bitboard moves = calcThreatenedSquaresBishopBulk<us, pieceId>();

    moves ^= (materialbb & moves);
    return moves;
}

template<Set us, u8 pieceId>
Bitboard
Position::calcAvailableMovesRookBulk() const
{
    const Bitboard materialbb = readMaterial<us>().combine();

    Bitboard moves = calcThreatenedSquaresRookBulk<us, pieceId>();

    moves ^= (materialbb & moves);
    return moves;
}

template<Set us>
Bitboard
Position::calcAvailableMovesQueenBulk() const
{
    Bitboard moves = 0;
    moves |= calcAvailableMovesBishopBulk<us, queenId>();
    moves |= calcAvailableMovesRookBulk<us, queenId>();
    return moves;
}

template<Set us>
Bitboard
Position::calcAvailableAttacksPawnBulk() const
{
    constexpr Set op = opposing_set<us>();

    u64 threats = calcThreatenedSquaresPawnBulk<s>();
    u64 opMaterial = readMaterial<op>().combine();

    return threats & opMaterial;
}

template<Set us>
Bitboard
Position::calcThreatenedSquaresKnightBulk() const
{
    return calcAvailableMovesKnightBulk<s>();
}

template<Set us, u8 pieceId>
Bitboard
Position::calcThreatenedSquaresBishopBulk() const
{
    const auto bounds = board_constants::boundsRelativeMasks[(size_t)us];

    Bitboard moves = 0;
    moves |= internalCalcAvailableMoves<us, northeast, pieceId>(bounds[north] | bounds[east]);
    moves |= internalCalcAvailableMoves<us, southeast, pieceId>(bounds[south] | bounds[east]);
    moves |= internalCalcAvailableMoves<us, southwest, pieceId>(bounds[south] | bounds[west]);
    moves |= internalCalcAvailableMoves<us, northwest, pieceId>(bounds[north] | bounds[west]);

    return moves;
}

template<Set us, u8 pieceId>
Bitboard
Position::calcThreatenedSquaresRookBulk() const
{
    const auto bounds = board_constants::boundsRelativeMasks[(size_t)us];

    Bitboard moves = 0;
    moves |= internalCalcAvailableMoves<us, north, pieceId>(bounds[north]);
    moves |= internalCalcAvailableMoves<us, east, pieceId>(bounds[east]);
    moves |= internalCalcAvailableMoves<us, south, pieceId>(bounds[south]);
    moves |= internalCalcAvailableMoves<us, west, pieceId>(bounds[west]);

    return moves;
}

template<Set us>
Bitboard
Position::calcThreatenedSquaresQueenBulk() const
{
    Bitboard moves = 0;

    moves |= calcThreatenedSquaresRookBulk<s, queenId>();
    moves |= calcThreatenedSquaresBishopBulk<s, queenId>();

    return moves;
}

template<Set us, bool includeMaterial, bool pierceKing>
Bitboard
Position::calcThreatenedSquaresDiagonal() const
{
    u64 result = ~universe;
    [[maybe_unused]] u64 kingMask = 0;
    constexpr Set opSet = opposing_set<us>();

    // removing king from opmaterial so it doesn't stop our sliding.
    if constexpr (pierceKing) {
        kingMask = readMaterial<opSet>()[kingId];
        writeMaterial<opSet>()[kingId] = 0;
    }

    result |= calcThreatenedSquaresBishopBulk<us>();
    result |= calcThreatenedSquaresBishopBulk<us, queenId>();

    if constexpr (includeMaterial) {
        const size_t usIndx = static_cast<size_t>(us);
        result |= readMaterial<us>()[queenId] | readMaterial<us>()[bishopId];
    }

    if constexpr (pierceKing)
        writeMaterial<opSet>()[kingId] = kingMask;

    return result;
}
template<Set us, bool includeMaterial, bool pierceKing>
Bitboard
Position::calcThreatenedSquaresOrthogonal() const
{
    u64 result = ~universe;
    [[maybe_unused]] u64 kingMask = 0;

    // removing king from opmaterial so it doesn't stop our sliding.
    if constexpr (pierceKing) {
        constexpr Set op = opposing_set<us>();
        kingMask = readMaterial<op>()[kingId];
        readMaterial<op>()[kingId] = 0;
    }

    result |= calcThreatenedSquaresRookBulk<s>();
    result |= calcThreatenedSquaresRookBulk<s, queenId>();

    if constexpr (includeMaterial)
        result |= readMaterial<us>()[queenId] | readMaterial<us>()[rookId];

    if constexpr (pierceKing) {
        constexpr Set op = opposing_set<us>();
        readMaterial<op>[kingId] = kingMask;
    }

    return result;
}

template<Set us, bool includeMaterial, bool pierceKing>
Bitboard
Position::calcThreatenedSquares() const
{
    Bitboard result = ~universe;
    constexpr Set op = opposing_set<us>();
    [[maybe_unused]] Bitboard kingMask = 0;

    // removing king from opmaterial so it doesn't stop our sliding.
    // needs to be reset later on.
    if constexpr (pierceKing) {
        // can we build a scoped struct to make this a bit cleaner?
        kingMask = readMaterial<op>()[kingId];
        readMaterial<op>()[kingId] = 0;
    }

    result |= calcThreatenedSquaresPawnBulk<us>();
    result |= calcAvailableMovesKnightBulk<us>();
    result |= calcThreatenedSquaresBishopBulk<us>();
    result |= calcThreatenedSquaresBishopBulk<us, queenId>();
    result |= calcThreatenedSquaresRookBulk<us>();
    result |= calcThreatenedSquaresRookBulk<us, queenId>();
    result |= calcThreatenedSquaresKing<us>();

    if constexpr (pierceKing)
        readMaterial<op>()[kingId] = kingMask;

    if constexpr (includeMaterial)
        result |= readMaterial<us>().combine();

    return result;
}

template<Set us, u8 pieceId, u8 direction>
Bitboard
Position::calcPinnedPiecesBulk(KingMask kingMask) const
{
    return kingMask.threats[direction] & readMaterial<us>()[pieceId];
}

template<Set us>
MaterialSlidingMask
Position::calcMaterialSlidingMasksBulk() const
{
    Bitboard orthogonal;
    Bitboard diagonal;

    diagonal |= calcAvailableMovesBishopBulk<us>();
    diagonal |= calcAvailableMovesBishopBulk<us, queenId>();

    orthogonal |= calcAvailableMovesRookBulk<us>();
    orthogonal |= calcAvailableMovesRookBulk<us, queenId>();

    // add material
    diagonal |= readMaterial<us>()[bishopId] | readMaterial<us>()[queenId];
    orthogonal |= readMaterial<us>()[rookId] | readMaterial<us>()[queenId];

    return {orthogonal, diagonal};
}

template<Set us, u8 pieceId>
std::tuple<Bitboard, Bitboard>
Position::isolatePiece(Notation source, Bitboard movesbb) const
{
    return isolatePiece<us>(pieceId, source, movesbb);
}