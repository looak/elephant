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
 * E12.52's 1.13% higher (0.084365 -> 0.085321, K 1.2302 -> 1.1608), the two datasets disagree.
 *
 * Then refitted on Elephant's games since 1.4, the same groups, kept values and centring (--min-usage 7250 on
 * positions-3, 1.45M positions from nyayo's games since 2026-10-30, K 1.3545): its held back error 0.57% lower.
 * Unlike the first own-games fit it agrees with E12.52, whose error is 0.26% lower (0.085307 -> 0.085085), while
 * elephant-positions-0.1's is 0.74% higher, the older games those tables were fitted on.    */

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
    S(  72, 128), S(  94, 158), S(  30, 158), S(  33, 146), S( 103, 148), S( 114, 139), S(  10, 178), S(  14, 124), // 7
    S(  -6,  29), S(   7,  49), S(   1,  22), S(  14,   3), S(  50,  -4), S(  28,  13), S(  16,  48), S( -10,  12), // 6
    S( -30,  27), S(  -5,  28), S( -14,   9), S(   2, -19), S(   8, -15), S( -16,   8), S(  -2,  20), S( -10,   7), // 5
    S( -31,  16), S( -12,  24), S( -16,   5), S( -14,  -7), S(  -6,  -5), S( -16,   8), S(   5,  15), S( -29,   3), // 4
    S( -38,  12), S( -19,  20), S( -27,   3), S( -25,   9), S( -17,   8), S( -24,   5), S(   0,   7), S( -28,  -2), // 3
    S( -31,  17), S( -14,  28), S( -24,  12), S( -27,  18), S( -18,  17), S( -10,   6), S(   8,  14), S( -26,   6), // 2
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0), // 1
};

constexpr TaperedScore pestoKnight[64] = {
    //   A             B             C             D             E             F             G             H
    S(-167, -58), S( -89, -38), S( -34, -13), S( -49, -28), S(  61, -31), S( -97, -27), S( -15, -63), S(-107, -99), // 8
    S( -73, -25), S( -41,  -8), S(  72, -25), S(  36,  -2), S(  23,  -9), S(  62, -25), S(   7, -24), S( -17, -52), // 7
    S( -47, -24), S(  18,   4), S(  58,  16), S(  58,  19), S(  86,   8), S(  99,   1), S(  73, -19), S(  44, -41), // 6
    S(  14,  -4), S(   6,  15), S(  38,  21), S(  48,  25), S(  21,  21), S(  42,  22), S(  18,  16), S(  31, -10), // 5
    S(   0,  -8), S(  11,   0), S(  15,  26), S(  18,  25), S(  24,  23), S(  16,  18), S(  34,  -4), S(   4,  -4), // 4
    S( -16, -40), S(  -5,  -6), S(   3,  -1), S(  16,  17), S(  24,   9), S(   9,  -7), S(  13, -18), S(   2, -25), // 3
    S( -29, -42), S( -53, -20), S(   2, -24), S(   8, -14), S(  12, -13), S(   2, -20), S(  -5, -21), S(   6, -40), // 2
    S(-105, -29), S( -10, -44), S( -58, -23), S(  -3, -19), S(   7, -31), S(   3, -39), S( -14, -44), S( -23, -64), // 1
};

