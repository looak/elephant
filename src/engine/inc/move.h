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
#ifndef MOVE_HEADER
#define MOVE_HEADER

#include <vector>
#include "chess_piece.h"
#include "defines.h"
#include "notation.h"

enum MoveFlag : byte {
    Zero = 0,
    Capture = 1,
    Promotion = 2,
    Castle = 4,
    Check = 8,
    EnPassant = 16,
    Checkmate = 32,
    Ambiguous = 64,  // Used for disambiguation of moves
    Invalid = 128
};

inline MoveFlag
operator|(MoveFlag a, MoveFlag b)
{
    return static_cast<MoveFlag>(static_cast<byte>(a) | static_cast<byte>(b));
}

inline MoveFlag
operator&(MoveFlag a, MoveFlag b)
{
    return static_cast<MoveFlag>(static_cast<byte>(a) & static_cast<byte>(b));
}

inline MoveFlag&
operator|=(MoveFlag& a, MoveFlag b)
{
    a = a | b;
    return a;
}

inline MoveFlag&
operator&=(MoveFlag& a, MoveFlag b)
{
    a = a & b;
    return a;
}

inline MoveFlag&
operator^=(MoveFlag& a, MoveFlag b)
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
 * with typeId - 2, i.e. knight 0, bishop 1, rook 2 and queen 3
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

constexpr int c_sourceSquareConstant = 0x3F;
constexpr int c_targetSquareConstant = 0xfc0;

enum PackedMoveType {
    QUIET_MOVES = 0,
    DBL_PAWN_PUSH = 1,
    CASTLE = 2,
    KING_CASTLE = 2,
    QUEEN_CASTLE = 3,
    CAPTURES = 4,
    EN_PASSANT_CAPTURE = 5,
    PROMOTIONS = 8,
    KNIGHT_PROMOTION = 8,
    BISHOP_PROMOTION = 9,
    ROOK_PROMOTION = 10,
    QUEEN_PROMOTION = 11,
    KNIGHT_PROMO_CAPTURE = 12,
    BISHOP_PROMO_CAPTURE = 13,
    ROOK_PROMO_CAPTURE = 14,
    QUEEN_PROMO_CAPTURE = 15
};

struct PackedMove {
public:
    static PackedMove NullMove() { return PackedMove{0x0}; };

    PackedMove() :
        m_internals(0)
    {
    }
    PackedMove(u16 packed) :
        m_internals(packed)
    {
    }
    PackedMove(Square source, Square target) :
        m_internals(0)
    {
        setSource(source);
        setTarget(target);
    }

    [[nodiscard]] constexpr Square sourceSqr() const { return static_cast<Square>(source()); }
    [[nodiscard]] constexpr Square targetSqr() const { return static_cast<Square>(target()); }
    [[nodiscard]] constexpr i32 source() const { return m_internals & c_sourceSquareConstant; }
    [[nodiscard]] constexpr i32 target() const { return (m_internals >> 6) & c_sourceSquareConstant; }
    [[nodiscard]] constexpr u16 flags() const { return m_internals >> 12; }

    [[nodiscard]] constexpr bool isNull() const { return m_internals == 0; }
    [[nodiscard]] constexpr bool isQuiet() const { return flags() == 0; }
    [[nodiscard]] constexpr bool isCapture() const { return !!(flags() & CAPTURES); }
    [[nodiscard]] constexpr bool isEnPassant() const
    {
        return isPromotion() ? false : !!((flags() & EN_PASSANT_CAPTURE) == EN_PASSANT_CAPTURE);
    }
    [[nodiscard]] constexpr bool isPromotion() const { return !!(flags() & PROMOTIONS); }
    [[nodiscard]] constexpr bool isCastling() const
    {
        u16 flag = m_internals >> 12;
        if (flag & PROMOTIONS)
            return false;
        return (flag & CASTLE);
    }

    [[nodiscard]] constexpr i32 readPromoteToPieceType() const { return ((m_internals >> 12) & 3) + 2; }

    void set(u16 packed) { m_internals = packed; }
    [[nodiscard]] constexpr u16 read() const { return m_internals; }

    inline void setSource(u16 source)
    {
        m_internals &= ~c_sourceSquareConstant;
        m_internals |= (source & c_sourceSquareConstant);
    }
    inline void setSource(Square sqr)
    {
        u16 u16sqr = static_cast<u16>(sqr);
        setSource(u16sqr);
    }

