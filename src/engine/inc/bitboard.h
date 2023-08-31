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
#include "intrinsics.hpp"

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

    /* Material Manpulators and readers */

    void Clear();

    bool PlacePiece(ChessPiece piece, Notation target);
    bool ClearPiece(ChessPiece piece, Notation target);
    bool IsValidMove(Notation source, ChessPiece piece, Notation target, byte castling, Notation enPassant,
                     u64 threatenedMask) const;

    template<Set s>
    u64 readCombinedMaterial() const;

    template<Set s>
    MaterialMask readMaterial() const;

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

    template<Set s>
    u64 calcAvailableMovesPawnsBulk() const;
    template<Set s>
    u64 calcAvailableMovesKnightBulk() const;
    template<Set s, u8 pieceId = rookId>
    u64 calcAvailableMovesRookBulk() const;
    template<Set s, u8 pieceId = bishopId>
    u64 calcAvailableMovesBishopBulk() const;
    template<Set s>
    u64 calcAvailableMovesQueenBulk() const;
    template<Set us, Set op = opposing_set<us>()>
    u64 calcAvailableMovesKingBulk() const;

    template<Set s>
    MaterialSlidingMask calcMaterialSlidingMasksBulk() const;

    template<Set s>
    u64 calcAvailableAttacksPawnsBulk() const;

    template<Set s>
    u64 calcThreatenedSquaresPawnsBulk() const;
    template<Set s>
    u64 calcThreatenedSquaresKnightBulk() const;
    template<Set s, u8 pieceId = bishopId>
    u64 calcThreatenedSquaresBishopBulk() const;
    template<Set s, u8 pieceId = rookId>
    u64 calcThreatenedSquaresRookBulk() const;
    template<Set s>
    u64 calcThreatenedSquaresQueensBulk() const;
    template<Set s>
    u64 calcThreatenedSquaresKingBulk() const;
    template<Set s, bool includeMaterial, bool pierceKing = false>
    u64 calcThreatenedSquares() const;

    template<Set s, bool includeMaterial, bool pierceKing = false>
    u64 calcThreatenedSquaresDiagonal() const;
    template<Set s, bool includeMaterial, bool pierceKing = false>
    u64 calcThreatenedSquaresOrthogonal() const;

    template<Set s, u8 pieceId, u8 direction>
    u64 calcPinnedPiecesBulk(KingMask kingMask) const;

    template<Set s>
    u64 isolatePiece(u8 pieceId, Notation source, u64 movesbb) const;

    template<Set s, u8 pieceId>
    u64 isolatePiece(Notation source, u64 movesbb) const;

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
    u64 GetMaterialCombined(Set set) const;
    u64 GetMaterial(ChessPiece piece) const;
    MaterialMask GetMaterial(Set set) const;

private:
    u64 calcThreatenedSquares(Notation source, ChessPiece piece, bool pierceKing = false) const;
    template<Set s, u8 direction, u8 pieceId>
    u64 internalCalcAvailableMoves(u64 bounds) const;

    /**
     * @brief Isolate a given pawn from the moves bitboard.
     * The following functions all do the same thing, but for different pieces. They take a bitboard representing all
     * available moves for a given piece type, and isolate the moves that are valid for the given piece at source square.
     * @param set The set of the pawn.
     * @param source The source square of the pawn.
     * @param movesbb The moves bitboard.    */
    u64 internalIsolatePawn(Set set, Notation source, u64 movesbb) const;
    u64 internalIsolateBishop(Set set, Notation source, u64 movesbb) const;
    u64 internalIsolateRook(Set set, Notation source, u64 movesbb) const;

    u64 MaterialCombined(byte set) const;
    u64 MaterialCombined() const;
    u64 SlidingMaterialCombined(byte set) const;
    u64 Castling(byte set, byte castling, u64 threatenedMask) const;
    u64 calcAvailableMovesForPawn(u64 mat, u64 opMat, Notation source, ChessPiece piece, Notation enPassant, u64 threatenedMask,
                                  KingMask checkedMask, KingMask kingMask) const;
    u64 calcAvailableMovesForKing(u64 mat, u64 threatenedMask, Notation source, ChessPiece piece, byte castling) const;
    mutable MaterialMask m_material[2];
};

