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

#pragma once
#include "defines.h"

class Chessboard;
struct ChessPiece;
struct Notation;

struct ZorbistHash
{ 
    static const ZorbistHash& Instance();

    u64 HashBoard(const Chessboard& board) const;
    u64 HashPiecePlacement(const u64& oldHash, ChessPiece piece, Notation position) const;
    u64 HashEnPassant(const u64& oldHash, Notation position) const;
    u64 HashCastling(const u64& oldHash, const u8 castlingState) const;
    u64 HashBlackToMove(const u64& oldHash) const;
    
private:

    void GenerateZorbistTable();
    ZorbistHash()
        : initialized(false)
    {
        GenerateZorbistTable();
    }
    static ZorbistHash instance;

    bool initialized;
    u64 table[64][12];
    u64 black_to_move;
    u64 castling[4];
    u64 enpassant[8];
};