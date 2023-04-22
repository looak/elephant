#pragma once
#include "defines.h"

class Chessboard;
struct Move;

class Evaluator
{
public:
    Evaluator();
    i32 Evaluate(const Chessboard& board, Move prevMove) const;

private:
    i32 EvaluateMaterial(const Chessboard& board) const;
    i32 EvaluateMove(Move move) const;
};