template<Set s>
[[nodiscard]] constexpr u64
shiftNorthRelative(u64 bb)
{
    if constexpr (s == Set::WHITE) {
        return bb << shifts::vertical;
    }
    else {
        return bb >> shifts::vertical;
    }
}
template<Set s>
[[nodiscard]] constexpr u64
shiftEastRelative(u64 bb)
{
    if constexpr (s == Set::WHITE) {
        return bb << shifts::horizontal;
    }
    else {
        return bb >> shifts::horizontal;
    }
}

template<Set s>
[[nodiscard]] constexpr u64
shiftSouthRelative(u64 bb)
{
    if constexpr (s == Set::WHITE) {
        return bb >> shifts::vertical;
    }
    else {
        return bb << shifts::vertical;
    }
}

template<Set s>
[[nodiscard]] constexpr u64
shiftWestRelative(u64 bb)
{
    if constexpr (s == Set::WHITE) {
        return bb >> shifts::horizontal;
    }
    else {
        return bb << shifts::horizontal;
    }
}

template<Set s>
[[nodiscard]] constexpr u64
shiftNorthEastRelative(u64 bb)
{
    if constexpr (s == Set::WHITE) {
        return bb << shifts::forward_diagonal;
    }
    else {
        return bb >> shifts::forward_diagonal;
    }
}

template<Set s>
[[nodiscard]] constexpr u64
shiftSouthEastRelative(u64 bb)
{
    if constexpr (s == Set::WHITE) {
        return bb >> shifts::backward_diagonal;
    }
    else {
        return bb << shifts::backward_diagonal;
    }
}

template<Set s>
[[nodiscard]] constexpr u64
shiftSouthWestRelative(u64 bb)
{
    if constexpr (s == Set::WHITE) {
        return bb >> shifts::forward_diagonal;
    }
    else {
        return bb << shifts::forward_diagonal;
    }
}

template<Set s>
[[nodiscard]] constexpr u64
shiftNorthWestRelative(u64 bb)
{
    if constexpr (s == Set::WHITE) {
        return bb << shifts::backward_diagonal;
    }
    else {
        return bb >> shifts::backward_diagonal;
    }
}

template<Set s, u8 direction>
[[nodiscard]] constexpr u64
shiftRelative(u64 bb)
{
    if constexpr (direction == north) {
        return shiftNorthRelative<s>(bb);
    }
    else if constexpr (direction == east) {
        return shiftEastRelative<s>(bb);
    }
    else if constexpr (direction == south) {
        return shiftSouthRelative<s>(bb);
    }
    else if constexpr (direction == west) {
        return shiftWestRelative<s>(bb);
    }
    else if constexpr (direction == northeast) {
        return shiftNorthEastRelative<s>(bb);
    }
    else if constexpr (direction == southeast) {
        return shiftSouthEastRelative<s>(bb);
    }
    else if constexpr (direction == southwest) {
        return shiftSouthWestRelative<s>(bb);
    }
    else if constexpr (direction == northwest) {
        return shiftNorthWestRelative<s>(bb);
    }

    FATAL_ASSERT(false) << "Invalid direction";
}

[[nodiscard]] constexpr u64
inclusiveFillWest(i16 file)
{
    u64 result = 0;
    do {
        result |= board_constants::fileMasks[file];
        file--;
    } while (file >= 0);
    return result;
}

[[nodiscard]] constexpr u64
inclusiveFillEast(i16 file)
{
    u64 result = 0;
    do {
        result |= board_constants::fileMasks[file];
        file++;
    } while (file < 8);
    return result;
}

[[nodiscard]] constexpr u64
inclusiveFillSouth(i16 rank)
{
    u64 result = 0;
    do {
        result |= board_constants::rankMasks[rank];
        rank--;
    } while (rank >= 0);
    return result;
}

[[nodiscard]] constexpr u64
inclusiveFillNorth(i16 rank)
{
    u64 result = 0;
    do {
        result |= board_constants::rankMasks[rank];
        rank++;
    } while (rank < 8);
    return result;
}

[[nodiscard]] constexpr u64
inclusiveFillNorthEast(i16 file, i16 rank)
{
    u64 result = 0;
    i16 index = 7 + file - rank;
    do {
        result |= board_constants::backwardDiagonalMasks[index];
        index++;
    } while (index < 15);
    return result;
}

[[nodiscard]] constexpr u64
inclusiveFillSouthEast(i16 file, i16 rank)
{
    u64 result = 0;
    i16 index = 7 + file - rank;
    do {
        result |= board_constants::forwardDiagonalMasks[index];
        index++;
    } while (index < 15);
    return result;
}

