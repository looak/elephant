#include "move.h"


Move::Move(const Notation& source, const Notation& target) :
    TargetSquare(target),
    SourceSquare(source),
    Piece(ChessPiece()),
    Promote(ChessPiece()),
    Flags(MoveFlag::Zero),
    PrevMove(nullptr),
    NextMoveCount(0),
    NextMove(nullptr)
{
}

Move::Move(const Move& other)
{
    *this = other;
}

Move& Move::operator=(const Move& other)
{   
    TargetSquare = Notation(other.TargetSquare);
    SourceSquare = Notation(other.SourceSquare);
    Piece = other.Piece;
    Promote = other.Promote;
    Flags = other.Flags;
    PrevMove = other.PrevMove;
    NextMoveCount = other.NextMoveCount;
    NextMove = other.NextMove;

    return *this;
}