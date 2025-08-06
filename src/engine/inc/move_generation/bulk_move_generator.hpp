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
 * @file bulk_move_generator.hpp
 * @brief BulkMoveGenerator class used by move generation to calculate and cache pseudo-legal moves for a given position.
 *
 * @author Alexander Loodin Ek    */
#pragma once

#include <bitboard.hpp>
#include <move.h>
#include <move_generation/king_pin_threats.hpp>
#include <position/position_accessors.hpp>

template<Set set, MoveTypes moveFilter = MoveTypes::ALL>
class BulkMoveGenerator {
public:
    BulkMoveGenerator(const Position& position, const MoveGenerator& moveGen) :
        m_position(position),
        m_moveGen(moveGen)
    {}

    void compute();
    void compute(PieceType ptype);


    Bitboard computeBulkPawnMoves() const;
    Bitboard computeBulkKnightMoves() const;
    template<u8 pieceId = rookId>
    Bitboard computeBulkRookMoves() const;
    template<u8 pieceId = bishopId>
    Bitboard computeBulkBishopMoves() const;
    Bitboard computeBulkQueenMoves() const;
    
    template<Set op = opposing_set<us>()>
    Bitboard calcAvailableMovesKing(byte castlingRights) const;

private:
    const MoveGenerator& m_moveGen;
    PositionReader m_position;
    Bitboard moves[2][6];
    
};

template<Set us, MoveTypes moveFilter>
void BulkMoveGenerator<us, moveFilter>::compute() {
    // Generate all moves for the given set and move filter
    if (m_position.empty()) return;

    const size_t usIndx = static_cast<size_t>(us);

    m_moveGen.readKingPinThreats<us>();
}

template<Set us, MoveTypes moveFilter>
Bitboard BulkMoveGenerator<us, moveFilter>::computeBulkPawnMoves() const {
    const size_t usIndx = static_cast<size_t>(us);
    const Bitboard usMat = m_materialMask.combine<us>();
    const Bitboard opMat = m_materialMask.combine<opposing_set<us>()>();
    const Bitboard unoccupied(~(usMat | opMat));
    const Bitboard piecebb = m_materialMask.pawns<us>();

    Bitboard movesMask;
    movesMask = piecebb.shiftNorthRelative<us>();
    Bitboard doublePush = movesMask & pawn_constants::baseRank[usIndx] & unoccupied;
    movesMask |= doublePush.shiftNorthRelative<us>();

    movesMask &= unoccupied;

    const Bitboard threatenedSquares = m_position.readMaterial().topology<us>().computeThreatenedSquaresPawnBulk();
    movesMask |= (opMat | m_enpassantState.readBitboard()) & threatenedSquares;

    // TODO: I think by moving this kingPinThreats to the individual move generation portion we could remove this code.
    auto kingMask = m_moveGen.readKingPinThreats<us>();

    if (kingMask.isChecked()) {
        Bitboard checksMask(kingMask.checks());
        auto otherMask = squareMaskTable[(u32)m_enpassantState.readTarget()];
        if (checksMask & otherMask) {
            checksMask |= m_enpassantState.readBitboard();
        }
        movesMask &= checksMask;
    }

    if constexpr (moveFilter == MoveTypes::CAPTURES_ONLY)
        movesMask &= opMat;

    return movesMask;
}

template<Set us, MoveTypes moveFilter>
Bitboard BulkMoveGenerator<us, moveFilter>::computeBulkKnightMoves() const {
    auto moves = m_position.readMaterial().topology<us>().computeThreatenedSquaresKnightBulk();
    const Bitboard ourMaterial = readMaterial().combine<us>();

    moves &= ~ourMaterial;

    if (kingMask.isChecked())
        return moves & kingMask.checks();

    if constexpr (moveFilter == MoveTypes::CAPTURES_ONLY)
        return moves & readMaterial().combine<opposing_set<us>()>();

    return moves;
}