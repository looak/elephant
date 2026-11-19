// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021-2026  Alexander Loodin Ek

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
 * @file polyglot.hpp
 * @brief A position's Polyglot hash, the standard one python-chess's chess.polyglot.zobrist_hash computes, which is how
 * nyayo finds games by position: the pieces, the side to move, the castling rights a king & rook on their squares still
 * have, and the en passant file only when a pawn of the side to move stands next to the pawn that double pushed
 * (legality ignored). Not the move counters.
 * @author Alexander Loodin Ek  */
#pragma once
#include <string>

#include <system/platform.hpp>

class Chessboard;

namespace indexer {

u64 polyglotHash(const Chessboard& board);

// 16 lowercase hex digits, as the cards write a hash.
std::string toHex(u64 hash);

} // namespace indexer
