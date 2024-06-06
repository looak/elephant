#include "defines.hpp"

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

i32 flip(const i32 index) {
    return index ^ 56;
}

#define TS(x,y) TaperedScore{x,y}

/* piece/sq tables */
constexpr i32 pawnPositionTable_mg[64] = {
    //   A    B    C    D    E    F    G    H
         0,   0,   0,   0,   0,   0,   0,   0,   // 1
        15,  10,   5,  10,  10,   5,  10,  15,   // 2
        -5,   0,  10,  10,  10,  10,   0,  -5,   // 3
         0,   0,  10,  20,  20,  10,   0,   0,   // 4
         0,   0,   5,  10,  10,   5,   0,   0,   // 5
        10,  10,  10,  20,  20,  10,  10,  10,   // 6
        20,	 20,  20,  30,  30,  20,  20,  20,   // 7
         0,   0,   5,  10,  10,   5,   0,   0    // 8
};

/* piece/sq tables */
constexpr i32 pawnPositionTable_eg[64] = {
    //   A    B    C    D    E    F    G    H
         0,   0,   0,   0,   0,   0,   0,   0,   // 1
         0,   0,  -5, -10, -10,  -5,   0,   0,   // 2
        -5,   0,  10,  10,  10,  10,   0,  -5,   // 3
         0,   0,  10,  20,  20,  10,   0,   0,   // 4
        10,  10,  15,  20,  20,  15,  10,  10,   // 5
       100, 100, 100, 100, 100, 100, 100, 100,   // 6
       160, 160, 160, 160, 160, 160, 160, 160,   // 7
         0,   0,   0,   0,   0,   0,   0,   0    // 8
};

constexpr TaperedScore pawnPositionTaperedScoreTable[64] = {
    TS(pawnPositionTable_mg[0], pawnPositionTable_eg[0]),  TS(pawnPositionTable_mg[1], pawnPositionTable_eg[1]),   TS(pawnPositionTable_mg[2], pawnPositionTable_eg[2]),   TS(pawnPositionTable_mg[3], pawnPositionTable_eg[3]),   TS(pawnPositionTable_mg[4], pawnPositionTable_eg[4]),   TS(pawnPositionTable_mg[5], pawnPositionTable_eg[5]),   TS(pawnPositionTable_mg[6], pawnPositionTable_eg[6]),   TS(pawnPositionTable_mg[7], pawnPositionTable_eg[7]),
    TS(pawnPositionTable_mg[8], pawnPositionTable_eg[8]),  TS(pawnPositionTable_mg[9], pawnPositionTable_eg[9]),   TS(pawnPositionTable_mg[10], pawnPositionTable_eg[10]), TS(pawnPositionTable_mg[11], pawnPositionTable_eg[11]), TS(pawnPositionTable_mg[12], pawnPositionTable_eg[12]), TS(pawnPositionTable_mg[13], pawnPositionTable_eg[13]), TS(pawnPositionTable_mg[14], pawnPositionTable_eg[14]), TS(pawnPositionTable_mg[15], pawnPositionTable_eg[15]),
    TS(pawnPositionTable_mg[16], pawnPositionTable_eg[16]), TS(pawnPositionTable_mg[17], pawnPositionTable_eg[17]),  TS(pawnPositionTable_mg[18], pawnPositionTable_eg[18]), TS(pawnPositionTable_mg[19], pawnPositionTable_eg[19]), TS(pawnPositionTable_mg[20], pawnPositionTable_eg[20]), TS(pawnPositionTable_mg[21], pawnPositionTable_eg[21]), TS(pawnPositionTable_mg[22], pawnPositionTable_eg[22]), TS(pawnPositionTable_mg[23], pawnPositionTable_eg[23]),
    TS(pawnPositionTable_mg[24], pawnPositionTable_eg[24]), TS(pawnPositionTable_mg[25], pawnPositionTable_eg[25]),  TS(pawnPositionTable_mg[26], pawnPositionTable_eg[26]), TS(pawnPositionTable_mg[27], pawnPositionTable_eg[27]), TS(pawnPositionTable_mg[28], pawnPositionTable_eg[28]), TS(pawnPositionTable_mg[29], pawnPositionTable_eg[29]), TS(pawnPositionTable_mg[30], pawnPositionTable_eg[30]), TS(pawnPositionTable_mg[31], pawnPositionTable_eg[31]),
    TS(pawnPositionTable_mg[32], pawnPositionTable_eg[32]), TS(pawnPositionTable_mg[33], pawnPositionTable_eg[33]),  TS(pawnPositionTable_mg[34], pawnPositionTable_eg[34]), TS(pawnPositionTable_mg[35], pawnPositionTable_eg[35]), TS(pawnPositionTable_mg[36], pawnPositionTable_eg[36]), TS(pawnPositionTable_mg[37], pawnPositionTable_eg[37]), TS(pawnPositionTable_mg[38], pawnPositionTable_eg[38]), TS(pawnPositionTable_mg[39], pawnPositionTable_eg[39]),
    TS(pawnPositionTable_mg[40], pawnPositionTable_eg[40]), TS(pawnPositionTable_mg[41], pawnPositionTable_eg[41]),  TS(pawnPositionTable_mg[42], pawnPositionTable_eg[42]), TS(pawnPositionTable_mg[43], pawnPositionTable_eg[43]), TS(pawnPositionTable_mg[44], pawnPositionTable_eg[44]), TS(pawnPositionTable_mg[45], pawnPositionTable_eg[45]), TS(pawnPositionTable_mg[46], pawnPositionTable_eg[46]), TS(pawnPositionTable_mg[47], pawnPositionTable_eg[47]),
    TS(pawnPositionTable_mg[48], pawnPositionTable_eg[48]), TS(pawnPositionTable_mg[49], pawnPositionTable_eg[49]),  TS(pawnPositionTable_mg[50], pawnPositionTable_eg[50]), TS(pawnPositionTable_mg[51], pawnPositionTable_eg[51]), TS(pawnPositionTable_mg[52], pawnPositionTable_eg[52]), TS(pawnPositionTable_mg[53], pawnPositionTable_eg[53]), TS(pawnPositionTable_mg[54], pawnPositionTable_eg[54]), TS(pawnPositionTable_mg[55], pawnPositionTable_eg[55]),
    TS(pawnPositionTable_mg[56], pawnPositionTable_eg[56]), TS(pawnPositionTable_mg[57], pawnPositionTable_eg[57]),  TS(pawnPositionTable_mg[58], pawnPositionTable_eg[58]), TS(pawnPositionTable_mg[59], pawnPositionTable_eg[59]), TS(pawnPositionTable_mg[60], pawnPositionTable_eg[60]), TS(pawnPositionTable_mg[61], pawnPositionTable_eg[61]), TS(pawnPositionTable_mg[62], pawnPositionTable_eg[62]), TS(pawnPositionTable_mg[63], pawnPositionTable_eg[63])
};

