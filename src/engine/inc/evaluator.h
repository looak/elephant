#pragma once
#include "defines.hpp"
#include <position/position_accessors.hpp>

#include <unordered_map>

class Chessboard;
class MoveGenerator;
struct PackedMove;
class Position;

class Evaluator
{
public:
    Evaluator(PositionReader position) : m_position(position) {}    

    i32 Evaluate(const MoveGenerator& movegen);
    i32 EvaluatePlus(const MoveGenerator& movegen, PackedMove move);

    /**
     * @brief Calculates the end game coeficient.
     * Used for tapered evaluation.
     * @return a float between 0 and 1 where 1 is endgame and 0 is midgame.     */    
    float calculateEndGameCoeficient() const;

private:
    i32 EvaluateMaterial() const;
    i32 EvalutePiecePositions() const;
    i32 EvaluatePawnStructure() const;
    i32 EvaluatePawnManhattanDistance() const;
    i32 EvaluateKingSafety(const MoveGenerator& movegen) const;

    i32 MopUpValue(i32 materialScore) const;

    template<Set us>
    i32 EvaluatePassedPawn() const;
    template<Set us>
    i32 EvaluatePawnProtection( Bitboard pawns) const;

    template<Set us>
    i32 MopUpValue(i32 materialScore) const;


    PositionProxy<PositionReadOnlyPolicy> m_position;

};