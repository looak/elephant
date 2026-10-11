// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021-2026  Alexander Loodin Ek

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
 * @file move_gen_params.hpp
 * @brief Parameters and shared enums for the tusk move generator.
 *
 * @author Alexander Loodin Ek    */

#pragma once

#include <move/move.hpp>
#include <move/generation/move_ordering_view.hpp>

namespace tusk {

// Which subset of moves a single generation pass produces. Used as a template parameter so each pass is a fully
// specialized instantiation.
enum class GenType : u8 {
    CAPTURES,   // captures and promotions
    QUIETS,     // everything else, including castling
    ALL
};

// Order in which MoveGenResult hands out moves. *_GEN stages generate a batch into the result and fall through to
// the stage that hands it out.
enum class Stage : u8 {
    PV_MOVE,
    TT_MOVE,
    CAPTURES_GEN,
    CAPTURES,
    KILLERS,
    QUIETS_GEN,
    QUIETS,
    LOSING_CAPTURES,  // captures that lose material by SEE, held back from CAPTURES when deferLosingCaptures is set
    UNORDERED,  // generateAll(), moves handed out in generation order
    DONE
};

// Read-only input for a MoveGenerator. Held by value in the generator, keep it small.
struct MoveGenParams {
    const MoveOrderingView* ordering = nullptr;
    MoveTypes moveFilter = MoveTypes::ALL;
    // captures & promotions that lose material by static exchange are handed out after the quiets instead of with the
    // other captures. Only with MoveTypes::ALL.
    bool deferLosingCaptures = false;
};

} // namespace tusk
