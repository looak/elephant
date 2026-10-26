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
 * king table was tried as well and left out, it learned to march the king up the board.
 *
 * Rebalanced in 1.4, every term fitted together but material, the king's table and kingDanger (--tune psqt_pawn,
 * psqt_knight, psqt_bishop, psqt_rook, psqt_queen, mobility, pawns_passed, pawns_structure, pawns_passed_extras,
 * king_shelter, pieces_pairs --min-usage 50000 on E12.52-1M-D12-Resolved, K 1.1889, 1000 passes at rate 1, error
 * 0.085192 -> 0.084398, held back 0.87% lower). Each tuned piece square table kept centred on its own average, so
 * the pieces keep their values and only the shapes moved; the 120 values fewer than 50k positions use kept as they
 * were.    */

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
    S(  90, 165), S(  94, 172), S(  67, 174), S(  82, 158), S( 103, 148), S( 114, 139), S(  10, 178), S(  38, 153), // 7
    S( -31,  46), S(  -6,  39), S(   0,  21), S(   6,  -4), S(  23,  -8), S(  27,  15), S(   4,  34), S( -33,  38), // 6
    S( -30,  30), S(  -5,  20), S( -22,  10), S(  -5, -16), S(  10, -16), S( -17,   4), S(  12,  14), S( -25,  15), // 5
    S( -32,  20), S( -11,  20), S( -21,   2), S( -19,  -7), S(  -8, -10), S( -13,   4), S(  13,  10), S( -31,   9), // 4
    S( -39,  16), S( -21,  15), S( -32,   7), S( -26,   2), S( -18,   5), S( -19,   5), S(   8,   7), S( -34,   6), // 3
    S( -30,  22), S( -12,  19), S( -28,  13), S( -22,  11), S( -24,  18), S(  -9,   7), S(  17,   9), S( -24,  13), // 2
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), // 1
};

constexpr TaperedScore pestoKnight[64] = {
    //   A             B             C             D             E             F             G             H
    S(-167, -58), S( -89, -38), S( -34, -13), S( -49, -28), S(  61, -31), S( -97, -27), S( -15, -63), S(-107, -99), // 8
    S( -73, -25), S( -41,  -8), S(  72, -25), S(  36,  -2), S(  23,  -9), S(  62, -25), S(   7, -24), S( -17, -52), // 7
    S( -47, -24), S(  15,  -5), S(  45,  17), S(  52,  18), S(  83,   5), S(  97,   6), S(  73, -19), S(  44, -41), // 6
    S(   3, -16), S(  15,   5), S(  33,  24), S(  48,  31), S(  25,  33), S(  48,  23), S(  22,  15), S(  30, -13), // 5
    S(  -5, -18), S(   8,  -2), S(  16,  23), S(  19,  29), S(  21,  25), S(  17,  24), S(  28,   3), S(   6, -12), // 4
    S( -16, -52), S(  -1, -12), S(   6,  -5), S(  17,  14), S(  19,  10), S(  13,  -6), S(   7, -17), S(  -3, -29), // 3
    S( -29, -42), S( -53, -20), S(  -7, -28), S(   6, -10), S(   3,  -5), S(   3, -27), S(  -3, -24), S(  -4, -38), // 2
    S(-105, -29), S( -12, -54), S( -58, -23), S( -16, -17), S(  -4, -21), S(  -9, -24), S(  -6, -49), S( -23, -64), // 1
};