[[nodiscard]] constexpr u64
inclusiveFillSouthWest(i16 file, i16 rank)
{
    u64 result = 0;
    i16 index = 7 + file - rank;
    do {
        result |= board_constants::backwardDiagonalMasks[index];
        index--;
    } while (index >= 0);
    return result;
}

[[nodiscard]] constexpr u64
inclusiveFillNorthWest(i16 file, i16 rank)
{
    u64 result = 0;
    i16 index = 7 + file - rank;
    do {
        result |= board_constants::forwardDiagonalMasks[index];
        index--;
    } while (index >= 0);
    return result;
}

template<Set s>
u64
Bitboard::readCombinedMaterial() const
{
    return readMaterial<s>().combine();
}

template<Set s>
MaterialMask
Bitboard::readMaterial() const
{
    if constexpr (s == Set::WHITE) {
        return m_material[0];
    }
    else {
        return m_material[1];
    }
}

template<Set s>
u64
Bitboard::calcAvailableMovesPawnsBulk() const
{
    u64 unoccupied = ~(m_material[0].combine() | m_material[1].combine());
    u64 piecebb = m_material[(size_t)s].material[pawnId];
    u64 mvsbb = ~universe;

    mvsbb = shiftNorthRelative<s>(piecebb);
    u64 doublePush = mvsbb & pawn_constants::baseRank[(size_t)s] & unoccupied;
    mvsbb |= shiftNorthRelative<s>(doublePush);

    return mvsbb & unoccupied;
}

template<Set s, u8 direction, u8 pieceId>
u64
Bitboard::internalCalcAvailableMoves(u64 bounds) const
{
    const u64 piecebb = m_material[(size_t)s].material[pieceId];
    const u64 materialbb = readCombinedMaterial<s>();
    const Set opSet = ChessPiece::FlipSet<s>();
    const u64 opMaterial = m_material[(size_t)opSet].combine();

    bounds |= opMaterial;

    u64 bbCopy = piecebb;
    u64 moves = 0;
    do {
        u64 purge = bbCopy & bounds;
        bbCopy &= ~purge;

        bbCopy = shiftRelative<s, direction>(bbCopy);
        moves |= bbCopy;
        bbCopy ^= (materialbb & bbCopy);

    } while (bbCopy > 0);

    return moves;
}

template<Set s>
u64
Bitboard::calcAvailableMovesKnightBulk() const
{
    u64 result = 0;
    const u64 knights = readMaterial<s>().material[knightId];
    if (knights == 0)
        return result;  // early out

    const u8 moveCount = ChessPieceDef::MoveCount(knightId);

    for (u8 moveIndx = 0; moveIndx < moveCount; ++moveIndx) {
        const u16 dir = ChessPieceDef::Attacks0x88(knightId, moveIndx);

        u64 knightsCopy = knights;
        while (knightsCopy > 0) {
            byte curSqr = intrinsics::lsbIndex(knightsCopy);
            knightsCopy = intrinsics::resetLsb(knightsCopy);
            // build a 0x88 square out of current square.
            signed char sq0x88 = to0x88(curSqr);
            // do move
            sq0x88 += dir;
            if (sq0x88 & 0x88)  // validate move, are we still on the board?
                break;

            // convert our sqr0x88 back to a square index
            curSqr = fr0x88(sq0x88);
            // build a square mask from current square
            u64 sqrMask = squareMaskTable[curSqr];
            result |= sqrMask;
        }
    }

    return result;
}

template<Set s, u8 pieceId>
u64
Bitboard::calcAvailableMovesBishopBulk() const
{
    const u64 materialbb = m_material[(size_t)s].combine();

    u64 moves = calcThreatenedSquaresBishopBulk<s, pieceId>();

    moves ^= (materialbb & moves);
    return moves;
}

template<Set s, u8 pieceId>
u64
Bitboard::calcAvailableMovesRookBulk() const
{
    const u64 materialbb = m_material[(size_t)s].combine();

    u64 moves = calcThreatenedSquaresRookBulk<s, pieceId>();

    moves ^= (materialbb & moves);
    return moves;
}

template<Set s>
u64
Bitboard::calcAvailableMovesQueenBulk() const
{
    u64 moves = 0;
    moves |= calcAvailableMovesBishopBulk<s, queenId>();
    moves |= calcAvailableMovesRookBulk<s, queenId>();
    return moves;
}

