#include "move.h"


Move::Move(const Notation& source, const Notation& target) :
    TargetSquare(target),
    SourceSquare(source),
    Flags(MoveFlag::Zero),
    PrevMove(nullptr),
    NextMoveCount(0),
    NextMove(nullptr)
{
}