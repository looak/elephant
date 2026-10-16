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
 * Shape only: each table's average (midgame & endgame apart, pawns over ranks 2-7) moved back to PeSTO's, the tuned
 * shape within the table kept. Most of the tuning moved whole tables, which works like a material change; this keeps
 * only how the squares compare. A test of whether that move is what lost Elo.
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
    S( 145, 145), S( 156, 141), S( 138, 130), S( 154,  95), S( 170,  88), S( 144,  92), S(  16, 148), S(  39, 135), // 7
    S( -24, 114), S(  17, 109), S(  35,  93), S(  39,  77), S(  49,  66), S(  71,  56), S(  39,  82), S( -11,  82), // 6
    S( -25,  46), S(  -4,  36), S(  -9,  27), S(   1,   9), S(  16,   4), S(  14,   5), S(  13,  21), S( -14,  17), // 5
    S( -30,  22), S( -14,  21), S( -18,   2), S( -15,   0), S(  -8,  -1), S( -10,   1), S(   5,   6), S( -14,   0), // 4
    S( -32,  14), S( -22,  15), S( -26,   4), S( -24,   6), S( -15,   8), S( -13,   5), S(   7,   7), S( -10,  -7), // 3
    S( -34,  24), S( -25,  23), S( -32,  19), S( -31,  17), S( -31,  22), S(  -3,  20), S(  16,  14), S( -13,  -9), // 2
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), // 1
};

constexpr TaperedScore pestoKnight[64] = {
    //   A             B             C             D             E             F             G             H
    S(-155, -73), S( -93, -10), S( -63,   3), S(  18, -14), S(  62, -12), S( -55,  21), S(  43, -32), S( -95,-111), // 8
    S(  -8, -46), S(  -7, -18), S(  25,  -8), S(  51,  14), S(  58,   6), S(  73, -21), S(  -8, -13), S(  30, -49), // 7
    S( -12, -27), S(  16,   1), S(  43,  19), S(  53,  22), S(  88,  12), S( 101,  16), S(  53,   2), S(  35, -25), // 6
    S(   0, -25), S(   8,   7), S(  24,  25), S(  37,  38), S(  21,  41), S(  46,  32), S(  17,  26), S(  27, -10), // 5
    S( -12, -29), S(   1,   1), S(   5,  27), S(   3,  34), S(   9,  34), S(   8,  32), S(  23,  10), S(   0, -16), // 4
    S( -23, -60), S( -10, -12), S(  -5,  -4), S(   5,  15), S(   5,  13), S(  -2,  -2), S(  -2,  -9), S( -13, -46), // 3
    S( -36, -62), S( -35, -23), S( -18, -27), S( -10, -14), S( -13,  -9), S(  -8, -29), S( -17, -34), S( -15, -51), // 2
    S( -61, -97), S( -23, -86), S( -54, -43), S( -34, -32), S( -25, -33), S( -21, -43), S( -19, -73), S( -64,-124), // 1
};

constexpr TaperedScore pestoBishop[64] = {
    //   A             B             C             D             E             F             G             H
    S( -38,  11), S( -15,   4), S( -54,   7), S( -41,  14), S( -38,   6), S( -54,   6), S(  51, -15), S( -13,  -2), // 8
    S( -18, -14), S(  12,   6), S(   7,   8), S( -11,  11), S(  11,  10), S(  26,   2), S(   8,   7), S(   4, -19), // 7
    S(   3,  -4), S(  19,   5), S(  39,   7), S(  39,   5), S(  57,   3), S(  69,  15), S(  55,   8), S(  15,   1), // 6
    S(  -5,  -6), S(  15,   9), S(  23,   8), S(  49,  13), S(  30,  25), S(  42,  11), S(  12,  23), S(   7,  -1), // 5
    S(  -3, -15), S(   7,   1), S(  10,  15), S(  14,  17), S(  23,  13), S(   5,  13), S(  10,   1), S(   3, -13), // 4
    S(  -3, -18), S(   5,  -3), S(   5,   4), S(   8,   6), S(   5,   8), S(   4,  -1), S(   6, -14), S(  11, -20), // 3
    S(   2, -24), S(   4, -29), S(  10, -20), S(  -5, -12), S(  -4,  -9), S(   1, -23), S(  13, -25), S(  13, -52), // 2
    S(   4, -40), S(   6, -25), S( -14, -46), S( -31, -23), S( -26, -23), S( -11, -31), S( -13, -33), S(  -5, -38), // 1
};

