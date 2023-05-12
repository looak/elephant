#include "defines.h"

struct TaperedScore
{
    i32 midgame;
    i32 endgame;    
};

i32 operator*(const TaperedScore& lhs, const float& rhs)
{
    return (i32)(lhs.midgame + (lhs.endgame - lhs.midgame) * rhs);
}

namespace evaluator_data
{

#define TS(x,y) TaperedScore{x,y}

/* piece/sq tables */
const i32 pawnPositionTable[64] = {
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   5,  10,  10,   5,   0,   0,
     0,   0,  10,  10,  10,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,   5,  10,  10,   5,   0,   0,
    10,  10,  10,  20,  20,  10,  10,  10,
    20,	 20,  20,  30,  30,  20,  20,  20,
     0,   0,   5,  10,  10,   5,   0,   0
};

constexpr TaperedScore pawnPositionTaperedScoreTable[64] = {
    TS(0, 0),  TS(0, 0),  TS(0, 0),  TS(0, 0),   TS(0, 0),   TS(0, 0),   TS(0, 0),   TS(0, 0),
    TS(0, 0),  TS(0, 0),  TS(5, 0),  TS(10, 0),  TS(10, 0),  TS(5, 0),   TS(0, 0),   TS(0, 0),
    TS(0, 0),  TS(0, 0),  TS(10, 0), TS(10, 0),  TS(10, 0),  TS(10, 0),  TS(0, 0),   TS(0, 0),
    TS(0, 0),  TS(0, 0),  TS(10, 0), TS(20, 0),  TS(20, 0),  TS(10, 0),  TS(0, 0),   TS(0, 0),
    TS(0, 0),  TS(0, 0),  TS(5, 0),  TS(10, 0),  TS(10, 0),  TS(5, 0),   TS(0, 0),   TS(0, 0),
    TS(10, 0), TS(10, 0), TS(10, 0), TS(20, 0),  TS(20, 0),  TS(10, 0),  TS(10, 0),  TS(10, 0),
    TS(20, 0), TS(20, 0), TS(20, 0), TS(30, 0),  TS(30, 0),  TS(20, 0),  TS(20, 0),  TS(20, 0),
    TS(0, 0),  TS(0, 0),  TS(5, 0),  TS(10, 0),  TS(10, 0),  TS(5, 0),   TS(0, 0),   TS(0, 0)
};

constexpr i32 knightPositionTable[64] = {
   -50, -10, -20, -20, -20, -20, -10, -50,
   -40, -20,   0,   5,   5,   0, -20, -40,
   -30,   5,  10,  15,  15,  10,   5, -30,
   -30,   0,  15,  20,  20,  15,   0, -30,
   -30,   5,  15,  20,  20,  15,   5, -30,
   -30,   0,  10,  15,  15,  10,   0, -30,
   -40, -20,   0,   0,   0,   0, -20, -40,
   -50, -40, -20, -20, -20, -20, -40, -50
};

constexpr i32 bishopPositionTable[64] = {
   -20, -10, -10, -10, -10, -10, -10, -20,
   -10,   5,   0,   0,   0,   0,   5, -10,
   -10,  10,  10,  10,  10,  10,  10, -10,
   -10,   0,  10,  20,  20,  10,   0, -10,
   -10,   5,   5,  20,  20,   5,   5, -10,
   -10,   0,   5,  10,  10,   5,   0, -10,
   -10,   5,   0,   0,   0,   0,   5, -10,
   -20, -10, -10, -10, -10, -10, -10, -20
};

constexpr i32 rookPositionTable[64] = {
     0,   0,   5,   5,   5,   5,   0,   0,
    -5,   0,   5,  10,  10,   5,   0,  -5,
    -5,   0,   5,  10,  10,   5,   0,  -5,
    -5,   0,   5,  10,  10,   5,   0,  -5,
    -5,   0,   5,  10,  10,   5,   0,  -5,
    -5,   0,   5,  10,  10,   5,   0,  -5,
    20,  25,  25,  35,  35,  25,  25,  20,
     0,   0,   0,  10,  10,   0,   0,   0
};

constexpr i32 queenPositionTable[64] = {
   -20, -10, -10, -5, -5, -10, -10, -20,
   -10,   0,   0,  0,  0,  0,  0, -10,
   -10,   0,   5,  5,  5,  5,  0, -10,
    -5,   0,   5,  5,  5,  5,  0,  -5,
     0,   0,   5,  5,  5,  5,  0,  -5,
   -10,   5,   5,  5,  5,  5,  0, -10,
   -10,   0,   5,  0,  0,  0,  0, -10
};

constexpr i32 kingPositionTable[64] = {
     20,  30,  10,   0,   0,  10,  30,  20,
     20,  20,   0,   0,   0,   0,  20,  20,
    -10, -20, -20, -20, -20, -20, -20, -10,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -10, -20, -20, -20, -20, -20, -20, -10
};

constexpr i32 mirrored[64] = {
    56,  57,  58,  59,  60,  61,  62,  63,
    48,  49,  50,  51,  52,  53,  54,  55,
    40,  41,  42,  43,  44,  45,  46,  47,
    32,  33,  34,  35,  36,  37,  38,  39,
    24,  25,  26,  27,  28,  29,  30,  31,
    16,  17,  18,  19,  20,  21,  22,  23,
     8,   9,  10,  11,  12,  13,  14,  15,
     0,   1,   2,   3,   4,   5,   6,   7
};

const i32* pestoTables[6] =
{
    pawnPositionTable,
    knightPositionTable,
    bishopPositionTable,
    rookPositionTable,
    queenPositionTable,
    kingPositionTable
};

/**
 * Idea here is that doubling pawns early or midgame will hurt your structure,
 * later in the game it's not as important where there are less pawns on the board.  */
static constexpr TaperedScore doubledPawnScore{-50, -25};

/**
 * Idea here is that isolated pawns are bad but will be worse in the endgame.  */
static constexpr TaperedScore isolatedPawnScore{-25, -50};

/**
 * Passed pawns are a strong factor in the endgame and something to strive for. */
static constexpr TaperedScore passedPawnScore{50, 200};

} // namespace evaluator_data