template<Set us, Set op>
u64
Bitboard::calcAvailableMovesKingBulk() const
{
    bool constexpr includeMaterial = false;
    u64 treatened = calcThreatenedSquares<op, includeMaterial>();
    u64 moves = calcThreatenedSquaresKingBulk<us>();
    moves &= ~treatened;
    return moves;
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
    u64 afilePawns = piecebb & board_constants::boundsRelativeMasks[(size_t)s][west];
    piecebb &= ~afilePawns;
    afilePawns = shiftNorthEastRelative<s>(afilePawns);

    u64 hfilePawns = piecebb & board_constants::boundsRelativeMasks[(size_t)s][east];
    piecebb &= ~hfilePawns;
    hfilePawns = shiftNorthWestRelative<s>(hfilePawns);

    u64 threatbb = afilePawns | hfilePawns;
    threatbb |= shiftNorthWestRelative<s>(piecebb);
    threatbb |= shiftNorthEastRelative<s>(piecebb);

    return threatbb;
}

template<Set s>
u64
Bitboard::calcThreatenedSquaresKnightBulk() const
{
    return calcAvailableMovesKnightBulk<s>();
}

template<Set s, u8 pieceId>
u64
Bitboard::calcThreatenedSquaresBishopBulk() const
{
    const auto bounds = board_constants::boundsRelativeMasks[(size_t)s];

    u64 moves = 0;
    moves |= internalCalcAvailableMoves<s, northeast, pieceId>(bounds[north] | bounds[east]);
    moves |= internalCalcAvailableMoves<s, southeast, pieceId>(bounds[south] | bounds[east]);
    moves |= internalCalcAvailableMoves<s, southwest, pieceId>(bounds[south] | bounds[west]);
    moves |= internalCalcAvailableMoves<s, northwest, pieceId>(bounds[north] | bounds[west]);

    return moves;
}

template<Set s, u8 pieceId>
u64
Bitboard::calcThreatenedSquaresRookBulk() const
{
    const auto bounds = board_constants::boundsRelativeMasks[(size_t)s];

    u64 moves = 0;
    moves |= internalCalcAvailableMoves<s, north, pieceId>(bounds[north]);
    moves |= internalCalcAvailableMoves<s, east, pieceId>(bounds[east]);
    moves |= internalCalcAvailableMoves<s, south, pieceId>(bounds[south]);
    moves |= internalCalcAvailableMoves<s, west, pieceId>(bounds[west]);

    return moves;
}

template<Set s>
u64
Bitboard::calcThreatenedSquaresQueensBulk() const
{
    u64 moves = 0;

    moves |= calcThreatenedSquaresRookBulk<s, queenId>();
    moves |= calcThreatenedSquaresBishopBulk<s, queenId>();

    return moves;
}

template<Set s>
u64
Bitboard::calcThreatenedSquaresKingBulk() const
{
    size_t setIndx = (size_t)s;
    u64 moves = 0;
    const u64 kingSqrMask = m_material[setIndx].material[kingId];
    if (kingSqrMask == 0)
        return moves;

    const i32 kingSqr = intrinsics::lsbIndex(kingSqrMask);

    i32 newKingSqr = kingSqr - king_constants::maskOffset;

    if (newKingSqr >= 0) {
        moves = (king_constants::moveMask << newKingSqr);
    }
    else if (newKingSqr < 0) {
        moves = (king_constants::moveMask >> -newKingSqr);
    }

    // clean up opposing side
    if (kingSqrMask & board_constants::fileaMask)
        moves ^= (moves & board_constants::filehMask);
    else if (kingSqrMask & board_constants::filehMask)
        moves ^= (moves & board_constants::fileaMask);

    moves &= ~m_material[setIndx].combine();

    return moves;
}

