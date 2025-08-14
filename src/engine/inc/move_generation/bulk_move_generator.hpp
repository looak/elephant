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
#include <move/move.hpp>
#include <move_generation/king_pin_threats.hpp>
#include <position/position_accessors.hpp>

class BulkMoveGenerator {
public:
    BulkMoveGenerator(PositionReader position) :
        m_position(position)
    {}

    // void compute();
    // void compute(PieceType ptype);

    template<Set set, MoveTypes moveFilter = MoveTypes::ALL>
    Bitboard computeBulkPawnMoves() const;

    template<Set set, MoveTypes moveFilter = MoveTypes::ALL>
    Bitboard computeBulkKnightMoves() const;

    template<Set set, u8 pieceId = rookId, MoveTypes moveFilter = MoveTypes::ALL>
    Bitboard computeBulkRookMoves() const;

    template<Set set, u8 pieceId = bishopId, MoveTypes moveFilter = MoveTypes::ALL>
    Bitboard computeBulkBishopMoves() const;

    template<Set set, MoveTypes moveFilter = MoveTypes::ALL>
    Bitboard computeBulkQueenMoves() const;
    
    template<Set set, Set op = opposing_set<set>(), MoveTypes moveFilter = MoveTypes::ALL>
    Bitboard computeKingMoves() const;

    template<Set us>
    Bitboard computeCastlingMoves(CastlingStateInfo castling, Bitboard threatenedMask) const;

    template<Set set, MoveTypes moveFilter = MoveTypes::ALL>
    Bitboard computeBulkMovesGeneric(u8 pieceId) const;

private:
    PositionReader m_position;
    // Bitboard moves[2][6];    
};

template<Set us, MoveTypes moveFilter>
Bitboard BulkMoveGenerator::computeBulkPawnMoves() const {
    const MaterialPositionMask& material = m_position.material();
    const size_t usIndx = static_cast<size_t>(us);
    const Bitboard usMat = material.combine<us>();
    const Bitboard opMat = material.combine<opposing_set<us>()>();
    const Bitboard unoccupied(~(usMat | opMat));
    const Bitboard piecebb = material.pawns<us>();

    Bitboard movesMask;
    movesMask = piecebb.shiftNorthRelative<us>();
    Bitboard doublePush = movesMask & pawn_constants::baseRank[usIndx] & unoccupied;
    movesMask |= doublePush.shiftNorthRelative<us>();

    movesMask &= unoccupied;

    const Bitboard threatenedSquares = material.topology<us>().computeThreatenedSquaresPawnBulk();
    movesMask |= (opMat | m_position.enPassant().readBitboard()) & threatenedSquares;

    // moved this code to MoveGenerator
    // auto kingMask = m_moveGen.readKingPinThreats<us>();

    // if (kingMask.isChecked()) {
    //     Bitboard checksMask(kingMask.checks());
    //     auto otherMask = squareMaskTable[(u32)m_position.enPassant().readTarget()];
    //     if (checksMask & otherMask) {
    //         checksMask |= m_position.enPassant().readBitboard();
    //     }
    //     movesMask &= checksMask;
    // }

    if constexpr (moveFilter == MoveTypes::CAPTURES_ONLY)
        movesMask &= opMat;

    return movesMask;
}

template<Set us, MoveTypes moveFilter>
Bitboard BulkMoveGenerator::computeBulkKnightMoves() const {
    const MaterialPositionMask& material = m_position.material();
    const Bitboard ourMaterial = material.combine<us>();
    Bitboard moves = material.topology<us>().computeThreatenedSquaresKnightBulk();

    moves &= ~ourMaterial;
    
    // auto kingMask = m_moveGen.readKingPinThreats<us>();
    // if (kingMask.isChecked())
    //     return moves & kingMask.checks();

    if constexpr (moveFilter == MoveTypes::CAPTURES_ONLY)
        return moves & material.combine<opposing_set<us>()>();

    return moves;
}


template<Set us, Set op, MoveTypes moveFilter>
Bitboard BulkMoveGenerator::computeKingMoves() const
{
    bool constexpr includeMaterial = false;
    bool constexpr pierceKing = true;
    const MaterialPositionMask& material = m_position.material();

    Bitboard threatened = material.topology<op>().computeThreatenedSquares<includeMaterial, pierceKing>();
    Bitboard moves = material.topology<us>().computeThreatenedSquaresKing();

    // remove any squares blocked by our own pieces.
    moves &= ~material.combine<us>();
    moves &= ~threatened;
    if ((threatened & material.king<us>()).empty()) // we're not in check
        moves |= computeCastlingMoves<us>(m_position.castling(), threatened);
    if constexpr (moveFilter == MoveTypes::CAPTURES_ONLY)
        moves &= material.combine<op>();
    return moves;
}

