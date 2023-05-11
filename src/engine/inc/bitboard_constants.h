#pragma once
#include "defines.h"
#include <array>

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

static constexpr u64 darkSquares = UINT64_C(0xAA55AA55AA55AA55);
static constexpr u64 lightSquares = UINT64_C(0x55AA55AA55AA55AA);

static constexpr u64 rankMasks[8] = {
    rank0Mask,
    rank1Mask,
    rank2Mask,
    rank3Mask,
    rank4Mask,
    rank5Mask,
    rank6Mask,
    rank7Mask
};

static constexpr u64 fileMasks[8] = {
    fileaMask,
    filebMask,
    filecMask,
    filedMask,
    fileeMask,
    filefMask,
    filegMask,
    filehMask
};

constexpr std::array<u64, 64> generateSquareLookupTable() 
{
    std::array<u64, 64> result{};
    for (int i = 0; i < 64; ++i)
        result[i] = UINT64_C(1) << i;

    return result;
}

static constexpr auto squareMaskTable = generateSquareLookupTable();

