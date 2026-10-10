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
 * @file move_gen_result.hpp
 * @brief MoveGenResult, the mutable, lazily filled output of a tusk::MoveGenerator.
 *
 * Owns the move buffer and all iteration state. Moves are generated stage by stage as next() runs out of moves,
 * so a node that cuts off on the first capture never generates quiets.
 *
 * Lifetime: a result keeps a pointer to the generator that created it, the generator must outlive it. Not copyable,
 * use it where generate() returns it (guaranteed copy elision).
 *
 * @author Alexander Loodin Ek    */

#pragma once

#include <array>
#include <span>
#include <type_traits>

#include <material/chess_piece_defines.hpp>
#include <move/move.hpp>
#include <move/generation/tusk/move_gen_params.hpp>

namespace tusk {

// Move & ordering score as stored in a MoveGenResult. Kept trivial so the 256 entry buffer is left uninitialized,
// only entries that are pushed are ever written.
struct ScoredMove {
    static constexpr u16 priorityMask = 0x7FFF;
    static constexpr u16 checkFlag = 0x8000;

    PackedMove move;
    u16 score;  // [check x1][priority x15]

    [[nodiscard]] static ScoredMove make(PackedMove move, u16 priority, bool check = false) {
        return { move, static_cast<u16>((priority & priorityMask) | (check ? checkFlag : 0)) };
    }

    [[nodiscard]] u16 priority() const { return score & priorityMask; }
    [[nodiscard]] bool isCheck() const { return (score & checkFlag) != 0; }

    [[nodiscard]] PrioritizedMove toPrioritized() const {
        PrioritizedMove result(move, priority());
        result.setCheck(isCheck());
        return result;
    }
};

static_assert(std::is_trivial_v<ScoredMove>, "ScoredMove must stay trivial, MoveGenResult relies on it being uninitialized");
static_assert(sizeof(ScoredMove) == 4, "ScoredMove is not 4 bytes");

template<Set us>
class MoveGenerator;

template<Set us>
class MoveGenResult {
public:
    // Stage::UNORDERED generates every legal move up front, any other stage generates lazily from next().
    explicit MoveGenResult(const MoveGenerator<us>& generator, Stage startStage);

    MoveGenResult(const MoveGenResult&) = delete;
    MoveGenResult& operator=(const MoveGenResult&) = delete;
    MoveGenResult(MoveGenResult&&) = delete;
    MoveGenResult& operator=(MoveGenResult&&) = delete;

    // Next move in priority order, generating the next stage when the current one is exhausted.
    // Returns a null move when there are no more moves.
    [[nodiscard]] PrioritizedMove next();

    // Same move next() would return, without consuming it. Not const, may trigger generation.
    [[nodiscard]] PackedMove peek();

    // Moves already handed out by next(), e.g. quiets searched before a cutoff for history updates.
    [[nodiscard]] std::span<const ScoredMove> searched() const {
        return { m_moves.data(), m_current };
    }

    // Number of moves generated so far, not necessarily all legal moves unless the result came from generateAll().
    [[nodiscard]] u32 generatedCount() const { return m_end; }

    [[nodiscard]] Stage stage() const { return m_stage; }

private:
    friend class MoveGenerator<us>;

    // used by the generator to append to the current batch.
    void push(ScoredMove move) { m_moves[m_end++] = move; }

    // swaps the highest priority move in [m_current, m_end) into m_current.
    void pickBest();

    std::array<ScoredMove, 256> m_moves;  // intentionally uninitialized, see ScoredMove
    // per piece type, squares giving direct check. Computed by the first scored stage and reused by the next, the
    // position is the same for every stage of a result.
    std::array<u64, 6> m_checkSquares;
    bool m_checkSquaresReady = false;
    const MoveGenerator<us>* m_generator;
    u32 m_current = 0;
    u32 m_end = 0;
    Stage m_stage;
};

} // namespace tusk