constexpr TaperedScore pestoBishop[64] = {
    //   A             B             C             D             E             F             G             H
    S( -29, -14), S(   4, -21), S( -82, -11), S( -37,  -8), S( -25,  -7), S( -42,  -9), S(   7, -17), S(  -8, -24), // 8
    S( -26,  -8), S(  -5,  19), S(   5,   8), S( -13, -12), S(  30,  -3), S(  59, -13), S(  18,  -4), S( -47, -14), // 7
    S(  10,   7), S(  15,  12), S(  30,   3), S(  32,   5), S(  64,   5), S(  95,   8), S(  55,  21), S(   8,   5), // 6
    S(  -7,   3), S(  12,  11), S(  21,   8), S(  45,   9), S(  32,   7), S(  31,  14), S(   2,  11), S(   0,   8), // 5
    S(   2,  -7), S(   3,   3), S(  10,   8), S(  32,  12), S(  23,   8), S(   1,   1), S(   9,   3), S(  14, -10), // 4
    S(   1, -11), S(  13,   2), S(  15,   2), S(  12,   2), S(  10,  13), S(  16,   0), S(  19, -14), S(  18, -11), // 3
    S(  10,   1), S(  14, -17), S(  20, -11), S(  -1,  -3), S(  12,  -3), S(  19, -14), S(  27, -10), S(  18, -26), // 2
    S(   6, -12), S(   8, -11), S(  -4, -13), S(  -9,  -9), S(  -2, -14), S(  -3,  -4), S( -39,  -5), S( -21, -17), // 1
};

constexpr TaperedScore pestoRook[64] = {
    //   A             B             C             D             E             F             G             H
    S(  40,  13), S(  36,  15), S(  22,  19), S(  17,  24), S(  -4,  28), S(  53,   5), S(  49,   4), S(  44,   8), // 8
    S(  14,  24), S(   6,  29), S(  38,  23), S(  60,  13), S(  63,   8), S(  84,  -4), S(  59,  -1), S(  65,   2), // 7
    S(   6,  25), S(  27,  17), S(  20,  20), S(  56,   3), S(  68,  -3), S(  70,  -1), S(  61,   6), S(  26,   7), // 6
    S(  -8,  24), S(   5,  22), S(  20,  18), S(  34,  14), S(  27,  13), S(  21,  12), S(  12,  13), S( -15,  15), // 5
    S( -24,  13), S( -18,  14), S( -15,  14), S(  -4,  12), S(  -2,   5), S( -28,  15), S( -27,  11), S( -44,  10), // 4
    S( -38,   3), S( -25,   0), S( -26,  11), S( -23,   5), S( -15,  -5), S( -18,  -7), S( -12, -11), S( -42,  -3), // 3
    S( -46,   0), S( -27,  -3), S(  -8,  -7), S(  -3,  -8), S( -11, -13), S(  -6, -21), S(  -9,  -8), S( -71,  -3), // 2
    S( -13,  -6), S(  -5,  -9), S(   2,  -1), S(  12, -10), S(  10, -15), S(   2, -16), S( -22,   5), S( -18, -14), // 1
};

constexpr TaperedScore pestoQueen[64] = {
    //   A             B             C             D             E             F             G             H
    S( -28,  -9), S(   0,  22), S(  29,  22), S(  12,  27), S(  59,  27), S(  44,  19), S(  43,  10), S(  45,  20), // 8
    S( -33,  33), S( -34,  47), S(  -5,  32), S(   1,  41), S( -16,  58), S(  57,  25), S(  28,  30), S(  54,   0), // 7
    S( -17,  10), S( -17,   9), S(  -9,  51), S(   8,  49), S(  29,  47), S(  56,  35), S(  47,  19), S(  -7,  43), // 6
    S( -22,  20), S( -10,  31), S( -16,  55), S(  -5,  49), S(  -7,  58), S( -31,  70), S( -16,  56), S( -22,  41), // 5
    S(  -8,  -4), S( -16,  33), S(  -9,  34), S( -13,  39), S( -12,  44), S( -12,  25), S(  -4,  28), S( -25,  45), // 4
    S(  -9,  -8), S(  -2,   0), S(  -3,   3), S(  -9,  -2), S(   1,  -9), S(  -2,  15), S(   1, -10), S(   0, -18), // 3
    S(   2, -21), S(  -5,  -5), S(   4, -23), S(  11, -32), S(  11, -29), S(  14, -63), S(  26,-112), S(   1, -32), // 2
    S(   0, -39), S(  -8, -38), S(   2, -32), S(   9, -26), S(  20, -55), S(  -4, -52), S( -31, -20), S( -50, -41), // 1
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
