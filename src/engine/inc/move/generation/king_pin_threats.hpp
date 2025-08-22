/******************************************************************************
 * Elephant Gambit Chess Engine - a Chess AI
 * Copyright(C) 2025  Alexander Loodin Ek
 * 
 * This program is free software : you can redistribute it and /or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.If not, see < http://www.gnu.org/licenses/>. 
 *****************************************************************************/

/**
 * @file king_pin_threats.hpp
 * @brief Implements functionality to assess king pin threats and check conditions in a chess game 
 * 
 */

#pragma once

#include <bitboard/bitboard.hpp>
#include <material/chess_piece_defines.hpp>
#include <position/position_accessors.hpp>

struct SlidingMaterialMasks {
    Bitboard orthogonal;
    Bitboard diagonal;
};

template<Set us>
class KingPinThreats {
public:
    KingPinThreats(Square kingSquare, PositionReader position);

    [[nodiscard]] u32 isCheckedCount() const;
    [[nodiscard]] bool isChecked() const;
    [[nodiscard]] Bitboard combined() const;
    [[nodiscard]] Bitboard pins() const;
    [[nodiscard]] Bitboard checks() const;
    [[nodiscard]] Bitboard pinned(Bitboard mask) const;

    [[nodiscard]] const Bitboard readEnPassantMask() const { return m_specialEnPassantMask; }
    [[nodiscard]] const Bitboard* readOpponentOpenAngles() const { return &m_opponentOpenAngles[0]; }
    // [[nodiscard]] const Bitboard& readKnightsAndPawns() const { return m_knightsAndPawns; }
    // [[nodiscard]] const Bitboard* readThreatenedAngles() const { return &m_threatenedAngles[0]; }
    
    void calculateOpponentOpenAngles(const Square kingSquare, PositionReader position);
    
private:
    void compute(Square kingSquare, PositionReader position);
    void calculateEnPassantPinThreat(Square kingSquare, PositionReader position);

    Bitboard m_threatenedAngles[8];
    Bitboard m_specialEnPassantMask;
    Bitboard m_opponentOpenAngles[2];
    Bitboard m_knightsAndPawns;
    bool m_checkedAngles[8];
    bool m_knightOrPawnCheck;
};