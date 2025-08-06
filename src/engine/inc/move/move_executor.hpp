#pragma once

#include <position/position_accessors.hpp>
#include <move/move.hpp>

class MoveExecutor {
public:
    MoveExecutor(PositionProxy<PositionEditPolicy> position);

    MoveUndoUnit makeMove()

private:
    PositionProxy<PositionEditPolicy> m_position;
}