template<Set s, bool includeMaterial, bool pierceKing>
u64
Bitboard::calcThreatenedSquares() const
{
    u64 result = ~universe;

    [[maybe_unused]] u64 kingMask = 0;

    // removing king from opmaterial so it doesn't stop our sliding.
    // needs to be reset later on.
    if constexpr (pierceKing) {
        // can we build a scoped struct to make this a bit cleaner?
        Set opSet = ChessPiece::FlipSet<s>();
        kingMask = m_material[(size_t)opSet].material[kingId];
        m_material[(size_t)opSet].material[kingId] = 0;
    }

    result |= calcThreatenedSquaresPawnsBulk<s>();
    result |= calcAvailableMovesKnightBulk<s>();
    result |= calcThreatenedSquaresBishopBulk<s>();
    result |= calcThreatenedSquaresBishopBulk<s, queenId>();
    result |= calcThreatenedSquaresRookBulk<s>();
    result |= calcThreatenedSquaresRookBulk<s, queenId>();
    result |= calcThreatenedSquaresKingBulk<s>();

    if constexpr (pierceKing) {
        Set opSet = ChessPiece::FlipSet<s>();
        m_material[(size_t)opSet].material[kingId] = kingMask;
    }

    if constexpr (includeMaterial)
        result |= m_material[(size_t)s].combine();

    return result;
}

template<Set s, bool includeMaterial, bool pierceKing>
u64
Bitboard::calcThreatenedSquaresDiagonal() const
{
    u64 result = ~universe;
    [[maybe_unused]] u64 kingMask = 0;

    // removing king from opmaterial so it doesn't stop our sliding.
    if constexpr (pierceKing) {
        Set opSet = ChessPiece::FlipSet<s>();
        kingMask = m_material[(size_t)opSet].material[kingId];
        m_material[(size_t)opSet].material[kingId] = 0;
    }

    result |= calcThreatenedSquaresBishopBulk<s>();
    result |= calcThreatenedSquaresBishopBulk<s, queenId>();

    if constexpr (includeMaterial)
        result |= m_material[(size_t)s].material[queenId] | m_material[(size_t)s].material[bishopId];

    if constexpr (pierceKing) {
        Set opSet = ChessPiece::FlipSet<s>();
        m_material[(size_t)opSet].material[kingId] = kingMask;
    }

    return result;
}
template<Set s, bool includeMaterial, bool pierceKing>
u64
Bitboard::calcThreatenedSquaresOrthogonal() const
{
    u64 result = ~universe;
    [[maybe_unused]] u64 kingMask = 0;

    // removing king from opmaterial so it doesn't stop our sliding.
    if constexpr (pierceKing) {
        Set opSet = ChessPiece::FlipSet<s>();
        kingMask = m_material[(size_t)opSet].material[kingId];
        m_material[(size_t)opSet].material[kingId] = 0;
    }

    result |= calcThreatenedSquaresRookBulk<s>();
    result |= calcThreatenedSquaresRookBulk<s, queenId>();

    if constexpr (includeMaterial)
        result |= m_material[(size_t)s].material[queenId] | m_material[(size_t)s].material[rookId];

    if constexpr (pierceKing) {
        Set opSet = ChessPiece::FlipSet<s>();
        m_material[(size_t)opSet].material[kingId] = kingMask;
    }

    return result;
}

template<Set s, u8 pieceId, u8 direction>
u64
Bitboard::calcPinnedPiecesBulk(KingMask kingMask) const
{
    return kingMask.threats[direction] & m_material[(size_t)s].material[pieceId];
}

template<Set s>
MaterialSlidingMask
Bitboard::calcMaterialSlidingMasksBulk() const
{
    u64 orthogonal = ~universe;
    u64 diagonal = ~universe;

    diagonal |= calcAvailableMovesBishopBulk<s>();
    diagonal |= calcAvailableMovesBishopBulk<s, queenId>();

    orthogonal |= calcAvailableMovesRookBulk<s>();
    orthogonal |= calcAvailableMovesRookBulk<s, queenId>();

    // add material
    diagonal |= m_material[(size_t)s].material[bishopId] | m_material[(size_t)s].material[queenId];
    orthogonal |= m_material[(size_t)s].material[rookId] | m_material[(size_t)s].material[queenId];

    return {orthogonal, diagonal};
}

template<Set us, u8 pieceId>
u64
Bitboard::isolatePiece(Notation source, u64 movesbb) const
{
    return isolatePiece<us>(pieceId, source, movesbb);
}

template<Set us>
u64
Bitboard::isolatePiece(u8 pieceId, Notation source, u64 movesbb) const
{
    switch (pieceId) {
        case pawnId:
            return internalIsolatePawn(us, source, movesbb);
        case bishopId:
            return internalIsolateBishop(us, source, movesbb);
        case rookId:
            return internalIsolateRook(us, source, movesbb);
        default:
            FATAL_ASSERT(false) << "Not implemented";
    }

    return 0;
}