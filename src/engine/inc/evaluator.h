#pragma once
#include "defines.h"

#include <unordered_map>

class Chessboard;
struct Move;

class Evaluator
{
public:
    Evaluator();
    i32 Evaluate(const Chessboard& board);

private:
    i32 EvaluateMaterial(const Chessboard& board) const;
    i32 EvaluateMove(Move move) const;
    i32 EvalutePiecePositions(const Chessboard& board) const;
    i32 EvaluatePawnStructure(const Chessboard& board);

    template<typename Comparator>
    bool EvaluatePassedPawn(const Chessboard& board, u32 potentialPassedPawn, u64 opponentsPawns) const;

    struct PawnStructureHashEntry
    {
        i32 score;
        bool passedPawn[8];
    };

    std::unordered_map<u64, PawnStructureHashEntry> m_pawnStructureTable; 
};