template<Set us, u8 pieceId, MoveTypes moveFilter>
Bitboard BulkMoveGenerator::computeBulkBishopMoves() const
{
    const MaterialPositionMask& material = m_position.material();
    const Bitboard materialbb = material.combine<us>();
    const Bitboard occupancy = material.combine();
    // const auto& kingMask = m_moveGen.readKingPinThreats<us>();
    Bitboard moves = material.topology<us>().computeThreatenedSquaresBishopBulk(occupancy);

    // if (kingMask.isChecked())
    //     moves &= kingMask.checks();
    // else
    moves ^= (materialbb & moves); // Can't capture our own pieces
    // moves &= ~ourMaterial; // seems cleaner

    if constexpr (moveFilter == MoveTypes::CAPTURES_ONLY)
        moves &= material.combine<opposing_set<us>()>();

    return moves;
}

template<Set us, u8 pieceId, MoveTypes moveFilter>
Bitboard BulkMoveGenerator::computeBulkRookMoves() const
{
    const MaterialPositionMask& material = m_position.material();
    const Bitboard materialbb = material.combine<us>();
    const Bitboard occupancy = material.combine();
 //   const auto& kingMask = m_moveGen.readKingPinThreats<us>();
    Bitboard moves = material.topology<us>().computeThreatenedSquaresRookBulk(occupancy);

    // if (kingMask.isChecked())
    //     moves &= kingMask.checks();
    // else
    moves ^= (materialbb & moves); // Can't capture our own pieces
    // moves &= ~ourMaterial; // seems cleaner

    if constexpr (moveFilter == MoveTypes::CAPTURES_ONLY)
        moves &= material.combine<opposing_set<us>()>();

    return moves;
}

template<Set us, MoveTypes moveFilter>
Bitboard BulkMoveGenerator::computeBulkQueenMoves() const
{
    Bitboard moves = 0;
    moves |= computeBulkBishopMoves<us, queenId, moveFilter>();
    moves |= computeBulkRookMoves<us, queenId, moveFilter>();
    return moves;
}

template<Set us>
Bitboard BulkMoveGenerator::computeCastlingMoves(CastlingStateInfo castlingState, Bitboard threatenedMask) const
{
    Bitboard retVal = ~universe;
    byte rank = 0;
    u8 castling = castlingState.read();

    if constexpr (us == Set::BLACK) {
        rank = 7;
        // shift castling right
        // this should make black castling 1 & 2
        castling = castling >> 2;
    }

    // early out in case we don't have any castling available to us.
    if (castling == 0)
        return retVal;

    const Bitboard attacked = threatenedMask;
    const Bitboard combMat = m_position.material().combine();

    // check king side
    if (castling & 1) {
        // build castling square mask
        byte fsqr = (rank * 8) + 5;
        byte gsqr = fsqr + 1;
        Bitboard mask = ~universe;
        mask |= squareMaskTable[fsqr];
        mask |= squareMaskTable[gsqr];

        if (!(attacked & mask) && !(combMat & mask))
            retVal |= squareMaskTable[gsqr];
    }
    // check queen side
    if (castling & 2) {
        // build castling square mask
        byte bsqr = (rank * 8) + 1;
        byte csqr = bsqr + 1;
        byte dsqr = csqr + 1;
        u64 threatMask = ~universe;
        u64 blockedMask = ~universe;
        threatMask |= squareMaskTable[csqr];
        threatMask |= squareMaskTable[dsqr];

        blockedMask |= threatMask;
        blockedMask |= squareMaskTable[bsqr];

        if (!(attacked & threatMask) && !(combMat & blockedMask))
            retVal |= squareMaskTable[csqr];
    }
    return retVal;
}

template<Set set, MoveTypes moveFilter>
Bitboard BulkMoveGenerator::computeBulkMovesGeneric(u8 pieceId) const
{
    if (pieceId == bishopId)
        return computeBulkBishopMoves<set, bishopId, moveFilter>();
    else if (pieceId == rookId)
        return computeBulkRookMoves<set, rookId, moveFilter>();
    else if (pieceId == queenId)
        return computeBulkQueenMoves<set, moveFilter>();
    else
        LOG_ERROR() << "Unsupported piece type for generic move generation: " << pieceId;

    return 0;
}