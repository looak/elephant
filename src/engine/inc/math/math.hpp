#pragma once
#include <system/platform.hpp>

constexpr byte mod_by_eight(byte v) { return v & 7; }

constexpr u64 constexprAbs(i64 val) {
    u64 uval = static_cast<u64>(val);
    // If negative, negate the UNSIGNED value.
    // In 2's complement, -x is equivalent to (~x + 1).
    // Unsigned negation performs exactly this operation safely.
    return (val < 0) ? -uval : uval;
}