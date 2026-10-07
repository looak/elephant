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
 * @brief Policies wrapping each move generator behind the same interface, so callers like perft can be templated on
 * the generator and A/B tested.
 *
 * A policy provides:
 *     static constexpr const char* name;
 *     template<Set us, typename F> static void forEachMove(PositionReader position, F&& onMove);
 * onMove(PackedMove) is called once per legal move. The generator stays alive for the duration of the loop, so onMove
 * may make/unmake moves as long as the position is restored before it returns.
 *
 * @author Alexander Loodin Ek    */

#pragma once

#include <move/generation/king_pin_threats.hpp>
#include <move/generation/move_generator.hpp>
#include <move/generation/tusk/move_generator.hpp>
#include <position/position_accessors.hpp>

namespace move_gen_policy {

// The original MoveGenerator, scores and sorts every move.
struct Legacy {
    static constexpr const char* name = "legacy";

    template<Set us, typename F>
    static void forEachMove(PositionReader position, F&& onMove) {
        MoveGenParams params;
        MoveGenerator<us> generator(position, params);
        while (PrioritizedMove move = generator.pop())
            onMove(move.move);
    }
};

// tusk::MoveGenerator, unordered generateAll().
struct Tusk {
    static constexpr const char* name = "tusk";

    template<Set us, typename F>
    static void forEachMove(PositionReader position, F&& onMove) {
        KingPinThreats<us> pins(to_square(position.material().king<us>().lsbIndex()), position);
        tusk::MoveGenerator<us> generator(position, pins);
        tusk::MoveGenResult<us> moves = generator.generateAll();
        while (PrioritizedMove move = moves.next())
            onMove(move.move);
    }
};

} // namespace move_gen_policy
