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
 * @brief Resolves a position to a quiet one for the tuner: the engine's own quiescence search records the line it
 * plays (Search::quiescenceLine), then that line is played out. The evaluation is fitted on the position the line ends
 * in, where no capture is pending that the static evaluation can't see.    */

#pragma once

#include <search/search_results.hpp>
#include <search/search_threadcontext.hpp>

namespace tuner {

/**
 * Searches context's position with quiescence and plays its line on context, leaving context's position & accumulator
 * on the quiet position at its end. Returns that line, empty when the position already was quiet. Sets decided, and
 * leaves context as it was, when the search finds a mate for either side: the line ends in a mated position, there's
 * no quiet one to fit. context needs stopEnabled false.    */
PVLine resolveToQuiet(ThreadSearchContext& context, bool& decided);

} // namespace tuner
