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
 * @file material_topology.hpp
 * @brief MaterialTopology class used by move generation to calculate threatened squares which often results in available moves.
 * Separated from position and move generation for clarity and separation of concerns. Heavy use of bitboards and bitboard 
 * operations. Doesn't hold any internal state so everything is marked as nodiscard to encourage use of the return value.
 * 
 * This class is templated on the Set type to allow easier use.
 *
 * @author Alexander Loodin Ek    */

#pragma once
#include <material/chess_piece_defines.hpp>
#include <bitboard.hpp>

struct MaterialPositionMask;

template<Set us>
class MaterialTopology {
private:
    const MaterialPositionMask& m_material;
    
public:
    MaterialTopology(const MaterialPositionMask& material) : m_material(material) {}

    [[nodiscard]] Bitboard computeThreatenedSquaresPawnBulk() const;
    [[nodiscard]] Bitboard computeThreatenedSquaresKnightBulk() const;   
    
    // Bulk threat calculation for bishops and rooks. Template parameter is used by queens to combine the two.
    template<u8 pieceId = bishopId>
    [[nodiscard]] Bitboard computeThreatenedSquaresBishopBulk(Bitboard occupancy) const;
    template<u8 pieceId = rookId>
    [[nodiscard]] Bitboard computeThreatenedSquaresRookBulk(Bitboard occupancy) const;
    
    [[nodiscard]] Bitboard computeThreatenedSquaresQueenBulk(Bitboard occupancy) const;
    [[nodiscard]] Bitboard computeThreatenedSquaresKing() const;

    [[nodiscard]] Bitboard computeThreatenedDiagonals() const;
    [[nodiscard]] Bitboard computeThreatenedOrthogonals() const;

    template<bool includeMaterial, bool pierceKing>
    [[nodiscard]] Bitboard computeThreatenedSquares() const;
};