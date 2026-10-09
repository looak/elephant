#pragma once
#include <system/platform.hpp>

constexpr byte mod_by_eight(byte v) { return v & 7; }

consteval u64 constexprAbs(i64 val) {
    // add the 1 back after converting, -INT64_MIN doesn't fit in an i64.
    return val >= 0 ? static_cast<u64>(val) : static_cast<u64>(-(val + 1)) + 1;
}

static_assert(constexprAbs(-3) == 3 && constexprAbs(3) == 3 && constexprAbs(0) == 0);
static_assert(constexprAbs(INT64_MIN) == 9223372036854775808ull);