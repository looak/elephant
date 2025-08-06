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
#include "bitboard_constants.hpp"
#include <material/chess_piece.hpp>
#include "defines.hpp"
#include "intrinsics.hpp"
#include <move_generation/king_pin_threats.hpp>
#include "notation.h"
#include <material/material_mask.hpp>
#include <material/material_topology.hpp>
#include "position/castling_state_info.hpp"
#include "position/en_passant_state_info.hpp"
#include "position/position_accessors.hpp"

struct Notation;


/**
 * A chess position, represented as a set of bitboards and some bytes of additional state.
 * 64 bytes of material information, by using 2 boards for set and 6 for pieces
 * 1 byte of castling information.
 * 1 byte for enpassant information.
 * 7 bits for halfmoves // no point in tracking it past 100
 * 1 bit determining active set
 * 2 bytes for fullmoves  (max number 65,535) */
class Position {
    friend class PositionProxy<PositionEditPolicy>;
    friend class PositionProxy<PositionReadOnlyPolicy>;
public:
    static bool IsValidSquare(signed short currSqr);
    static bool IsValidSquare(Notation source);

public:
    Position();
    Position(const Position& other);
    Position& operator=(const Position& other);

    PositionEditor edit() { return PositionEditor(*this); }
    PositionReader read() const { return PositionReader(*this); }

    ChessPiece readPieceAt(Square square) const;

protected:
    u64 readHash() const { return m_hash; }
    void setHash(u64 hash) { m_hash = hash; }

    MaterialPositionMask& editMaterialMask() { return m_materialMask; }
    
    template<Set us, bool captures = false>
    Bitboard calcAvailableMovesPawnBulk(const KingPinThreats& kingPinThreats) const;
    template<Set us, bool captures = false>
    Bitboard calcAvailableMovesKnightBulk(const KingPinThreats& kingPinThreats) const;
    template<Set us, bool captures = false, u8 pieceId = rookId>
    Bitboard calcAvailableMovesRookBulk(const KingPinThreats& kingPinThreats) const;
    template<Set us, bool captures = false, u8 pieceId = bishopId>
    Bitboard calcAvailableMovesBishopBulk(const KingPinThreats& kingPinThreats) const;
    template<Set us, bool captures = false>
    Bitboard calcAvailableMovesQueenBulk(const KingPinThreats& kingPinThreats) const;
    template<Set us, bool captures = false, Set op = opposing_set<us>()>
    Bitboard calcAvailableMovesKing(byte castlingRights) const;
    
private:

    /**
     * @brief Isolate a given pawn from the moves bitboard.
     * The following functions all do the same thing, but for different pieces. They take a bitboard representing all
     * available moves for a given piece type, and isolate the moves that are valid for the given piece at source square. */
    

    u64 Castling(byte set, byte castling, u64 threatenedMask) const;

    mutable MaterialPositionMask m_materialMask;
    CastlingStateInfo m_castlingState;
    EnPassantStateInfo m_enpassantState;
    u64 m_hash = 0;
};