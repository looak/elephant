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
 * @file move_gen_policy.hpp
 * @brief Policies wrapping a move generator behind one interface, so callers like perft and the move generator tests
 * are templated on the generator and a new one can be A/B tested against tusk.
 *
 * A policy provides:
 *     static constexpr const char* name;
 *     template<Set us, typename F> static void forEachMove(PositionReader position, F&& onMove);
 * onMove(PackedMove) is called once per legal move. The generator stays alive for the duration of the loop, so onMove
 * may make/unmake moves as long as the position is restored before it returns.
 *
 * @author Alexander Loodin Ek    */

#pragma once

#include <move/generation/tusk/move_generator.hpp>
#include <position/position_accessors.hpp>

namespace move_gen_policy {

// tusk::MoveGenerator, unordered generateAll().
struct Tusk {
    static constexpr const char* name = "tusk";

    template<Set us, typename F>
    static void forEachMove(PositionReader position, F&& onMove) {
        tusk::CheckInfo<us> checkInfo(position);
        tusk::MoveGenerator<us> generator(position, checkInfo);
        tusk::MoveGenResult<us> moves = generator.generateAll();
        while (PrioritizedMove move = moves.next())
            onMove(move.move);
    }
};

} // namespace move_gen_policy
