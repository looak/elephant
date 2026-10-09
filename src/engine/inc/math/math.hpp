#pragma once
#include <system/platform.hpp>

constexpr byte mod_by_eight(byte v) { return v & 7; }

consteval u64 constexprAbs(i64 val) {
    return static_cast<u64>(val >= 0 ? val : -(val + 1) + 1);
}