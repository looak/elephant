#pragma once
#include "defines.hpp"
#include "chess_piece_defines.hpp"

#include <unordered_map>

class Chessboard;
class MoveGenerator;
struct Move;
struct PackedMove;

class Evaluator
{
public:
    Evaluator();
    i32 Evaluate(const Chessboard& board, const MoveGenerator& movegen);
    i32 EvaluatePlus(const Chessboard& board, const MoveGenerator& movegen, PackedMove move);

private:
    i32 EvaluateMaterial(const Chessboard& board) const;
    i32 EvalutePiecePositions(const Chessboard& board) const;
    i32 EvaluatePawnStructure(const Chessboard& board) const;
    i32 EvaluatePawnManhattanDistance(const Chessboard& board) const;
    i32 EvaluateKingSafety(const Chessboard& board, const MoveGenerator& movegen) const;

    i32 MopUpValue(const Chessboard& board, i32 materialScore) const;

    template<Set us>
    i32 EvaluatePassedPawn(const Chessboard& board) const;

    template<Set us>
    i32 MopUpValue(const Chessboard& board, i32 materialScore) const;

};