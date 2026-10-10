// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2025  Alexander Loodin Ek

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
 * @file check_info.hpp
 * @brief tusk::CheckInfo, what the side to move's king is facing: the pieces giving check & our pinned pieces.
 *
 * Built from lines rather than per direction state. Checkers are the attackers of the king square, pins come from
 * x-raying the king through our own pieces: an enemy slider with exactly one of our pieces between it and the king
 * pins that piece. A pinned piece may only move along the line through the king (ray::getLine).
 *
 *     tusk::CheckInfo<us> checkInfo(position);
 *     tusk::MoveGenerator<us> generator(position, checkInfo);
 *
 * @author Alexander Loodin Ek    */

#pragma once

#include <bitboard/attacks/attacks.hpp>
#include <bitboard/bitboard_constants.hpp>
#include <bitboard/intrinsics.hpp>
#include <bitboard/rays/rays.hpp>
#include <material/chess_piece_defines.hpp>
#include <position/position_accessors.hpp>

namespace tusk {

template<Set us>
class CheckInfo {
public:
    explicit CheckInfo(PositionReader position) {
        constexpr Set op = opposing_set<us>();
        const MaterialPositionMask& material = position.material();

        const u64 king = material.king<us>().read();
        if (king == 0)
            return;  // positions without our king, nothing can check or pin it.

        m_kingSqr = intrinsics::lsbIndex(king);
        const u64 opMat = material.combine<op>().read();
        const u64 occupancy = opMat | material.combine<us>().read();
        const u64 queens = material.queens<op>().read();
        const u64 orthogonals = material.rooks<op>().read() | queens;
        const u64 diagonals = material.bishops<op>().read() | queens;

        // an opponent pawn checks from the squares one of our pawns on the king square would attack.
        m_checkers = pawnAttacks(king) & material.pawns<op>().read();
        m_checkers |= attacks::getKnightAttacks(m_kingSqr) & material.knights<op>().read();

        // sliders, seen from the king through our own pieces, only opponent pieces block.
        u64 snipers = (attacks::getRookAttacks(m_kingSqr, opMat) & orthogonals)
                    | (attacks::getBishopAttacks(m_kingSqr, opMat) & diagonals);
        while (snipers) {
            const u32 sniperSqr = intrinsics::lsbIndex(snipers);
            const u64 sniper = snipers & (0 - snipers);
            snipers ^= sniper;

            // getRay is the squares between king & sniper plus the sniper itself.
            const u64 blockers = ray::getRay(m_kingSqr, sniperSqr) & occupancy & ~sniper;
            if (blockers == 0)
                m_checkers |= sniper;
            else if (intrinsics::resetLsb(blockers) == 0)
                m_pinned |= blockers;  // exactly one blocker, it's one of ours since opponent pieces stop the x-ray.
        }
    }

    CheckInfo(const CheckInfo&) = delete;
    CheckInfo& operator=(const CheckInfo&) = delete;

    [[nodiscard]] bool isChecked() const { return m_checkers != 0; }
    [[nodiscard]] u32 checkCount() const { return static_cast<u32>(intrinsics::popcnt(m_checkers)); }
    [[nodiscard]] u64 checkers() const { return m_checkers; }
    [[nodiscard]] u64 pinned() const { return m_pinned; }
    [[nodiscard]] u32 kingSquare() const { return m_kingSqr; }

    // squares a non king move must land on, the checker or between it & the king. Everything when not in check,
    // nothing in double check where only the king can move.
    [[nodiscard]] u64 checkMask() const {
        if (m_checkers == 0)
            return ~0ull;
        if (intrinsics::resetLsb(m_checkers) != 0)
            return 0;
        // getRay is empty for a knight or pawn, they aren't on a line with the king, the checker alone is the mask.
        return ray::getRay(m_kingSqr, intrinsics::lsbIndex(m_checkers)) | m_checkers;
    }

private:
    static constexpr u64 pawnAttacks(u64 pawns) {
        if constexpr (us == Set::WHITE)
            return ((pawns & ~board_constants::filehMask) << 9) | ((pawns & ~board_constants::fileaMask) << 7);
        else
            return ((pawns & ~board_constants::filehMask) >> 7) | ((pawns & ~board_constants::fileaMask) >> 9);
    }

    u64 m_checkers = 0;
    u64 m_pinned = 0;
    u32 m_kingSqr = 0;
};

} // namespace tusk
