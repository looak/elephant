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

inline constexpr u32 c_maxSearchDepth = 64;
inline constexpr i32 c_infinity = 28000;
inline constexpr i32 c_checkmateConstant = 24000;
inline constexpr i32 c_checkmateMaxDistance = 256;
inline constexpr i16 c_checkmateMinScore = c_checkmateConstant - c_checkmateMaxDistance;
inline constexpr i16 c_drawConstant = 0;

// late move reduction parameters
namespace lmr_params {
inline constexpr u8 minDepth = 3;
// moves before this index are searched at full depth, covers the pv/tt move and usually the killers.
inline constexpr u16 fullDepthMoves = 3;
// reduction = base + ln(depth) * ln(move index) / divisor, one ply less at PV nodes.
inline constexpr double base = 0.75;
inline constexpr double divisor = 2.25;
} // namespace lmr_params

// history heuristic parameters
namespace history_params {
// bonus for a quiet cutoff move is depth * depth * scale up to maxBonus, the quiets tried before it get the same malus.
inline constexpr i32 bonusScale = 32;
inline constexpr i32 maxBonus = 2048;
} // namespace history_params

// null move pruning parameters
namespace nmp_params {
inline constexpr u8 minDepth = 3;
// null move fail highs from this depth up are verified with a reduced normal search before cutting, guards against
// zugzwang where passing is better than any real move.
inline constexpr u8 verificationDepth = 6;
} // namespace nmp_params

namespace quiescence_params {
inline constexpr u32 defaultMaxDepth = 8;
// delta pruning, slack on top of the material a capture can win before it's judged hopeless.
inline constexpr i16 deltaMargin = 200; // 2 pawns
} // namespace quiescence_params
