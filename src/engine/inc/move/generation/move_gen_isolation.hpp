// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021-2025  Alexander Loodin Ek

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
 * @file move_gen_isolation.hpp
 * @brief PieceIsolator class used by move generation to isolate moves from a bulk generated bitboard for a specific piece.
 *
 * @author Alexander Loodin Ek */
#pragma once
#include <bitboard/bitboard.hpp>
#include <bitboard/attacks/attacks.hpp>
#include <material/chess_piece_defines.hpp>
#include <move/generation/king_pin_threats.hpp>
#include <move/generation/move_gen_isolation.hpp>
#include <position/position_accessors.hpp>
#include <position/position.hpp>


struct MovesMask {
    Bitboard quiets;
    Bitboard captures;
};

template<Set us, u8 pieceId>
class PieceIsolator
{
public:
    PieceIsolator(PositionReader position, Bitboard moveMask, const KingPinThreats<us>& pinThreats)
        : m_position(position), m_movesMask(moveMask), m_pinThreats(pinThreats) {}
    
    inline MovesMask isolate(Square src) const;

private:
    const KingPinThreats<us>& m_pinThreats;
    PositionReader m_position;
    Bitboard m_movesMask;
};

template <u8 pieceId> struct PieceIsoImpl;

template<Set us, u8 pieceId>
MovesMask PieceIsolator<us, pieceId>::isolate(Square src) const {
    return PieceIsoImpl<pieceId>::template apply<us>(m_position, m_movesMask, src, m_pinThreats);
}

// Pawn
template <>
struct PieceIsoImpl<pawnId> {
    template<Set us>
    static MovesMask apply(PositionReader position, Bitboard movesbb, Square source, const KingPinThreats<us>& pinThreats) {
        const size_t usIndx = static_cast<size_t>(us);

        Bitboard opMatCombined = position.material().combine<opposing_set<us>()>() | position.enPassant().readBitboard();
        Bitboard srcMask = Bitboard(squareMaskTable[*source]);

        const Bitboard pinned = pinThreats.pinned(srcMask);

        // special case for when there is a enpassant available.
        if (position.enPassant()) {        
            Bitboard potentialPin(pinThreats.readEnPassantMask() & srcMask);
            if (potentialPin) {
                opMatCombined ^= position.enPassant().readBitboard();
            }
        }

        Bitboard threatns;
        if ((srcMask & board_constants::boundsRelativeMasks[usIndx][west]).empty())
            threatns |= srcMask.shiftNorthWestRelative<us>();
        if ((srcMask & board_constants::boundsRelativeMasks[usIndx][east]).empty())
            threatns |= srcMask.shiftNorthEastRelative<us>();

        Bitboard isolatedbb = srcMask.shiftNorthRelative<us>();
        Bitboard unoccupied = ~(position.material().combine<us>() | opMatCombined);
        Bitboard doublePush = isolatedbb & pawn_constants::baseRank[usIndx] & unoccupied;
        isolatedbb |= doublePush.shiftNorthRelative<us>();
        isolatedbb &= unoccupied;    

        if (pinThreats.isChecked()) {
            Bitboard checksMask(pinThreats.checks());
            if (position.enPassant()) {
                auto otherMask = squareMaskTable[(u32)position.enPassant().readTarget()];
                if (checksMask & otherMask) 
                    checksMask |= position.enPassant().readBitboard();
            }
            isolatedbb &= checksMask;
            threatns &= checksMask;
        }

        if (srcMask & pinned) {
            isolatedbb &= pinned;
            threatns &= pinned;
        }
        return { movesbb & isolatedbb, movesbb & opMatCombined & threatns };
    }
};

// knight
template<>
struct PieceIsoImpl<knightId> {
    template<Set us>
    static MovesMask apply(PositionReader position, Bitboard movesbb, Square source, const KingPinThreats<us>& pinThreats) {
        u64 srcMask = squareMaskTable[*source];
        Bitboard opMatCombined = position.material().combine<opposing_set<us>()>();

        // figure out if piece is pinned
        const Bitboard pinned = pinThreats.pinned(srcMask);
        if (pinned.empty() == false) {
            movesbb &= pinned;
        }

        if (pinThreats.isChecked())
            movesbb &= pinThreats.checks();

        movesbb &= attacks::getKnightAttacks(*source);
        movesbb &= ~position.material().combine<us>();

        return { movesbb & ~opMatCombined, movesbb & opMatCombined };
    }
};

// bishop
template<>
struct PieceIsoImpl<bishopId> {
    template<Set us>
    static MovesMask apply(PositionReader position, Bitboard movesbb, Square source, const KingPinThreats<us>& pinThreats) {
    const Bitboard opMatCombined = position.material().combine<opposing_set<us>()>();
    const Bitboard allMaterial = position.material().combine();
    const Bitboard usMaterial = opMatCombined ^ allMaterial;

    // figure out if piece is pinned
    u64 srcMask = squareMaskTable[*source];
    const Bitboard pinned = pinThreats.pinned(srcMask);
    if (pinned.empty() == false) {
        movesbb &= pinned;
    }

    if (pinThreats.isChecked()) {
        movesbb &= pinThreats.checks();
    }

    movesbb &= attacks::getBishopAttacks(*source, allMaterial.read());
    movesbb &= ~usMaterial;

    return { movesbb & ~opMatCombined, movesbb & opMatCombined };
    }
};

// rook
template<>
struct PieceIsoImpl<rookId> {
    template<Set us>
    static MovesMask apply(PositionReader position, Bitboard movesbb, Square source, const KingPinThreats<us>& pinThreats) {
        const Bitboard opMatCombined = position.material().combine<opposing_set<us>()>();
        const Bitboard allMaterial = position.material().combine();
        const Bitboard usMaterial = opMatCombined ^ allMaterial;

        // figure out if piece is pinned
        u64 srcMask = squareMaskTable[*source];
        const Bitboard pinned = pinThreats.pinned(srcMask);
        if (pinned.empty() == false) {
            movesbb &= pinned;
        }

        if (pinThreats.isChecked()) {
            movesbb &= pinThreats.checks();
        }

        movesbb &= attacks::getRookAttacks(*source, allMaterial.read());
        movesbb &= ~usMaterial;

        return { movesbb & ~opMatCombined, movesbb & opMatCombined };
    }
};

// queen
template<>
struct PieceIsoImpl<queenId> {
    template<Set us>
    static MovesMask apply(PositionReader position, Bitboard movesbb, Square source, const KingPinThreats<us>& pinThreats) {
        auto ortho = PieceIsoImpl<rookId>::apply<us>(position, movesbb, source, pinThreats);
        auto diag = PieceIsoImpl<bishopId>::apply<us>(position, movesbb, source, pinThreats);
        return { ortho.quiets | diag.quiets, ortho.captures | diag.captures };
    }
};


