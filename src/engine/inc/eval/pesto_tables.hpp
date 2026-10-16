// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2025  Alexander Loodin Ek

// This program is free software : you can redistribute it and /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.If not, see < http://www.gnu.org/licenses/>.

/**
 * @file pesto_tables.hpp
 * @brief PeSTO's material values, piece square tables & game phase weights, split out of evaluator_data.hpp so the
 * position's PestoAccumulator can use them. All constexpr, safe to include anywhere.
 *
 * The values are packed midgame & endgame Scores, S(mg, eg), the form a tuner writes them back in.    */

#pragma once
#include <system/platform.hpp>
#include <eval/tapered_score.hpp>

namespace evaluator_data
{

constexpr u32 flip(const u32 index) {
    return index ^ 56;
}

/**
 * PeSTO's material values and piece/sq tables, Texel tuned by Ronald Friederich for PeSTO (values from Rofchade).
 * https://www.chessprogramming.org/PeSTO%27s_Evaluation_Function
 * Material and tables were tuned together, the eval uses these values and not piece_constants::value.
 *
 * The tables were then retuned by ElephantTuner from PeSTO's, material kept as PeSTO's: E12.52-1M-D12-Resolved
 * (~10M positions resolved with Elephant's quiescence), K 1.1545, 1000 passes at rate 1, error 0.089893 -> 0.087695
 * (held back positions 2.39% lower).
 *
 * The tables are laid out as published: from white's point of view with A8 at index 0, rank 8 on the top row. Our
 * squares have A1 at 0, so a white piece reads the table at flip(sqr) and a black piece at sqr. */
constexpr TaperedScore pestoMaterial[6] = {
    S(82, 94), S(337, 281), S(365, 297), S(477, 512), S(1025, 936), S(0, 0)
};

// game phase each piece adds, maxGamePhase (tapered_score.hpp) with all pieces on the board. Promotions can push it
// past, the taper caps it there.
constexpr i32 gamePhaseIncrement[6] = { 0, 1, 1, 2, 4, 0 };

constexpr TaperedScore pestoPawn[64] = {
    //   A             B             C             D             E             F             G             H
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), // 8
    S( 123, 172), S( 134, 168), S( 116, 157), S( 132, 122), S( 148, 115), S( 122, 119), S(  -6, 175), S(  17, 162), // 7
    S( -46, 141), S(  -5, 136), S(  13, 120), S(  17, 104), S(  27,  93), S(  49,  83), S(  17, 109), S( -33, 109), // 6
    S( -47,  73), S( -26,  63), S( -31,  54), S( -21,  36), S(  -6,  31), S(  -8,  32), S(  -9,  48), S( -36,  44), // 5
    S( -52,  49), S( -36,  48), S( -40,  29), S( -37,  27), S( -30,  26), S( -32,  28), S( -17,  33), S( -36,  27), // 4
    S( -54,  41), S( -44,  42), S( -48,  31), S( -46,  33), S( -37,  35), S( -35,  32), S( -15,  34), S( -32,  20), // 3
    S( -56,  51), S( -47,  50), S( -54,  46), S( -53,  44), S( -53,  49), S( -25,  47), S(  -6,  41), S( -35,  18), // 2
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), // 1
};

constexpr TaperedScore pestoKnight[64] = {
    //   A             B             C             D             E             F             G             H
    S(-254,   0), S(-192,  63), S(-162,  76), S( -81,  59), S( -37,  61), S(-154,  94), S( -56,  41), S(-194, -38), // 8
    S(-107,  27), S(-106,  55), S( -74,  65), S( -48,  87), S( -41,  79), S( -26,  52), S(-107,  60), S( -69,  24), // 7
    S(-111,  46), S( -83,  74), S( -56,  92), S( -46,  95), S( -11,  85), S(   2,  89), S( -46,  75), S( -64,  48), // 6
    S( -99,  48), S( -91,  80), S( -75,  98), S( -62, 111), S( -78, 114), S( -53, 105), S( -82,  99), S( -72,  63), // 5
    S(-111,  44), S( -98,  74), S( -94, 100), S( -96, 107), S( -90, 107), S( -91, 105), S( -76,  83), S( -99,  57), // 4
    S(-122,  13), S(-109,  61), S(-104,  69), S( -94,  88), S( -94,  86), S(-101,  71), S(-101,  64), S(-112,  27), // 3
    S(-135,  11), S(-134,  50), S(-117,  46), S(-109,  59), S(-112,  64), S(-107,  44), S(-116,  39), S(-114,  22), // 2
    S(-160, -24), S(-122, -13), S(-153,  30), S(-133,  41), S(-124,  40), S(-120,  30), S(-118,   0), S(-163, -51), // 1
};

constexpr TaperedScore pestoBishop[64] = {
    //   A             B             C             D             E             F             G             H
    S(-152,  96), S(-129,  89), S(-168,  92), S(-155,  99), S(-152,  91), S(-168,  91), S( -63,  70), S(-127,  83), // 8
    S(-132,  71), S(-102,  91), S(-107,  93), S(-125,  96), S(-103,  95), S( -88,  87), S(-106,  92), S(-110,  66), // 7
    S(-111,  81), S( -95,  90), S( -75,  92), S( -75,  90), S( -57,  88), S( -45, 100), S( -59,  93), S( -99,  86), // 6
    S(-119,  79), S( -99,  94), S( -91,  93), S( -65,  98), S( -84, 110), S( -72,  96), S(-102, 108), S(-107,  84), // 5
    S(-117,  70), S(-107,  86), S(-104, 100), S(-100, 102), S( -91,  98), S(-109,  98), S(-104,  86), S(-111,  72), // 4
    S(-117,  67), S(-109,  82), S(-109,  89), S(-106,  91), S(-109,  93), S(-110,  84), S(-108,  71), S(-103,  65), // 3
    S(-112,  61), S(-110,  56), S(-104,  65), S(-119,  73), S(-118,  76), S(-113,  62), S(-101,  60), S(-101,  33), // 2
    S(-110,  45), S(-108,  60), S(-128,  39), S(-145,  62), S(-140,  62), S(-125,  54), S(-127,  52), S(-119,  47), // 1
};

