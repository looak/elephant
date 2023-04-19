#pragma once
#include "defines.h"

static const u64 rank0Mask = UINT64_C(0x00000000000000FF);
static const u64 rank1Mask = UINT64_C(0x000000000000FF00);
static const u64 rank2Mask = UINT64_C(0x0000000000FF0000);
static const u64 rank3Mask = UINT64_C(0x00000000FF000000);
static const u64 rank4Mask = UINT64_C(0x000000FF00000000);
static const u64 rank5Mask = UINT64_C(0x0000FF0000000000);
static const u64 rank6Mask = UINT64_C(0x00FF000000000000);
static const u64 rank7Mask = UINT64_C(0xFF00000000000000);

static const u64 rankMasks[8] = {
    rank0Mask,
    rank1Mask,
    rank2Mask,
    rank3Mask,
    rank4Mask,
    rank5Mask,
    rank6Mask,
    rank7Mask
};