    inline void setTarget(u16 target)
    {
        m_internals &= ~c_targetSquareConstant;
        m_internals |= ((target & c_sourceSquareConstant) << 6);
    }
    inline void setTarget(Square sqr)
    {
        u16 u16sqr = static_cast<u16>(sqr);
        setTarget(u16sqr);
    }
    inline void setCapture(bool value)
    {
        if (value == true)
            m_internals |= CAPTURES << 12;
        else
            m_internals &= ~(CAPTURES << 12);
    }
    inline void setEnPassant(bool value)
    {
        if (value == true)
            m_internals |= EN_PASSANT_CAPTURE << 12;
        else
            m_internals &= ~(EN_PASSANT_CAPTURE << 12);
    }

    inline void setPromoteTo(ChessPiece piece) { setPromoteTo(piece.index()); }
    inline void setPromoteTo(u16 pieceIndx)
    {
        m_internals &= ~(11 << 12);              // clear promotion bits
        m_internals |= (8 << 12);                // set promotion flag
        m_internals |= ((pieceIndx - 1) << 12);  // store piece type
    }

    inline void setCastleQueenSide(bool value)
    {
        if (value == true)
            m_internals |= QUEEN_CASTLE << 12;
        else
            m_internals &= ~(QUEEN_CASTLE << 12);
    }

    inline void setCastleKingSide(bool value)
    {
        if (value == true)
            m_internals |= KING_CASTLE << 12;
        else
            m_internals &= ~(KING_CASTLE << 12);
    }

    // operators
    bool operator==(const PackedMove& rhs) const { return m_internals == rhs.m_internals; }
    bool operator!=(const PackedMove& rhs) const { return m_internals != rhs.m_internals; }
    operator bool() const { return m_internals != 0; }

    [[nodiscard]] std::string toString() const
    {
        std::string ret;
        ret += Notation(sourceSqr()).toString();
        ret += Notation(targetSqr()).toString();
        return ret;
    }

private:

#ifdef __clang__
    #define PUSH_DIAGNOSTIC _Pragma("clang diagnostic push")
    #define POP_DIAGNOSTIC  _Pragma("clang diagnostic pop")
    #define IGNORE_WARNING(warning) _Pragma("clang diagnostic ignored " #warning)
#elif defined(__GNUC__) || defined(__GNUG__)
    #define PUSH_DIAGNOSTIC _Pragma("GCC diagnostic push")
    #define POP_DIAGNOSTIC  _Pragma("GCC diagnostic pop")
    #define IGNORE_WARNING(warning) _Pragma("GCC diagnostic ignored " #warning)
#else
    #define PUSH_DIAGNOSTIC
    #define POP_DIAGNOSTIC
    #define IGNORE_WARNING(warning)
#endif

PUSH_DIAGNOSTIC
    union {
        u16 m_internals;
        struct {
            // public:
            //     internals_struct() = default;
            //     internals_struct(u16 data) { memcpy(this, &data, sizeof(u16)); }
            u16 src : 6;
            u16 trg : 6;
            u16 flag : 1;
            u16 castle : 1;
            u16 capture : 1;
            u16 promote : 1;
        } m_internals_struct;
    };
POP_DIAGNOSTIC
};



static_assert(sizeof(PackedMove) == 2, "PackedMove is not 2 bytes");

struct PrioratizedMove {
    PrioratizedMove() :
        move(0),
        priority(0),
        check(0){};

    PrioratizedMove(PackedMove move, int _priority) :
        move(move),
        priority(_priority),
        check(0){};

    void setCheck(bool value) { check = value ? 1 : 0; }
    bool isCheck() const { return check == 1; }

    PackedMove move;
    u16 priority : 15;
    u16 check : 1;
};
static_assert(sizeof(PrioratizedMove) == 4, "PrioratizedMove is not 4 bytes");

struct PrioratizedMoveComparator {
    constexpr bool operator()(const PrioratizedMove& lhs, const PrioratizedMove& rhs) const
    {
        return lhs.priority < rhs.priority;
    }
};

struct Move {
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

    i32 Score;

    Move* PrevMove;
    unsigned short NextMoveCount;
    Move* NextMove;
};

inline bool
operator==(const Move& lhs, const Move& rhs)
{
    return lhs.SourceSquare == rhs.SourceSquare && lhs.TargetSquare == rhs.TargetSquare &&
           lhs.PromoteToPiece == rhs.PromoteToPiece;
}

struct MoveResult {
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

#endif  // MOVE_HEADER