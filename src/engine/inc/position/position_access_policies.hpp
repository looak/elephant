// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021-2023  Alexander Loodin Ek

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
 * @file position_access_policies.hpp
 * @brief header only structs for defining access policies for the position proxy. Put in it's own header to break some circular
 * dependencies between poisition_proxy, position_accessors and this. Which disallowed implementation of implicit or explicit 
 * conversions from edit policy proxy to read only proxy.
 *
 * @author Alexander Loodin Ek
 */

#pragma once
#include <system/platform.hpp>

class Position;
struct CastlingStateInfo;
class CastlingStateProxy;
struct EnPassantStateInfo;
class EnPassantStateProxy;
struct MaterialPositionMask;
struct ChessPiece;

struct PositionEditPolicy {
    using position_t = Position&;
    using castling_t = CastlingStateProxy;
    using en_passant_t = EnPassantStateProxy;
    using material_t = MaterialPositionMask&;
    using hash_t = u64&;
    
    //  currently we don't have a mutable chess piece type on the position proxy.
    // TODO: implement a way to modify the chess piece on a poistion proxy, would only allow to set or unset a piece
    // since the material is stored in two bitboards, one for the set and one for the piece type.
    using chess_piece_t = const ChessPiece;
};

struct PositionReadOnlyPolicy {
    using position_t = const Position&;
    using castling_t = const CastlingStateInfo;
    using en_passant_t = const EnPassantStateInfo;
    using material_t = const MaterialPositionMask&;
    using hash_t = const u64;

    using chess_piece_t = const ChessPiece;
};