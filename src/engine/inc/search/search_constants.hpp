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
#include "defines.hpp"

static constexpr u32 c_maxSearchDepth = 64;
static constexpr i32 c_infinity = 32000;
static constexpr i32 c_checkmateConstant = 24000;
static constexpr i32 c_nullMoveOffset = 500;
static constexpr i32 c_checkmateMaxDistance = 256;
static constexpr i16 c_checkmateMinScore = c_checkmateConstant - c_checkmateMaxDistance;
static constexpr i16 c_drawConstant = 0;

// late move reduction parameters
namespace lmr_params {
static constexpr u32 minDepth = 3;
static constexpr u32 earlyReductionThreshold = 8; // was 6
static constexpr u32 reduceAfterIndex = 5;
} // namespace lmr_params

