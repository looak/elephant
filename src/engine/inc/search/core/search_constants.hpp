// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2024  Alexander Loodin Ek

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
#pragma once
#include <system/platform.hpp>

#include <array>

inline constexpr u32 c_maxSearchDepth = 64;
inline constexpr i32 c_infinity = 28000;
inline constexpr i32 c_checkmateConstant = 24000;
inline constexpr i32 c_checkmateMaxDistance = 256;
inline constexpr i16 c_checkmateMinScore = c_checkmateConstant - c_checkmateMaxDistance;
inline constexpr i16 c_drawConstant = 0;

namespace time_management {
    inline constexpr std::array<u8, 6> unchangedMoveFactors = { 140, 115, 100, 85, 70, 60 };
}

#ifdef ELEPHANT_TUNE
    #define TUNABLE(ns, name, type, def, lo, hi, step) namespace ns { inline type name = def; }
#else
    #define TUNABLE(ns, name, type, def, lo, hi, step) namespace ns { inline constexpr type name = def; }
#endif
#include "search_tunables.def"
#undef TUNABLE

// aspiration window parameters
namespace aspiration_params {
// a side of the window that would grow beyond this opens up fully.
inline constexpr i32 maxDelta = 1000;
} // namespace aspiration_params

// late move reduction parameters
namespace lmr_params {
inline constexpr u8 minDepth = 3;
// moves before this index are searched at full depth, covers the pv/tt move and usually the killers.
inline constexpr u16 fullDepthMoves = 3;
// reduction = base + ln(depth) * ln(move index) / divisor, one ply less at PV nodes.
inline constexpr double base = 0.75;
inline constexpr double divisor = 2.25;
// a ply less per this much history, a ply more per this much below zero. The score is three tables of at most 16383 each,
// so at most 5 plies either way, and under this it changes nothing. A first guess, not fitted on anything.
inline constexpr i32 historyDivisor = 8192;
} // namespace lmr_params


namespace singular_params {
    inline constexpr u8 marginPerDepth = 4;

} // namespace singular_params

// internal iterative reduction parameters
namespace iir_params {
// nodes from this depth with neither a TT nor a PV move to search first are searched a ply shallower.
inline constexpr u8 minDepth = 4;
} // namespace iir_params

namespace quiescence_params {
inline constexpr u32 defaultMaxDepth = 8;
} // namespace quiescence_params

namespace continuation_params {
// parameters for continuation history heuristic
// pliesBack, slots reserved in front of the per-ply continuation pointers. Scoring a move reads the entries of the
// moves made 1 and 2 plies earlier, which near the root would index ply - 1 and ply - 2 below zero.
inline constexpr u8 pliesBack = 2;
} // namespace continuation_params