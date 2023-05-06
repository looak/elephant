#pragma once
#include "defines.h"

class Chessboard;
struct Move;

class Evaluator
{
public:
    Evaluator();
    i32 Evaluate(const Chessboard& board, i32 perspective);

private:
    i32 EvaluateMaterial(const Chessboard& board) const;
    i32 EvaluateMove(Move move) const;
    i32 EvalutePiecePositions(const Chessboard& board) const;
    i32 EvaluatePawnStructure(const Chessboard& board);

    struct PawnStructureHashEntry
    {
        i32 score;
        bool passedPawn[8];
    };

    std::unordered_map<u64, PawnStructureHashEntry> m_pawnStructureTable; 
};