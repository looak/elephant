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
#include "king_pin_threats.hpp"
#include "notation.h"

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

    [[nodiscard]] constexpr const Bitboard& kings() const { return material[kingId]; }
    [[nodiscard]] constexpr const Bitboard& queens() const { return material[queenId]; }
    [[nodiscard]] constexpr const Bitboard& rooks() const { return material[rookId]; }
    [[nodiscard]] constexpr const Bitboard& bishops() const { return material[bishopId]; }
    [[nodiscard]] constexpr const Bitboard& knights() const { return material[knightId]; }
    [[nodiscard]] constexpr const Bitboard& pawns() const { return material[pawnId]; }
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
    bool hasNone() const { return m_innerState == NONE; }
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

    byte read() const { return m_innerState; }
    void write(byte state) { m_innerState = state; }

    std::string toString() const;

    bool operator==(const CastlingStateInfo& rhs) const { return m_innerState == rhs.m_innerState; }

private:
    byte m_innerState;
};

struct EnPassantStateInfo {
public:
    EnPassantStateInfo() = default;
    operator bool() const { return m_innerState != 0; }
    void clear() { m_innerState = 0; }

    void writeSquare(Square sq)
    {
        m_innerState = static_cast<byte>(sq);
        m_innerState = m_innerState << 2;
        m_innerState += 1;
        // m_innerState += ((byte)set << 1);
    }

    Square readSquare() const
    {
        if (*this)
            return static_cast<Square>(m_innerState >> 2);
        else
            return Square::NullSQ;
    }

    Square readTarget() const
    {
        byte sq = m_innerState >> 2;
        // Set set = static_cast<Set>(m_innerState & 2);
        if (sq < 31) {
            return static_cast<Square>(sq + 8);
        }
        return static_cast<Square>(sq - 8);
    }
    Bitboard readBitboard() const
    {
        if (*this == true) {
            Square sq = readSquare();
            return squareMaskTable[(size_t)sq];
        }
        return 0;
    }

    std::string toString() const
    {
        if (*this == true) {
            return Notation::toString(readSquare());
        }
        return "-";
    }

    // read & write will mainly be used by make / unmake to track state.
    byte read() const { return m_innerState; }
    void write(byte state) { m_innerState = state; }

private:
    // [sqr] [sqr] [sqr] [sqr] [sqr] [sqr] [set] [hasEnPassant]
    byte m_innerState;
};

/**
 * A chess position, represented as a set of bitboards and some bytes of additional state.
 * 96 bytes of material information, could be reduced to 64 by using 2 boards for set and 6 for pieces
 * instaed of 2x6.
 * 1 byte of castling information.
 * 1 byte for enpassant information.
 * 7 bits for halfmoves // no point in tracking it past 100
 * 1 bit determining active set
 * 2 bytes for fullmoves  (max number 65,535)
 * */
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
    ChessPiece readPieceAt(Square sqr) const;

    bool empty() const;

    template<Set us>
    const MaterialMask& readMaterial() const;
    const MaterialMask& readMaterial(Set set) const
    {
        return set == Set::WHITE ? readMaterial<Set::WHITE>() : readMaterial<Set::BLACK>();
    }

    EnPassantStateInfo& editEnPassant() { return m_enpassantState; }
    EnPassantStateInfo readEnPassant() const { return m_enpassantState; }

    CastlingStateInfo& editCastling() { return m_castlingState; }
    CastlingStateInfo readCastling() const { return m_castlingState; }
    const CastlingStateInfo& refCastling() const { return m_castlingState; }

    template<Set us>
    Bitboard calcAvailableMovesPawnBulk(const KingPinThreats& kingPinThreats, bool captures = false) const;
    template<Set us>
    Bitboard calcAvailableMovesKnightBulk(const KingPinThreats& kingPinThreats, bool captures = false) const;
    template<Set us, u8 pieceId = rookId>
    Bitboard calcAvailableMovesRookBulk(const KingPinThreats& kingPinThreats, bool captures = false) const;
    template<Set us, u8 pieceId = bishopId>
    Bitboard calcAvailableMovesBishopBulk(const KingPinThreats& kingPinThreats, bool captures = false) const;
    template<Set us>
    Bitboard calcAvailableMovesQueenBulk(const KingPinThreats& kingPinThreats, bool captures = false) const;
    template<Set us, Set op = opposing_set<us>()>
    Bitboard calcAvailableMovesKing(byte castlingRights, bool captures = false) const;

    template<Set us>
    SlidingMaterialMasks calcMaterialSlidingMasksBulk() const;

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

    template<Set us>
    std::tuple<Bitboard, Bitboard> isolatePiece(u8 pieceId, Notation source, Bitboard movesbb,
                                                const KingPinThreats& kingPinThreats) const;

    template<Set us, u8 pieceId>
    std::tuple<Bitboard, Bitboard> isolatePiece(Notation source, Bitboard movesbb, const KingPinThreats& kingPinThreats) const;

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
    KingPinThreats calcKingMask(ChessPiece king, Square source, const SlidingMaterialMasks& opponentSlidingMask) const;
    template<Set us>
    KingPinThreats calcKingMask() const;

