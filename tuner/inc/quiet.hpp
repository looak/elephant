// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2025  Alexander Loodin Ek

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

/**
 * @file quiet.hpp
 * @brief Resolves a position to a quiet one for the tuner: the engine's quiescence search, mirrored here with a
 * principal variation, then that line played out. The evaluation is fitted on the position the line ends in, where
 * no capture is pending that the static evaluation can't see.
 *
 * Search::quiescence returns only a score, the engine's hot path stays as it is. This copy follows it step for step,
 * stand pat, captures only, delta & SEE pruning, every evasion in check, and the same depth limit, built from the
 * engine's own move source, move executor & evaluation.    */

#pragma once

#include <array>

#include <move/move.hpp>
#include <search/search_threadcontext.hpp>
#include <system/platform.hpp>

namespace tuner {

// the longest quiescence line followed, the engine's ply limit bounds it anyway.
constexpr u32 c_maxQuietLine = 64;

struct QuietLine {
    std::array<PackedMove, c_maxQuietLine> moves{};
    u32 length = 0;
};

/**
 * Searches context's position with quiescence and plays its principal variation on context, leaving context's
 * position & accumulator on the quiet position at its end. Returns that line, empty when the position already was
 * quiet. Sets decided, and leaves context as it was, when the search finds a mate for either side: the line ends in a
 * mated position, there's no quiet one to fit. context needs stopEnabled false.    */
QuietLine resolveToQuiet(ThreadSearchContext& context, bool& decided);

} // namespace tuner