constexpr TaperedScore pestoRook[64] = {
    //   A             B             C             D             E             F             G             H
    S(  60,  14), S(  67,  15), S(  57,  24), S(  72,  18), S(  75,  22), S(  92,  17), S( 112,   8), S( 114,   6), // 8
    S(  16,  23), S(   8,  29), S(  37,  27), S(  50,  27), S(  46,  26), S(  84,  -1), S(  46,  10), S(  83,  -4), // 7
    S(  -6,  19), S(  25,  12), S(  23,  22), S(  42,  13), S(  57,   5), S(  79,   4), S(  96, -14), S(  48,   2), // 6
    S( -19,  10), S(  -9,  15), S(   2,  20), S(  20,  14), S(  14,  14), S(  22,  10), S(  30,  -2), S(  21,   0), // 5
    S( -37,  -5), S( -36,  10), S( -35,  15), S( -26,  12), S( -29,   8), S( -29,   9), S(  -6,  -1), S( -16,  -9), // 4
    S( -46, -25), S( -37,  -7), S( -42,  -6), S( -37,  -9), S( -35,  -8), S( -41, -11), S(  -8, -15), S( -30, -24), // 3
    S( -69, -26), S( -39, -26), S( -32, -18), S( -32, -19), S( -34, -20), S( -23, -26), S( -16, -28), S( -59, -21), // 2
    S( -32, -24), S( -29, -15), S( -24,  -7), S( -20,  -8), S( -23, -10), S( -26, -12), S(  -8, -17), S( -26, -34), // 1
};

constexpr TaperedScore pestoQueen[64] = {
    //   A             B             C             D             E             F             G             H
    S( -19,  36), S(  -4,  50), S(  11,  55), S(  16,  59), S(  26,  68), S(  70,  65), S(  56,  70), S(  64,  49), // 8
    S( -18,  11), S( -35,  38), S(  -8,  46), S( -37,  91), S( -17, 102), S(  43,  59), S( -35, 110), S(  30,  56), // 7
    S( -14,  -6), S(  -5,   1), S(  -6,  31), S(  -1,  57), S(  15,  77), S(  58,  68), S(  63,  46), S(  30,  62), // 6
    S(  -4, -29), S( -10,  12), S( -10,  26), S( -12,  57), S(  -4,  75), S(   9,  82), S(  15,  71), S(  14,  55), // 5
    S(  -6, -37), S(  -5,   5), S( -11,   7), S( -13,  39), S( -13,  36), S(  -1,  30), S(   1,  22), S(  16,  18), // 4
    S(  -7, -46), S(  -2, -28), S(  -5, -10), S(  -7, -20), S(  -7, -19), S(  -6,  -7), S(  10, -28), S(   9, -34), // 3
    S(  -9, -60), S(  -6, -42), S(  -1, -56), S(  -5, -41), S(  -3, -49), S(   8, -90), S(  13,-103), S(   4, -71), // 2
    S(  -8, -44), S( -11, -62), S( -12, -55), S(  -5, -58), S(  -7, -66), S( -25, -65), S( -21, -80), S( -10, -43), // 1
};

constexpr TaperedScore pestoKing[64] = {
    //   A             B             C             D             E             F             G             H
    S( -57,-158), S(  92, -63), S(  97, -50), S(  55, -27), S( -19, -27), S(  -4, -24), S(  32, -20), S( -37,-106), // 8
    S(  20, -14), S(  84,  19), S(  91,   9), S(  72,  -5), S(  35,   2), S(  81,  10), S(  45,  35), S( -84,   1), // 7
    S(  29,  15), S(  88,  33), S( 116,  24), S(  45,  27), S(  66,  19), S(  87,  36), S(  62,  40), S( -58,  19), // 6
    S(   6,   9), S(  40,  29), S(  37,  39), S(   0,  42), S(  14,  37), S(  25,  35), S(   4,  33), S( -55,   9), // 5
    S( -33, -16), S(   1,  19), S(   1,  33), S( -46,  45), S( -23,  38), S( -33,  33), S( -25,  19), S( -96,   0), // 4
    S( -58, -12), S( -56,  10), S( -66,  24), S( -76,  34), S( -71,  32), S( -78,  25), S( -54,   7), S( -78,  -5), // 3
    S( -48,   2), S( -63,   4), S( -90,  17), S(-124,  22), S(-103,  18), S(-106,  21), S( -57,   0), S( -45, -18), // 2
    S( -57, -43), S( -33, -11), S( -67,  -2), S(-137, -10), S( -85, -41), S(-135,   5), S( -47, -19), S( -38, -63), // 1
};

// by piece index, pawn to king.
constexpr const TaperedScore* pestoTables[6] = {
    pestoPawn, pestoKnight, pestoBishop, pestoRook, pestoQueen, pestoKing
};

} // namespace evaluator_data