private:
    template<Set us, u8 direction, u8 pieceId>
    Bitboard internalCalculateThreat(Bitboard bounds) const;
    template<Set us, u8 direction, u8 pieceId>
    Bitboard internalCalculateThreat(Bitboard bounds, Bitboard piecebb, Bitboard materialbb, Bitboard opMaterial) const;

    /**
     * @brief Isolate a given pawn from the moves bitboard.
     * The following functions all do the same thing, but for different pieces. They take a bitboard representing all
     * available moves for a given piece type, and isolate the moves that are valid for the given piece at source square. */
    template<Set us>
    std::tuple<Bitboard, Bitboard> internalIsolatePawn(Notation source, Bitboard movesbb,
                                                       const KingPinThreats& kingPinThreats) const;
    template<Set us>
    std::tuple<Bitboard, Bitboard> internalIsolateKnightMoves(Notation source, Bitboard movesbb,
                                                              const KingPinThreats& kingPinThreats) const;
    template<Set us>
    std::tuple<Bitboard, Bitboard> internalIsolateBishop(Notation source, Bitboard movesbb,
                                                         const KingPinThreats& kingPinThreats, i8 pieceIndex = bishopId) const;
    template<Set us>
    std::tuple<Bitboard, Bitboard> internalIsolateRook(Notation source, Bitboard movesbb, const KingPinThreats& kingPinThreats,
                                                       i8 pieceIndex = rookId) const;

    Bitboard SlidingMaterialCombined(byte set) const;
    u64 Castling(byte set, byte castling, u64 threatenedMask) const;

    mutable MaterialMask m_material[2];
    CastlingStateInfo m_castlingState;
    EnPassantStateInfo m_enpassantState;
};

template<Set us>
const MaterialMask&
Position::readMaterial() const
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
Position::internalCalculateThreat(Bitboard bounds) const
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

        bbCopy = bbCopy.shiftRelative<us, direction>();
        moves |= bbCopy;
        bbCopy ^= (materialbb & bbCopy);

    } while (bbCopy.empty() == false);

    return moves;
}

template<Set us, u8 direction, u8 pieceId>
Bitboard
Position::internalCalculateThreat(Bitboard bounds, Bitboard piecebb, Bitboard materialbb, Bitboard opMaterial) const
{
    bounds |= opMaterial;

    Bitboard bbCopy = piecebb;
    Bitboard moves = 0;
    do {
        Bitboard purge = bbCopy & bounds;
        bbCopy &= ~purge;

        bbCopy = bbCopy.shiftRelative<us, direction>();
        moves |= bbCopy;
        bbCopy ^= (materialbb & bbCopy);

    } while (bbCopy.empty() == false);

    return moves;
}

template<Set us, bool includeMaterial, bool pierceKing>
Bitboard
Position::calcThreatenedSquares() const
{
    Bitboard result = ~universe;
    [[maybe_unused]] constexpr Set op = opposing_set<us>();
    [[maybe_unused]] Bitboard kingMask = 0;

    // removing king from opmaterial so it doesn't stop our sliding.
    // needs to be reset later on.
    if constexpr (pierceKing) {
        // can we build a scoped struct to make this a bit cleaner?
        kingMask = readMaterial<op>()[kingId];
        m_material[(size_t)op].material[kingId] = 0;
    }

    result |= calcThreatenedSquaresPawnBulk<us>();
    result |= calcThreatenedSquaresKnightBulk<us>();
    result |= calcThreatenedSquaresBishopBulk<us>();
    result |= calcThreatenedSquaresBishopBulk<us, queenId>();
    result |= calcThreatenedSquaresRookBulk<us>();
    result |= calcThreatenedSquaresRookBulk<us, queenId>();
    result |= calcThreatenedSquaresKing<us>();

    if constexpr (pierceKing)
        m_material[(size_t)op].material[kingId] = kingMask;

    if constexpr (includeMaterial)
        result |= readMaterial<us>().combine();

    return result;
}

template<Set us>
SlidingMaterialMasks
Position::calcMaterialSlidingMasksBulk() const
{
    Bitboard orthogonal;
    Bitboard diagonal;

    diagonal |= calcThreatenedSquaresBishopBulk<us>();
    diagonal |= calcThreatenedSquaresBishopBulk<us, queenId>();

    orthogonal |= calcThreatenedSquaresRookBulk<us>();
    orthogonal |= calcThreatenedSquaresRookBulk<us, queenId>();

    // add material
    diagonal |= readMaterial<us>()[bishopId] | readMaterial<us>()[queenId];
    orthogonal |= readMaterial<us>()[rookId] | readMaterial<us>()[queenId];

    return {orthogonal, diagonal};
}

template<Set us, u8 pieceId>
std::tuple<Bitboard, Bitboard>
Position::isolatePiece(Notation source, Bitboard movesbb, const KingPinThreats& kingMask) const
{
    return isolatePiece<us>(pieceId, source, movesbb, kingMask);
}