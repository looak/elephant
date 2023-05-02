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
    Checkmate = 32,
	Ambiguous = 64, // Used for disambiguation of moves
    Invalid = 128
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

inline MoveFlag& operator&=(MoveFlag& a, MoveFlag b)
{
	a = a & b;
	return a;
}

inline MoveFlag& operator^=(MoveFlag& a, MoveFlag b)
{
	a = static_cast<MoveFlag>(static_cast<byte>(a) ^ static_cast<byte>(b));
	return a;
}

/**
 * Packed move
 * as described on chessprogramming.org
 * [Source Square x6][Target Square x6][Flags x4]
 * Source & Target squares represented 0-63
 * bit 0 is for promotions
 * bit 1 is for captures
 * bit 2 & 3 are special cases
 * during promotions bit 2 & 3 represent the promotion type
 * with typeIndex - 2, i.e. knight 0, bishop 1, rook 2 and queen 3
 * 
 * Move Type Encoding:
 * -------------------
 *         bit 16 bit 15 bit 14 bit 13
 * value   bit 3  bit 2  bit 1  bit 0  Description
 * ------------------------------------------------
 *    0      0      0      0      0      Quiet moves
 *    1      0      0      0      1      Double pawn push
 *    2      0      0      1      0      King castle
 *    3      0      0      1      1      Queen castle
 *    4      0      1      0      0      Captures
 *    5      0      1      0      1      En passant capture
 *    8      1      0      0      0      Knight-promotion
 *    9      1      0      0      1      Bishop-promotion
 *    10     1      0      1      0      Rook-promotion
 *    11     1      0      1      1      Queen-promotion
 *    12     1      1      0      0      Knight-promo capture
 *    13     1      1      0      1      Bishop-promo capture
 *    14     1      1      1      0      Rook-promo capture
 *    15     1      1      1      1      Queen-promo capture
 * ------------------------------------------------       */

const int c_sourceSquareConstant = 0x3F;
const int c_targetSquareConstant = 0xfc0;

enum PackedMoveType {
    QUIET_MOVES          = 0,
    DOUBLE_PAWN_PUSH     = 1,
    CASTLE               = 2,   
    KING_CASTLE          = 2,
    QUEEN_CASTLE         = 3,
    CAPTURES             = 4,
    EN_PASSANT_CAPTURE   = 5,
    PROMOTIONS            = 8,
    KNIGHT_PROMOTION     = 8,
    BISHOP_PROMOTION     = 9,
    ROOK_PROMOTION       = 10,
    QUEEN_PROMOTION      = 11,
    KNIGHT_PROMO_CAPTURE = 12,
    BISHOP_PROMO_CAPTURE = 13,
    ROOK_PROMO_CAPTURE   = 14,
    QUEEN_PROMO_CAPTURE  = 15
};

struct PackedMove
{
public:
    int sourceSqr() { return m_internals & 0x3F; }
    int targetSqr() { return (m_internals >> 6) & 0x3F; }
    bool isQuiet() { return (m_internals >> 12) == 0; }
    bool isCapture() {return ((m_internals >> 12) & CAPTURES) == CAPTURES; }
    bool isPromotion() {return ((m_internals >> 12) & PROMOTIONS) == PROMOTIONS; }
    bool isCastling() {
        u16 flag = m_internals >> 12;
        if (flag & PROMOTIONS)
            return false;
        return (flag & CASTLE);
        }
    bool isPawnDoublePush() { return ((m_internals >> 12) & 0xF) == 1; }
    
    int readPromoteToPieceType() { return ((m_internals >> 12) & 3)+2; }

    void set(u16 packed) { m_internals = packed; }
    u16 read() const { return m_internals; }

private:
    u16 m_internals;    
};

struct Move
{
public:
    Move();
    Move(Notation source, Notation target);
    Move(const Move& other);
	Move(Move&& other);
    
    bool isCapture() const { return MoveFlag::Capture == (Flags & MoveFlag::Capture); }
    bool isCastling() const { return MoveFlag::Castle == (Flags & MoveFlag::Castle); }
	bool isPromotion() const { return MoveFlag::Promotion == (Flags & MoveFlag::Promotion); }
	bool isCheck() const { return MoveFlag::Check == (Flags & MoveFlag::Check); }
    bool isCheckmate() const { return MoveFlag::Checkmate == (Flags & MoveFlag::Checkmate); }
	bool isEnPassant() const { return MoveFlag::EnPassant == (Flags & MoveFlag::EnPassant); }
	bool isAmbiguous() const { return MoveFlag::Ambiguous == (Flags & MoveFlag::Ambiguous); }
    bool isInvalid() const { return MoveFlag::Invalid == (Flags & MoveFlag::Invalid); }
    i16 calcCaptureValue() const; 

	void setPromotion(bool value) { Flags = (MoveFlag)(value ? Flags | MoveFlag::Promotion : Flags & ~MoveFlag::Promotion); }
	void setCapture(bool value) { Flags = (MoveFlag)(value ? Flags | MoveFlag::Capture : Flags & ~MoveFlag::Capture); }
	void setAmbiguous(bool value) { Flags = (MoveFlag)(value ? Flags | MoveFlag::Ambiguous : Flags & ~MoveFlag::Ambiguous); }
    void setCastling(bool value) { Flags = (MoveFlag)(value ? Flags | MoveFlag::Castle : Flags & ~MoveFlag::Castle); }
    void setInvalid(bool value) { Flags = (MoveFlag)(value ? Flags | MoveFlag::Invalid : Flags & ~MoveFlag::Invalid); }

    PackedMove readPackedMove() const;
        
    Move& operator=(const Move& other);
    std::string toString() const;

    static std::vector<std::string> ParsePGN(std::string pgn, std::vector<Move>& ret);
    static Move fromPGN(std::string pgn, bool isWhiteMove);
    static Move fromString(std::string moveNotation);

    static Move Invalid() { return Move(); }

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

struct MoveResult
{
    Notation SourceSquare;
    Notation TargetSquare;

    byte PrevCastlingState;

    Notation EnPassantTargetSquare;
    ChessPiece Piece;
    ChessPiece PromoteToPiece;
    ChessPiece CapturedPiece;

    MoveFlag Flags;

    Move* PrevMove;
    unsigned short NextMoveCount;
    Move* NextMove;
};

struct
{
    bool operator()(const Move& lhs, const Move& rhs) const
    {
        if (lhs.isCapture() == true && rhs.isCapture() == true)
        {
            return lhs.calcCaptureValue() > rhs.calcCaptureValue();
        }
        else
        if (lhs.isCapture() == true && rhs.isCapture() == false)
        {
            return true;
        }        

        return false;
    }
} s_moveComparer;
