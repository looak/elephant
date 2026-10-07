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
 * @file move_generator.hpp
 * @brief tusk::MoveGenerator, a stateless legal move generator. All mutable state lives in MoveGenResult.
 *
 * Usage:
 *     KingPinThreats<us> pins(kingSq, pos);
 *     MoveGenerator<us> gen(pos, pins, { .ordering = &ordering });
 *     MoveGenResult<us> moves = gen.generate();
 *     while (PrioritizedMove m = moves.next()) { ... }
 *
 * The generator reads the position on every stage transition. Callers may make/unmake moves between next() calls
 * as long as the position is fully restored before the next call.
 *
 * @author Alexander Loodin Ek    */

#pragma once

#include <move/generation/king_pin_threats.hpp>
#include <move/generation/tusk/move_gen_params.hpp>
#include <move/generation/tusk/move_gen_result.hpp>
#include <position/position_accessors.hpp>

namespace tusk {

template<Set us>
class MoveGenerator {
public:
    MoveGenerator(PositionReader position, const KingPinThreats<us>& pinThreats, const MoveGenParams& params = {});

    // Lazy, staged and ordered. Nothing is generated until the first next()/peek().
    [[nodiscard]] MoveGenResult<us> generate() const;

    // Eager, every legal move in generation order and unscored. For perft and other callers that want all moves.
    [[nodiscard]] MoveGenResult<us> generateAll() const;

    [[nodiscard]] bool isChecked() const { return m_pinThreats.isChecked(); }

private:
    friend class MoveGenResult<us>;

    // Stage machine behind MoveGenResult::next(). Hands out pending moves in result, otherwise runs the next stage.
    [[nodiscard]] PrioritizedMove advance(MoveGenResult<us>& result) const;

    // Fills result with every legal move, unscored. Called by MoveGenResult when constructed in Stage::UNORDERED.
    void generateUnordered(MoveGenResult<us>& result) const;

    // Calls sink(PackedMove, u8 pieceId) for every legal move of the given type from the given source squares.
    template<GenType type, typename Sink>
    void generateMoves(Sink& sink, u64 sources) const;

    // Generates a scored batch into result, skipping moves already handed out by earlier stages.
    template<GenType type>
    void generateScored(MoveGenResult<us>& result) const;

    // True when move is legal in the current position, used for pv/tt/killer moves handed out before generation.
    [[nodiscard]] bool isLegal(PackedMove move) const;

    // True when move is one of the ordering moves (pv, tt and optionally killers) handed out before generation.
    [[nodiscard]] bool isOrderingMove(PackedMove move, bool includeKillers) const;

    PositionReader m_position;
    const KingPinThreats<us>& m_pinThreats;
    MoveGenParams m_params;
};

} // namespace tusk
