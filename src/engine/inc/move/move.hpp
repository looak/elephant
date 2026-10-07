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

#include <system/platform.hpp>
#include <core/square_notation.hpp>
#include <material/chess_piece.hpp>
#include <math/cast.hpp>
#include <position/en_passant_state_info.hpp>
#include <position/castling_state_info.hpp>

enum class MoveTypes {
    ALL,
    CAPTURES_ONLY
};

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

constexpr u16 operator~(PackedMoveType type) {
    return static_cast<u16>(~static_cast<u16>(type));
}

constexpr PackedMoveType operator<<(PackedMoveType type, int shift) {
    return static_cast<PackedMoveType>(static_cast<u16>(type) << shift);
}

struct PackedMove {
public:
    static PackedMove NullMove() { return PackedMove{ 0x0 }; };

    PackedMove() = default;

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

    PackedMove(const PackedMove&) = default;
    PackedMove& operator=(const PackedMove&) = default;
    ~PackedMove() = default;

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

    inline void setSource(u16 source) {
        ASSERT(source < 64);
        int internals = m_internals;
        internals &= ~c_sourceSquareConstant;
        internals |= (source & c_sourceSquareConstant);
        m_internals = checked_cast<u16>(internals);
    }

    inline void setSource(Square sqr)
    {
        u16 u16sqr = to_index<u16>(sqr);
        setSource(u16sqr);
    }

    inline void setTarget(u16 target) {
        ASSERT(target < 64);
        int internals = m_internals;
        internals &= ~c_targetSquareConstant;
        internals |= ((target & c_sourceSquareConstant) << 6);
        m_internals = checked_cast<u16>(internals);
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
        m_internals &= (u16)~(11 << 12);         // clear promotion bits
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
        THROW_EXPR(!isNull(), ephant::io_error, "Cannot convert NullMove to string.");
        std::string ret;
        ret += SquareNotation(sourceSqr()).toString();
        ret += SquareNotation(targetSqr()).toString();
        if (isPromotion())
            ret += ChessPiece(Set::BLACK, static_cast<PieceType>(readPromoteToPieceType())).toString();
        return ret;
    }

private:
    u16 m_internals;
};

static_assert(sizeof(PackedMove) == 2, "PackedMove is not 2 bytes");
static_assert(std::is_trivial_v<PackedMove>, "PackedMove is not trivial");

// TODO: this should just be an implementation detail for move generator and should not 
// be returned outside of that context.
struct PrioritizedMove {
    PrioritizedMove() :
        move(0),
        priority(0),
        check(0) {};

    PrioritizedMove(PackedMove _move, u16 _priority) :
        move(_move),
        priority(_priority),
        check(0) {};

    void setCheck(bool value) { check = value ? 1 : 0; }
    bool isCheck() const { return check == 1; }

    constexpr operator bool() const { return move.isNull() != true; }

    PackedMove move;
    u16 priority : 15;
    bool check : 1;
};
// static_assert(sizeof(PrioritizedMove) == 4, "PrioritizedMove is not 4 bytes");

struct PrioritizedMoveComparator {
    constexpr bool operator()(const PrioritizedMove& lhs, const PrioritizedMove& rhs) const
    {
        return lhs.priority > rhs.priority;
    }
};

struct Move {
public:
    Move();
    Move(Square source, Square target);
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
    void setEnPassant(bool value) { Flags = (MoveFlag)(value ? Flags | MoveFlag::EnPassant : Flags & ~MoveFlag::EnPassant); }
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

    Square TargetSquare;
    Square SourceSquare;

    Square EnPassantTargetSquare;
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

struct MoveUndoUnit {
    MoveUndoUnit() :
        move(),
        movedPiece(),
        capturedPiece(),
        castlingState(),
        enPassantState(),
        hash(0),
        plyCount(0)
    {
    }
    PackedMove move;
    ChessPiece movedPiece;
    ChessPiece capturedPiece;
    CastlingStateInfo castlingState;
    EnPassantStateInfo enPassantState;
    u64 hash;
    u16 plyCount;
};

#endif  // MOVE_HEADER