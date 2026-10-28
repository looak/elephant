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
 * were.
 *
 * Then fine-tuned on Elephant's own games, the same groups, kept values and centring as the rebalance (--min-usage
 * 6000 on elephant-positions-0.1, 1.2M positions from nyayo, K 1.2633): its held back error 1.23% lower, but
 * E12.52's 1.13% higher (0.084365 -> 0.085321, K 1.2302 -> 1.1608), the two datasets disagree.    */

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
    S(  90, 130), S(  65, 160), S(  30, 158), S(  33, 146), S( 103, 148), S( 114, 139), S(  10, 178), S(  14, 124), // 7
    S(   5,  17), S( -22,  35), S( -15,  14), S(   1,  -1), S(  35, -11), S( -24,  18), S(  32,  25), S( -46,  21), // 6
    S( -20,  24), S( -16,  28), S( -14,   4), S(   1, -13), S(  20, -13), S( -23,   7), S( -18,  24), S( -16,   9), // 5
    S( -23,  14), S( -25,  24), S( -15,  -4), S( -10,  -5), S(  -3,  -4), S( -22,   8), S( -15,  22), S( -25,   2), // 4
    S( -25,   8), S( -32,  21), S( -26,   0), S( -18,   7), S( -16,  12), S( -19,   4), S(  -5,  15), S( -24,  -2), // 3
    S( -16,  15), S( -21,  29), S( -23,   8), S( -18,  21), S( -14,  20), S( -12,   1), S(   0,  24), S( -11,  -4), // 2
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), // 1
};

constexpr TaperedScore pestoKnight[64] = {
    //   A             B             C             D             E             F             G             H
    S(-167, -58), S( -89, -38), S( -34, -13), S( -49, -28), S(  61, -31), S( -97, -27), S( -15, -63), S(-107, -99), // 8
    S( -73, -25), S( -41,  -8), S(  72, -25), S(  36,  -2), S(  23,  -9), S(  62, -25), S(   7, -24), S( -17, -52), // 7
    S( -47, -24), S(  13,   5), S(  51,  16), S(  59,  16), S(  94,   6), S(  99,   1), S(  73, -19), S(  44, -41), // 6
    S(  16,  -6), S(   3,  12), S(  35,  22), S(  55,  25), S(  30,  15), S(  50,  18), S(  22,  12), S(  41, -12), // 5
    S(  -5, -13), S(   1,   0), S(  19,  22), S(  22,  23), S(  26,  23), S(  24,  13), S(  39,  -6), S(  17, -12), // 4
    S( -37, -36), S( -12,  -1), S(  -4,  -2), S(  21,  12), S(  23,   9), S(   8,  -2), S(  15, -18), S(   1, -17), // 3
    S( -29, -42), S( -53, -20), S(  -9, -19), S(  12, -11), S(   8, -12), S(   2, -20), S(  -3, -24), S(   2, -38), // 2
    S(-105, -29), S( -14, -47), S( -58, -23), S( -16, -17), S(   4, -16), S(   3, -39), S( -16, -41), S( -23, -64), // 1
};

constexpr TaperedScore pestoBishop[64] = {
    //   A             B             C             D             E             F             G             H
    S( -29, -14), S(   4, -21), S( -82, -11), S( -37,  -8), S( -25,  -7), S( -42,  -9), S(   7, -17), S(  -8, -24), // 8
    S( -26,  -8), S(  -1,  25), S(  -2,  11), S( -13, -12), S(  30,  -3), S(  59, -13), S(  18,  -4), S( -47, -14), // 7
    S(   5,  13), S(  15,   7), S(  38,   3), S(  52,   8), S(  52,   4), S(  94,   5), S(  80,  20), S(  20,   8), // 6
    S( -21,  13), S(  11,  10), S(  17,  13), S(  59,   6), S(  24,  13), S(  37,  15), S(   7,   9), S(   2,   2), // 5
    S(   1,  -4), S(  -3,   6), S(  14,   9), S(  25,  16), S(  31,   1), S(   2,   5), S(  15,  -8), S(  20,  -2), // 4
    S(  -5, -10), S(  21,  -1), S(  13,   4), S(  19,   0), S(   9,  12), S(  14,   2), S(  13, -11), S(  13,  -4), // 3
    S(  21,  -1), S(   3, -10), S(  29, -15), S(   2,  -3), S(  14,  -5), S(   4,  -8), S(  21, -16), S(  12, -18), // 2
    S(  18,  -4), S(  15,  -9), S(  -6, -13), S( -18, -13), S(  -8, -11), S(  -2,  -4), S( -39,  -5), S( -21, -17), // 1
};

