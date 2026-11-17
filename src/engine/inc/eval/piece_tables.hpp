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
 * @file piece_tables.hpp
 * @brief Piece values, see pieces.hpp. Written by phant-tuner, laid out the way it writes them. All constexpr, safe
 * to include anywhere.
 *
 * Tuned by phant-tuner, every other value kept, on E12.52-1M-D12-Resolved (10M positions, 1000 passes at rate 1).
 * Pairs: --tune pieces_pairs, K 1.1694, error 0.085446 -> 0.085198, held back 0.30% lower.
 *
 * Rebalanced in 1.4, every term fitted together but material, the king's table and kingDanger (--tune psqt_pawn,
 * psqt_knight, psqt_bishop, psqt_rook, psqt_queen, mobility, pawns_passed, pawns_structure, pawns_passed_extras,
 * king_shelter, pieces_pairs --min-usage 50000 on E12.52-1M-D12-Resolved, K 1.1889, 1000 passes at rate 1, error
 * 0.085192 -> 0.084398, held back 0.87% lower). Each tuned piece square table kept centred on its own average, so
 * the pieces keep their values and only the shapes moved; the 120 values fewer than 50k positions use kept as they
 * were.
 *
 * Then fine-tuned on Elephant's own games, the same groups, kept values and centring as the rebalance (--min-usage
 * 6000 on elephant-positions-0.1, 1.2M positions from nyayo, K 1.2633): its held back error 1.23% lower, but
 * E12.52's 1.13% higher (0.084365 -> 0.085321, K 1.2302 -> 1.1608), the two datasets disagree.
 *
 * Then refitted on Elephant's games since 1.4, the same groups, kept values and centring (--min-usage 7250 on
 * positions-3, 1.45M positions from nyayo's games since 2026-10-30, K 1.3545): its held back error 0.57% lower.
 * Unlike the first own-games fit it agrees with E12.52, whose error is 0.26% lower (0.085307 -> 0.085085), while
 * elephant-positions-0.1's is 0.74% higher, the older games those tables were fitted on.    */

#pragma once
#include <system/platform.hpp>
#include <eval/tapered_score.hpp>

namespace evaluator_data
{

// a side with two or more knights, bishops on both colours, two or more rooks; knight, bishop & rook.
constexpr TaperedScore piecePair[3] = {
    S(   2,  18), S(   2,  52), S(  18,   5),
};

} // namespace evaluator_data
