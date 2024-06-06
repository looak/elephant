#pragma once
#include <array>
#include "defines.hpp"

namespace shifts {
static constexpr i8 horizontal = 1;
static constexpr i8 vertical = 8;
/**
 * Naming comes from forward slash and backslash relative to the set */
static constexpr i8 forward_diagonal = 9;
static constexpr i8 backward_diagonal = 7;

constexpr std::array<i8, 8> shifts = {
    vertical,           // lsh - north
    horizontal,         // lsh - east
    -vertical,           // rsh - south
    -horizontal,         // rsh - west
    forward_diagonal,   // lsh - northeast
    -backward_diagonal,  // rsh - southeast
    -forward_diagonal,   // rsh - southwest
    backward_diagonal   // lsh - northwest
};
}  // namespace shifts

namespace board_constants {
static constexpr u64 rank0Mask = UINT64_C(0x00000000000000FF);
static constexpr u64 rank1Mask = UINT64_C(0x000000000000FF00);
static constexpr u64 rank2Mask = UINT64_C(0x0000000000FF0000);
static constexpr u64 rank3Mask = UINT64_C(0x00000000FF000000);
static constexpr u64 rank4Mask = UINT64_C(0x000000FF00000000);
static constexpr u64 rank5Mask = UINT64_C(0x0000FF0000000000);
static constexpr u64 rank6Mask = UINT64_C(0x00FF000000000000);
static constexpr u64 rank7Mask = UINT64_C(0xFF00000000000000);

static constexpr u64 fileaMask = UINT64_C(0x0101010101010101);
static constexpr u64 filebMask = UINT64_C(0x0202020202020202);
static constexpr u64 filecMask = UINT64_C(0x0404040404040404);
static constexpr u64 filedMask = UINT64_C(0x0808080808080808);
static constexpr u64 fileeMask = UINT64_C(0x1010101010101010);
static constexpr u64 filefMask = UINT64_C(0x2020202020202020);
static constexpr u64 filegMask = UINT64_C(0x4040404040404040);
static constexpr u64 filehMask = UINT64_C(0x8080808080808080);

static constexpr u64 forwardDiagonalMasks[15] = {0x0100000000000000, 0x0201000000000000, 0x0402010000000000, 0x0804020100000000,
                                                 0x1008040201000000, 0x2010080402010000, 0x4020100804020100, 0x8040201008040201,
                                                 0x0080402010080402, 0x0000804020100804, 0x0000008040201008, 0x0000000080402010,
                                                 0x0000000000804020, 0x0000000000008040, 0x0000000000000080};

static constexpr u64 backwardDiagonalMasks[15] = {
    0x0000000000000001, 0x0000000000000102, 0x0000000000010204, 0x0000000001020408, 0x0000000102040810,
    0x0000010204081020, 0x0001020408102040, 0x0102040810204080, 0x0204081020408000, 0x0408102040800000,
    0x0810204080000000, 0x1020408000000000, 0x2040800000000000, 0x4080000000000000, 0x8000000000000000,
};

static constexpr u64 darkSquares = UINT64_C(0xAA55AA55AA55AA55);
static constexpr u64 lightSquares = UINT64_C(0x55AA55AA55AA55AA);

static constexpr u64 rankMasks[8] = {rank0Mask, rank1Mask, rank2Mask, rank3Mask, rank4Mask, rank5Mask, rank6Mask, rank7Mask};

static constexpr u64 fileMasks[8] = {fileaMask, filebMask, filecMask, filedMask, fileeMask, filefMask, filegMask, filehMask};

static constexpr u64 boundsRelativeMasks[2][4] = {{rank7Mask, filehMask, rank0Mask, fileaMask},
                                                  {rank0Mask, fileaMask, rank7Mask, filehMask}};

static constexpr u64 enPassantRankRelative[2] = {rank3Mask, rank4Mask};
static constexpr u64 baseRankRelative[2] = {rank0Mask, rank7Mask};

constexpr u64 constexprAbs(i64 val) {
    return val < 0 ? -val : val;
}

constexpr std::array<std::array<u64, 64>, 64>
generateManhattanDistances() {
    std::array<std::array<u64, 64>, 64> result{};
    for (int i = 0; i < 64; ++i)
    {
        for (int j = 0; j < 64; ++j)
        {
            int rankDiff = (i / 8) - (j / 8);
            int fileDiff = (i % 8) - (j % 8);
            result[i][j] = constexprAbs(rankDiff) + constexprAbs(fileDiff);
        }
    }

    return result;
}

static constexpr auto manhattanDistances = generateManhattanDistances();

}  // namespace board_constants

namespace pawn_constants {
static constexpr u64 baseRank[2] = {board_constants::rank2Mask, board_constants::rank5Mask};
static constexpr u64 promotionRank[2] = {board_constants::rank7Mask, board_constants::rank0Mask};
static constexpr u64 moveMask[2] = {UINT64_C(0x10100), UINT64_C(0x101)};
}  // namespace pawn_constants

namespace king_constants {
static constexpr u64 moveMask = UINT64_C(0x70507);
static constexpr i32 maskOffset = 9;
static constexpr u64 queenSideCastleMask = board_constants::filecMask;
static constexpr u64 kingSideCastleMask = board_constants::filegMask;
}  // namespace king_constants

consteval std::array<u64, 64>
generateSquareLookupTable()
{
    std::array<u64, 64> result{};
    for (int i = 0; i < 64; ++i)
        result[i] = UINT64_C(1) << i;

    return result;
}
/**
 * Lookup table for per-square 64bit masks. Will match the Square class enum     */
static constexpr auto squareMaskTable = generateSquareLookupTable();