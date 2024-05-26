#pragma once
#include "defines.h"

#include <unordered_map>

class Chessboard;
class MoveGenerator;
struct Move;

class Evaluator
{
public:
    Evaluator();
    i32 Evaluate(const Chessboard& board, const MoveGenerator& movegen);

private:
    i32 EvaluateMaterial(const Chessboard& board) const;
    i32 EvaluateMove(Move move) const;
    i32 EvalutePiecePositions(const Chessboard& board) const;
    i32 EvaluatePawnStructure(const Chessboard& board);
    i32 EvaluateKingSafety(const Chessboard& board, const MoveGenerator& movegen) const;

    // template<Set us>
    // i32 EvaluatePassedPawn(const Chessboard& board);

    struct PawnStructureHashEntry
    {
        i32 score;
        bool passedPawn[8];
    };

    std::unordered_map<u64, PawnStructureHashEntry> m_pawnStructureTable; 
};