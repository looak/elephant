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

// aspiration window parameters
namespace aspiration_params {
// from this depth the root is searched in a window around the previous iteration's score, earlier scores swing too much.
inline constexpr u8 minDepth = 5;
inline constexpr i32 initialDelta = 25;
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
// captures that lose material by SEE are reduced this many plies less than quiets, a sacrifice can still be the move.
inline constexpr i32 losingCaptureOffset = 2;
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
// reduction = base + depth / depthDivisor + min((staticEval - beta) / evalDivisor, maxEvalReduction), deeper nodes and
// a bigger margin over beta reduce more. The common 3 + depth / 3 + min(.., 3) never found the zugzwang win in the
// null move search test, the verification search shrinks with the null move's depth.
inline constexpr i32 baseReduction = 2;
inline constexpr i32 depthDivisor = 4;
inline constexpr i32 evalDivisor = 200;
inline constexpr i32 maxEvalReduction = 1;
} // namespace nmp_params

// futility pruning parameters
namespace futility_params {
// quiet moves are skipped at nodes up to this depth when the static eval is this far below alpha.
inline constexpr u8 maxDepth = 3;
// margin = base + perDepth * depth.
inline constexpr i16 base = 100;
inline constexpr i16 perDepth = 100;
} // namespace futility_params

// late move pruning parameters
namespace lmp_params {
// at non-PV nodes up to this depth only the first base + factor * depth * depth quiet moves are searched. The common
// 3 + depth * depth pushed the zugzwang win in the null move search test from depth 15 to 20, this costs it a ply.
inline constexpr u8 maxDepth = 5;
inline constexpr u16 base = 5;
inline constexpr u16 factor = 2;
} // namespace lmp_params

// reverse futility pruning parameters
namespace rfp_params {
// nodes up to this depth return when the static eval beats beta by margin * depth.
inline constexpr u8 maxDepth = 6;
inline constexpr i16 margin = 80;
} // namespace rfp_params

namespace quiescence_params {
inline constexpr u32 defaultMaxDepth = 8;
// delta pruning, slack on top of the material a capture can win before it's judged hopeless.
inline constexpr i16 deltaMargin = 200; // 2 pawns
} // namespace quiescence_params
