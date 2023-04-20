#pragma once
#include "defines.h"

class Chessboard;

class Evaluator
{
public:
    i32 Evaluate(const Chessboard& board) const;

private:
    i32 EvaluateMaterial(const Chessboard& board) const;
};