constexpr TaperedScore pestoRook[64] = {
    //   A             B             C             D             E             F             G             H
    S( -77, 137), S( -70, 138), S( -80, 147), S( -65, 141), S( -62, 145), S( -45, 140), S( -25, 131), S( -23, 129), // 8
    S(-121, 146), S(-129, 152), S(-100, 150), S( -87, 150), S( -91, 149), S( -53, 122), S( -91, 133), S( -54, 119), // 7
    S(-143, 142), S(-112, 135), S(-114, 145), S( -95, 136), S( -80, 128), S( -58, 127), S( -41, 109), S( -89, 125), // 6
    S(-156, 133), S(-146, 138), S(-135, 143), S(-117, 137), S(-123, 137), S(-115, 133), S(-107, 121), S(-116, 123), // 5
    S(-174, 118), S(-173, 133), S(-172, 138), S(-163, 135), S(-166, 131), S(-166, 132), S(-143, 122), S(-153, 114), // 4
    S(-183,  98), S(-174, 116), S(-179, 117), S(-174, 114), S(-172, 115), S(-178, 112), S(-145, 108), S(-167,  99), // 3
    S(-206,  97), S(-176,  97), S(-169, 105), S(-169, 104), S(-171, 103), S(-160,  97), S(-153,  95), S(-196, 102), // 2
    S(-169,  99), S(-166, 108), S(-161, 116), S(-157, 115), S(-160, 113), S(-163, 111), S(-145, 106), S(-163,  89), // 1
};

constexpr TaperedScore pestoQueen[64] = {
    //   A             B             C             D             E             F             G             H
    S( -51,  92), S( -36, 106), S( -21, 111), S( -16, 115), S(  -6, 124), S(  38, 121), S(  24, 126), S(  32, 105), // 8
    S( -50,  67), S( -67,  94), S( -40, 102), S( -69, 147), S( -49, 158), S(  11, 115), S( -67, 166), S(  -2, 112), // 7
    S( -46,  50), S( -37,  57), S( -38,  87), S( -33, 113), S( -17, 133), S(  26, 124), S(  31, 102), S(  -2, 118), // 6
    S( -36,  27), S( -42,  68), S( -42,  82), S( -44, 113), S( -36, 131), S( -23, 138), S( -17, 127), S( -18, 111), // 5
    S( -38,  19), S( -37,  61), S( -43,  63), S( -45,  95), S( -45,  92), S( -33,  86), S( -31,  78), S( -16,  74), // 4
    S( -39,  10), S( -34,  28), S( -37,  46), S( -39,  36), S( -39,  37), S( -38,  49), S( -22,  28), S( -23,  22), // 3
    S( -41,  -4), S( -38,  14), S( -33,   0), S( -37,  15), S( -35,   7), S( -24, -34), S( -19, -47), S( -28, -15), // 2
    S( -40,  12), S( -43,  -6), S( -44,   1), S( -37,  -2), S( -39, -10), S( -57,  -9), S( -53, -24), S( -42,  13), // 1
};

constexpr TaperedScore pestoKing[64] = {
    //   A             B             C             D             E             F             G             H
    S(  -9,-162), S( 140, -67), S( 145, -54), S( 103, -31), S(  29, -31), S(  44, -28), S(  80, -24), S(  11,-110), // 8
    S(  68, -18), S( 132,  15), S( 139,   5), S( 120,  -9), S(  83,  -2), S( 129,   6), S(  93,  31), S( -36,  -3), // 7
    S(  77,  11), S( 136,  29), S( 164,  20), S(  93,  23), S( 114,  15), S( 135,  32), S( 110,  36), S( -10,  15), // 6
    S(  54,   5), S(  88,  25), S(  85,  35), S(  48,  38), S(  62,  33), S(  73,  31), S(  52,  29), S(  -7,   5), // 5
    S(  15, -20), S(  49,  15), S(  49,  29), S(   2,  41), S(  25,  34), S(  15,  29), S(  23,  15), S( -48,  -4), // 4
    S( -10, -16), S(  -8,   6), S( -18,  20), S( -28,  30), S( -23,  28), S( -30,  21), S(  -6,   3), S( -30,  -9), // 3
    S(   0,  -2), S( -15,   0), S( -42,  13), S( -76,  18), S( -55,  14), S( -58,  17), S(  -9,  -4), S(   3, -22), // 2
    S(  -9, -47), S(  15, -15), S( -19,  -6), S( -89, -14), S( -37, -45), S( -87,   1), S(   1, -23), S(  10, -67), // 1
};

// by piece index, pawn to king.
constexpr const TaperedScore* pestoTables[6] = {
    pestoPawn, pestoKnight, pestoBishop, pestoRook, pestoQueen, pestoKing
};

} // namespace evaluator_data