constexpr TaperedScore pestoBishop[64] = {
    //   A             B             C             D             E             F             G             H
    S( -29, -14), S(   4, -21), S( -82, -11), S( -37,  -8), S( -25,  -7), S( -42,  -9), S(   7, -17), S(  -8, -24), // 8
    S( -26,  -8), S(   4,   8), S(   3,  10), S( -13, -12), S(  30,  -3), S(  59, -13), S(  18,  -4), S( -47, -14), // 7
    S(  -2,   8), S(  23,   5), S(  37,   5), S(  32,   3), S(  47,   6), S(  60,  16), S(  45,  11), S(   0,  12), // 6
    S(  -8,   1), S(  21,   7), S(  16,  10), S(  45,  12), S(  29,  17), S(  30,  10), S(  13,  16), S(  -5,   6), // 5
    S(   0,  -5), S(   8,   1), S(  14,  12), S(  21,  16), S(  25,   9), S(   5,  10), S(  12,  -1), S(   2,  -5), // 4
    S(   1, -10), S(  12,   1), S(  11,   5), S(  14,   6), S(  11,   9), S(  16,   1), S(  13,  -5), S(  17,  -8), // 3
    S(   4, -14), S(  13, -20), S(  15, -11), S(   4,  -7), S(   8,  -2), S(  12, -10), S(  26, -15), S(  20, -26), // 2
    S(  18,  -4), S(  -3,  -9), S(  -2, -14), S( -16,  -8), S( -13,  -8), S(   5,  -8), S( -39,  -5), S( -21, -17), // 1
};

constexpr TaperedScore pestoRook[64] = {
    //   A             B             C             D             E             F             G             H
    S(  35,  23), S(  36,  26), S(   6,  37), S(  24,  29), S(  20,  31), S(  44,  23), S(  64,  19), S(  75,  15), // 8
    S(   9,  27), S(  -2,  32), S(  28,  26), S(  38,  27), S(  34,  25), S(  62,   0), S(  31,  13), S(  64,   1), // 7
    S(  -6,  24), S(  31,  11), S(  26,  19), S(  45,  10), S(  53,   2), S(  66,   5), S(  77,  -9), S(  27,   9), // 6
    S( -11,  19), S(   3,  17), S(  14,  20), S(  35,  13), S(  26,  13), S(  24,  10), S(  27,   3), S(  13,   9), // 5
    S( -26,   7), S( -22,  17), S( -18,  20), S( -10,  14), S( -12,  12), S( -16,  13), S(  -1,   5), S( -17,   0), // 4
    S( -30, -10), S( -21,   4), S( -24,   3), S( -16,   0), S( -14,  -1), S( -20,  -3), S(   1,  -5), S( -24, -12), // 3
    S( -52, -10), S( -23, -15), S( -12, -10), S(  -7, -15), S(  -9, -15), S(  -1, -23), S(   0, -21), S( -71,  -3), // 2
    S( -13, -15), S(  -6, -10), S(   3,  -5), S(  10, -10), S(   7, -11), S(   3,  -7), S(   8, -14), S(  -7, -24), // 1
};

constexpr TaperedScore pestoQueen[64] = {
    //   A             B             C             D             E             F             G             H
    S( -28,  -9), S(   0,  22), S(  29,  22), S(  12,  27), S(  59,  27), S(  44,  19), S(  43,  10), S(  45,  20), // 8
    S( -33,  33), S( -56,  55), S(  -5,  32), S(   1,  41), S( -16,  58), S(  57,  25), S(  28,  30), S(  54,   0), // 7
    S( -31,  17), S( -17,   9), S( -22,  40), S(   8,  49), S(  29,  47), S(  56,  35), S(  47,  19), S(  -9,  91), // 6
    S( -13,  -6), S( -16,  26), S( -18,  30), S( -20,  68), S( -13,  80), S( -18,  97), S( -11,  95), S( -18,  79), // 5
    S(  -7, -19), S(  -7,  16), S( -10,  18), S( -14,  47), S( -19,  53), S(  -9,  43), S(  -5,  38), S(  -3,  39), // 4
    S(  -6, -27), S(   1,  -8), S(   0,   2), S(  -6,  -3), S(  -3,  -3), S(  -3,  13), S(   6,  -3), S(   5,   5), // 3
    S(  -3, -33), S(  -1, -28), S(   5, -39), S(   5, -29), S(   7, -29), S(  18, -68), S(  15, -74), S(   1, -32), // 2
    S(   0, -39), S(   5, -53), S(   6, -42), S(  12, -25), S(  15, -43), S(  -4, -52), S( -31, -20), S( -50, -41), // 1
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