constexpr TaperedScore pestoRook[64] = {
    //   A             B             C             D             E             F             G             H
    S(  54,  10), S(  54,  14), S(  43,  19), S(  47,  15), S(  50,  11), S(  79,   3), S(  29,  17), S(  37,  10), // 8
    S(  -2,  32), S(   2,  27), S(  30,  23), S(  49,  16), S(  76,   6), S( 101,  -9), S(  52,   2), S(  80,  -7), // 7
    S(  20,  22), S(  23,  20), S(  28,  19), S(  43,  11), S(  63,  -2), S(  76,  -3), S(  75,  -8), S(  56,   4), // 6
    S(  -9,  26), S(   1,  22), S(  22,  20), S(  37,   4), S(  37,   6), S(  30,   6), S(  48,  -2), S( -11,  13), // 5
    S( -34,  15), S( -37,  27), S(  -8,  16), S(   2,   9), S(   6,  11), S( -19,  16), S(  -3,   5), S( -37,   7), // 4
    S( -36,   5), S( -28,   2), S( -29,   9), S( -33,  12), S(  -8,  -6), S( -16,  -4), S(  -4, -14), S( -34, -11), // 3
    S( -37,  -1), S( -28,   0), S( -18,   1), S( -13,  -5), S( -11, -14), S(  -2, -26), S(  -1,  -8), S( -71,  -3), // 2
    S( -16,  -9), S(  -4, -13), S(  -1,  -2), S(   5,  -9), S(   5, -12), S(   1, -16), S(  -7,  -6), S( -18,  -8), // 1
};

constexpr TaperedScore pestoQueen[64] = {
    //   A             B             C             D             E             F             G             H
    S( -28,  -9), S(   0,  22), S(  29,  22), S(  12,  27), S(  59,  27), S(  44,  19), S(  43,  10), S(  45,  20), // 8
    S( -33,  33), S( -42,  55), S(  -5,  32), S(   1,  41), S( -16,  58), S(  57,  25), S(  28,  30), S(  54,   0), // 7
    S( -31,  17), S( -17,   9), S( -36,  71), S(   8,  49), S(  29,  47), S(  56,  35), S(  47,  19), S(  20,  15), // 6
    S( -12,   6), S(  -6,  24), S(  -9,  41), S( -18,  69), S(   5,  56), S( -37,  79), S( -16,  56), S(  -1,   9), // 5
    S( -12,  15), S( -21,  29), S(  -9,  32), S( -15,  52), S( -11,  36), S( -14,  24), S(   8,  10), S(   3,  -5), // 4
    S( -17,  -1), S(  -7,   6), S(  -9,  17), S( -12,   8), S(  -6,   9), S(  -2,  15), S(   0,  -4), S(   4,   3), // 3
    S(  -3, -33), S(  -9, -11), S(   1, -25), S(  14, -46), S(  10, -19), S(  20, -81), S(  15, -74), S(   1, -32), // 2
    S(   0, -39), S(  -3, -39), S(   6, -46), S(  13, -28), S(   9, -58), S(  -4, -52), S( -31, -20), S( -50, -41), // 1
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