constexpr i32 knightPositionTable[64] = {
   -50, -25, -15, -15, -15, -15, -25, -50,
   -40, -20,   0, -10, -10,   0, -20, -40,
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
    15,  20,  20,  25,  25,  20,  20,  15,
    20,  25,  25,  35,  35,  25,  25,  20,
};

constexpr i32 queenPositionTable[64] = {
   -20, -10, -10, -5, -5, -10, -10, -20,
   -10,   0,   0,  0,  0,  0,  0, -10,
   -10,   0,   5,  5,  5,  5,  0, -10,
    -5,   0,   5, 10, 10,  5,  0,  -5,
     0,   0,   5, 10, 10,  5,  0,  -5,
   -10,   5,   5,  5,  5,  5,  0, -10,
   -10,   0,   5,  0,  0,  0,  0, -10
};

constexpr i32 kingPositionTable_mg[64] = {
     20,  30,  10,   0,   0,  10,  30,  20,
     20,  20,   0,   0,   0,   0,  20,  20,
    -10, -20, -20, -20, -20, -20, -20, -10,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -10, -20, -20, -20, -20, -20, -20, -10
};

constexpr i32 kingPositionTable_eg[64] = {
    -30, -20, -10, -10, -10, -10, -20, -30,
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   5,   5,   5,   5,   5,   5, -10,
    -10,   5,  15,  20,  20,  15,   5, -10,
    -10,   5,  15,  20,  20,  15,   5, -10,
    -10,   5,   5,   5,   5,   5,   5, -10,
    -20, -10, -10, -10, -10, -10, -10, -20,
    -30, -20, -10, -10, -10, -10, -20, -30,
};

