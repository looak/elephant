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
 * @brief Piece values, see pieces.hpp. Written by ElephantTuner, laid out the way it writes them. All constexpr, safe
 * to include anywhere.
 *
 * Tuned by ElephantTuner, every other value kept, on E12.52-1M-D12-Resolved (10M positions, 1000 passes at rate 1).
 * Pairs: --tune pieces_pairs, K 1.1694, error 0.085446 -> 0.085198, held back 0.30% lower. Files: then --tune
 * pieces_files, K 1.1889, error 0.085192 -> 0.085075, held back 0.16% lower.    */

#pragma once
#include <system/platform.hpp>
#include <eval/tapered_score.hpp>

namespace evaluator_data
{

// a side with two or more knights, bishops on both colours, two or more rooks; knight, bishop & rook.
constexpr TaperedScore piecePair[3] = {
    S(   2,  15), S(   0,  52), S(  -1,  27),
};

// a rook or queen on a file without its own pawns: half-open (the enemy's pawns there), open (none at all).
constexpr TaperedScore rookOnFile[2] = {
    S(   6,   1), S(  15,   6),
};
constexpr TaperedScore queenOnFile[2] = {
    S(   4,   1), S(   3,   7),
};

} // namespace evaluator_data
