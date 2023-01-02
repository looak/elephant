// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021  Alexander Loodin Ek

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
#include "chess_piece.h"
#include "notation.h"
#include <vector>

enum MoveFlag : byte
{
    Zero = 0,
    Capture = 1,
    Promotion = 2,
    Castle = 4,
    Check = 8,
    EnPassant = 16,
    Legal = 32,
    Checkmate = 64,
	Ambiguous = 128, // Used for disambiguation of moves
    Invalid = 0xff
};

inline MoveFlag operator|(MoveFlag a, MoveFlag b)
{
    return static_cast<MoveFlag>(static_cast<byte>(a) | static_cast<byte>(b));
}

inline MoveFlag operator&(MoveFlag a, MoveFlag b)
{
    return static_cast<MoveFlag>(static_cast<byte>(a) & static_cast<byte>(b));
}

inline MoveFlag& operator|=(MoveFlag& a, MoveFlag b) 
{ 
    a = a | b;
    return a;
}

struct Move
{
public:
    Move();
    Move(const Notation& source, const Notation& target);
    Move(const Move& other);
	Move(Move&& other);
    
    bool isCapture() const { return MoveFlag::Capture == (Flags & MoveFlag::Capture); }
    bool isCastling() const { return MoveFlag::Castle == (Flags & MoveFlag::Castle); }
	bool isPromotion() const { return MoveFlag::Promotion == (Flags & MoveFlag::Promotion); }
	bool isCheck() const { return MoveFlag::Check == (Flags & MoveFlag::Check); }
	bool isEnPassant() const { return MoveFlag::EnPassant == (Flags & MoveFlag::EnPassant); }
	bool isAmbiguous() const { return MoveFlag::Ambiguous == (Flags & MoveFlag::Ambiguous); }

	void setPromotion(bool value) { Flags = (MoveFlag)(value ? Flags | MoveFlag::Promotion : Flags & ~MoveFlag::Promotion); }
	void setCapture(bool value) { Flags = (MoveFlag)(value ? Flags | MoveFlag::Capture : Flags & ~MoveFlag::Capture); }
	void setAmbiguous(bool value) { Flags = (MoveFlag)(value ? Flags | MoveFlag::Ambiguous : Flags & ~MoveFlag::Ambiguous); }
        
    Move& operator=(const Move& other);

    static std::vector<std::string> ParsePNG(std::string png, std::vector<Move>& ret);
    static Move FromString(std::string moveStr, bool isWhiteMove);

    Notation TargetSquare;
    Notation SourceSquare;

    Notation EnPassantTargetSquare;
    byte PrevCastlingState;

    ChessPiece Piece;
    ChessPiece PromoteToPiece;
    ChessPiece CapturedPiece;

    MoveFlag Flags;

    Move* PrevMove;
    unsigned short NextMoveCount;
    Move* NextMove;
};

