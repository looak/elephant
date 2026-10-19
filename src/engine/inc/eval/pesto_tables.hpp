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
 * The values are packed midgame & endgame Scores, S(mg, eg), the form a tuner writes them back in.
 *
 * PeSTO's published values, except pestoPawn: retuned with the pawn terms in pawn_tables.hpp, the rest kept
 * (--tune psqt_pawn,pawns_passed,pawns_structure,pawns_passed_extras on E12.52-1M-D12-Resolved, K 1.1324, 1000 passes
 * at rate 1, error 0.086746 -> 0.086180, held back positions 0.65% lower). The pawn terms now pay for passed and
 * connected pawns, the table no longer does: its 6th rank endgame values dropped, its central midgame ones too. The
 * king table was tried as well and left out, it learned to march the king up the board.    */

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
    S(  96, 164), S(  95, 175), S(  67, 174), S(  82, 158), S( 103, 148), S( 114, 139), S(  10, 178), S(  38, 153), // 7
    S( -31,  49), S(  -6,  49), S(  -3,  32), S(  -8,  18), S(  11,  11), S(  64,  15), S(  46,  25), S(  -1,  26), // 6
    S( -28,  25), S( -12,  23), S( -24,  12), S( -13,  -8), S(   4,  -6), S(   9,  -5), S(  11,   9), S( -10,   2), // 5
    S( -34,  17), S( -19,  22), S( -26,   3), S( -23,   1), S( -13,   0), S( -15,   1), S(   4,   5), S( -17,  -2), // 4
    S( -42,  14), S( -34,  19), S( -34,   8), S( -32,   8), S( -23,  11), S( -17,   7), S(  -4,   6), S( -17,  -8), // 3
    S( -35,  19), S( -20,  22), S( -32,  20), S( -28,  19), S( -28,  26), S(   4,  19), S(  27,  10), S(  -9, -12), // 2
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), // 1
};

constexpr TaperedScore pestoKnight[64] = {
    //   A             B             C             D             E             F             G             H
    S(-167, -58), S( -89, -38), S( -34, -13), S( -49, -28), S(  61, -31), S( -97, -27), S( -15, -63), S(-107, -99), // 8
    S( -73, -25), S( -41,  -8), S(  72, -25), S(  36,  -2), S(  23,  -9), S(  62, -25), S(   7, -24), S( -17, -52), // 7
    S( -47, -24), S(  60, -20), S(  37,  10), S(  65,   9), S(  84,  -1), S( 129,  -9), S(  73, -19), S(  44, -41), // 6
    S(  -9, -17), S(  17,   3), S(  19,  22), S(  53,  22), S(  37,  22), S(  69,  11), S(  18,   8), S(  22, -18), // 5
    S( -13, -18), S(   4,  -6), S(  16,  16), S(  13,  25), S(  28,  16), S(  19,  17), S(  21,   4), S(  -8, -18), // 4
    S( -23, -23), S(  -9,  -3), S(  12,  -1), S(  10,  15), S(  19,  10), S(  17,  -3), S(  25, -20), S( -16, -22), // 3
    S( -29, -42), S( -53, -20), S( -12, -10), S(  -3,  -5), S(  -1,  -2), S(  18, -20), S( -14, -23), S( -19, -44), // 2
    S(-105, -29), S( -21, -51), S( -58, -23), S( -33, -15), S( -17, -22), S( -28, -18), S( -19, -50), S( -23, -64), // 1
};

constexpr TaperedScore pestoBishop[64] = {
    //   A             B             C             D             E             F             G             H
    S( -29, -14), S(   4, -21), S( -82, -11), S( -37,  -8), S( -25,  -7), S( -42,  -9), S(   7, -17), S(  -8, -24), // 8
    S( -26,  -8), S(  16,  -4), S( -18,   7), S( -13, -12), S(  30,  -3), S(  59, -13), S(  18,  -4), S( -47, -14), // 7
    S( -16,   2), S(  37,  -8), S(  43,   0), S(  40,  -1), S(  35,  -2), S(  50,   6), S(  37,   0), S(  -2,   4), // 6
    S(  -4,  -3), S(   5,   9), S(  19,  12), S(  50,   9), S(  37,  14), S(  37,  10), S(   7,   3), S(  -2,   2), // 5
    S(  -6,  -6), S(  13,   3), S(  13,  13), S(  26,  19), S(  34,   7), S(  12,  10), S(  10,  -3), S(   4,  -9), // 4
    S(   0, -12), S(  15,  -3), S(  15,   8), S(  15,  10), S(  14,  13), S(  27,   3), S(  18,  -7), S(  10, -15), // 3
    S(   4, -14), S(  15, -18), S(  16,  -7), S(   0,  -1), S(   7,   4), S(  21,  -9), S(  33, -15), S(   1, -27), // 2
    S( -33, -23), S(  -3,  -9), S( -14, -23), S( -21,  -5), S( -13,  -9), S( -12, -16), S( -39,  -5), S( -21, -17), // 1
};

