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
 * @file position_hash.hpp
 * @brief The Zobrist hash of a position computed from scratch, apart from hash_zobrist.hpp which the position
 * headers include themselves. */

#pragma once
#include <position/position_accessors.hpp>

namespace zobrist {

// The hash of position computed from its pieces, castling rights, en passant square and whiteToMove. The search knows
// whose turn it is from its template parameter, a null move flips the hash but not the board's side to move, so the
// search passes the side it is searching for.
u64 computePositionHash(PositionReader position, bool whiteToMove);

} // namespace zobrist