constexpr TaperedScore kingPositionTaperedScoreTable[64] = {
    TS(kingPositionTable_mg[0], kingPositionTable_eg[0]),  TS(kingPositionTable_mg[1], kingPositionTable_eg[1]),   TS(kingPositionTable_mg[2], kingPositionTable_eg[2]),   TS(kingPositionTable_mg[3], kingPositionTable_eg[3]),   TS(kingPositionTable_mg[4], kingPositionTable_eg[4]),   TS(kingPositionTable_mg[5], kingPositionTable_eg[5]),   TS(kingPositionTable_mg[6], kingPositionTable_eg[6]),   TS(kingPositionTable_mg[7], kingPositionTable_eg[7]),
    TS(kingPositionTable_mg[8], kingPositionTable_eg[8]),  TS(kingPositionTable_mg[9], kingPositionTable_eg[9]),   TS(kingPositionTable_mg[10], kingPositionTable_eg[10]), TS(kingPositionTable_mg[11], kingPositionTable_eg[11]), TS(kingPositionTable_mg[12], kingPositionTable_eg[12]), TS(kingPositionTable_mg[13], kingPositionTable_eg[13]), TS(kingPositionTable_mg[14], kingPositionTable_eg[14]), TS(kingPositionTable_mg[15], kingPositionTable_eg[15]),
    TS(kingPositionTable_mg[16], kingPositionTable_eg[16]), TS(kingPositionTable_mg[17], kingPositionTable_eg[17]),  TS(kingPositionTable_mg[18], kingPositionTable_eg[18]), TS(kingPositionTable_mg[19], kingPositionTable_eg[19]), TS(kingPositionTable_mg[20], kingPositionTable_eg[20]), TS(kingPositionTable_mg[21], kingPositionTable_eg[21]), TS(kingPositionTable_mg[22], kingPositionTable_eg[22]), TS(kingPositionTable_mg[23], kingPositionTable_eg[23]),
    TS(kingPositionTable_mg[24], kingPositionTable_eg[24]), TS(kingPositionTable_mg[25], kingPositionTable_eg[25]),  TS(kingPositionTable_mg[26], kingPositionTable_eg[26]), TS(kingPositionTable_mg[27], kingPositionTable_eg[27]), TS(kingPositionTable_mg[28], kingPositionTable_eg[28]), TS(kingPositionTable_mg[29], kingPositionTable_eg[29]), TS(kingPositionTable_mg[30], kingPositionTable_eg[30]), TS(kingPositionTable_mg[31], kingPositionTable_eg[31]),
    TS(kingPositionTable_mg[32], kingPositionTable_eg[32]), TS(kingPositionTable_mg[33], kingPositionTable_eg[33]),  TS(kingPositionTable_mg[34], kingPositionTable_eg[34]), TS(kingPositionTable_mg[35], kingPositionTable_eg[35]), TS(kingPositionTable_mg[36], kingPositionTable_eg[36]), TS(kingPositionTable_mg[37], kingPositionTable_eg[37]), TS(kingPositionTable_mg[38], kingPositionTable_eg[38]), TS(kingPositionTable_mg[39], kingPositionTable_eg[39]),
    TS(kingPositionTable_mg[40], kingPositionTable_eg[40]), TS(kingPositionTable_mg[41], kingPositionTable_eg[41]),  TS(kingPositionTable_mg[42], kingPositionTable_eg[42]), TS(kingPositionTable_mg[43], kingPositionTable_eg[43]), TS(kingPositionTable_mg[44], kingPositionTable_eg[44]), TS(kingPositionTable_mg[45], kingPositionTable_eg[45]), TS(kingPositionTable_mg[46], kingPositionTable_eg[46]), TS(kingPositionTable_mg[47], kingPositionTable_eg[47]),
    TS(kingPositionTable_mg[48], kingPositionTable_eg[48]), TS(kingPositionTable_mg[49], kingPositionTable_eg[49]),  TS(kingPositionTable_mg[50], kingPositionTable_eg[50]), TS(kingPositionTable_mg[51], kingPositionTable_eg[51]), TS(kingPositionTable_mg[52], kingPositionTable_eg[52]), TS(kingPositionTable_mg[53], kingPositionTable_eg[53]), TS(kingPositionTable_mg[54], kingPositionTable_eg[54]), TS(kingPositionTable_mg[55], kingPositionTable_eg[55]),
    TS(kingPositionTable_mg[56], kingPositionTable_eg[56]), TS(kingPositionTable_mg[57], kingPositionTable_eg[57]),  TS(kingPositionTable_mg[58], kingPositionTable_eg[58]), TS(kingPositionTable_mg[59], kingPositionTable_eg[59]), TS(kingPositionTable_mg[60], kingPositionTable_eg[60]), TS(kingPositionTable_mg[61], kingPositionTable_eg[61]), TS(kingPositionTable_mg[62], kingPositionTable_eg[62]), TS(kingPositionTable_mg[63], kingPositionTable_eg[63])
};

const i32* pestoTables[6] = {
    pawnPositionTable_eg,
    knightPositionTable,
    bishopPositionTable,
    rookPositionTable,
    queenPositionTable,
    kingPositionTable_eg
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
static constexpr TaperedScore passedPawnScore{ 25, 100 };

constexpr i32 center_bias[64] = {
      2,   2,   2,   2,   2,   2,   2,  2,
      2,   4,   4,   4,   4,   4,   4,  2,
      2,   4,   8,   8,   8,   8,   4,  2,
      2,   4,   8,  16,  16,   8,   4,  2,
      2,   4,   8,  16,  16,   8,   4,  2,
      2,   4,   8,   8,   8,   8,   4,  2,
      2,   4,   4,   4,   4,   4,   4,  2,
      2,   2,   2,   2,   2,   2,   2,  2,
};

} // namespace evaluator_data