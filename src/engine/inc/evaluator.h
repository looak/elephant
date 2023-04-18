#pragma once
#include "defines.h"

class Chessboard;

class Evaluator
{
public:
    u64 Evaluate(const Chessboard& board) const;
};