constexpr TaperedScore pestoRook[64] = {
    //   A             B             C             D             E             F             G             H
    S(  32,  13), S(  42,  10), S(  32,  18), S(  51,  15), S(  63,  12), S(   9,  12), S(  31,   8), S(  43,   5), // 8
    S(  27,  11), S(  32,  13), S(  58,  13), S(  62,  11), S(  80,  -3), S(  67,   3), S(  26,   8), S(  44,   3), // 7
    S(  -5,   7), S(  19,   7), S(  26,   7), S(  36,   5), S(  17,   4), S(  45,  -3), S(  61,  -5), S(  16,  -3), // 6
    S( -24,   4), S( -11,   3), S(   7,  13), S(  26,   1), S(  24,   2), S(  35,   1), S(  -8,  -1), S( -20,   2), // 5
    S( -36,   3), S( -26,   5), S( -12,   8), S(  -1,   4), S(   9,  -5), S(  -7,  -6), S(   6,  -8), S( -23, -11), // 4
    S( -45,  -4), S( -25,   0), S( -16,  -5), S( -17,  -1), S(   3,  -7), S(   0, -12), S(  -5,  -8), S( -33, -16), // 3
    S( -44,  -6), S( -16,  -6), S( -20,   0), S(  -9,   2), S(  -1,  -9), S(  11,  -9), S(  -6, -11), S( -71,  -3), // 2
    S( -19,  -9), S( -13,   2), S(   1,   3), S(  17,  -1), S(  16,  -5), S(   7, -13), S( -37,   4), S( -26, -20), // 1
};

constexpr TaperedScore pestoQueen[64] = {
    //   A             B             C             D             E             F             G             H
    S( -28,  -9), S(   0,  22), S(  29,  22), S(  12,  27), S(  59,  27), S(  44,  19), S(  43,  10), S(  45,  20), // 8
    S( -24, -17), S( -39,  20), S(  -5,  32), S(   1,  41), S( -16,  58), S(  57,  25), S(  28,  30), S(  54,   0), // 7
    S( -13, -20), S( -17,   6), S(   7,   9), S(   8,  49), S(  29,  47), S(  56,  35), S(  47,  19), S(  57,   9), // 6
    S( -27,   3), S( -27,  22), S( -16,  24), S( -16,  45), S(  -1,  57), S(  17,  40), S(  -2,  57), S(   1,  36), // 5
    S(  -9, -18), S( -26,  28), S(  -9,  19), S( -10,  47), S(  -2,  31), S(  -4,  34), S(   3,  39), S(  -3,  23), // 4
    S( -14, -16), S(   2, -27), S( -11,  15), S(  -2,   6), S(  -5,   9), S(   2,  17), S(  14,  10), S(   5,   5), // 3
    S( -35, -22), S(  -8, -23), S(  11, -30), S(   2, -16), S(   8, -16), S(  15, -23), S(  -3, -36), S(   1, -32), // 2
    S(  -1, -33), S( -18, -28), S(  -9, -22), S(  10, -43), S( -15,  -5), S( -25, -32), S( -31, -20), S( -50, -41), // 1
};

constexpr TaperedScore pestoKing[64] = {
    //   A             B             C             D             E             F             G             H
    S( -65, -74), S(  23, -35), S(  16, -18), S( -15, -18), S( -56, -11), S( -34,  15), S(   2,   4), S(  13, -17), // 8
    S(  29, -12), S(  -1,  17), S( -20,  14), S(  -7,  17), S(  -8,  17), S(  -4,  38), S( -38,  23), S( -29,  11), // 7
    S(  -9,  10), S(  24,  17), S(   2,  23), S( -16,  15), S( -20,  20), S(   6,  45), S(  22,  44), S( -22,  13), // 6
    S( -17,  -8), S( -20,  22), S( -12,  24), S( -27,  27), S( -30,  26), S( -25,  33), S( -14,  26), S( -36,   3), // 5
    S( -49, -18), S(  -1,  -4), S( -27,  21), S( -39,  24), S( -46,  27), S( -44,  23), S( -33,   9), S( -51, -11), // 4
    S( -14, -19), S( -14,  -3), S( -22,  11), S( -46,  21), S( -44,  23), S( -30,  16), S( -15,   7), S( -27,  -9), // 3
    S(   1, -27), S(   7, -11), S(  -8,   4), S( -64,  13), S( -43,  14), S( -16,   4), S(   9,  -5), S(   8, -17), // 2
    S( -15, -53), S(  36, -34), S(  12, -21), S( -54, -11), S(   8, -28), S( -28, -14), S(  24, -24), S(  14, -43), // 1
};

// by piece index, pawn to king.
constexpr const TaperedScore* pestoTables[6] = {
    pestoPawn, pestoKnight, pestoBishop, pestoRook, pestoQueen, pestoKing
};

} // namespace evaluator_data
