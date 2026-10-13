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
 * @file search_move_source.hpp
 * @brief SearchMoveSource, the one interface search uses to pull moves, backed by tusk::MoveGenerator.
 *
 * Usage:
 *     SearchMoveSource<us> moves(position);
 *     if (moves.isChecked()) ...                 // known before any generation
 *     moves.start(&ordering, MoveTypes::ALL);    // once, before next()/peek()
 *     while (PrioritizedMove m = moves.next()) { ... }
 *
 * Not copyable or movable, construct it where it's used.
 *
 * @author Alexander Loodin Ek    */

#pragma once

#include <optional>

#include <move/generation/tusk/check_info.hpp>
#include <move/generation/move_ordering_view.hpp>
#include <move/generation/tusk/move_generator.hpp>
#include <position/position_accessors.hpp>
#include <search/search_policies.hpp>

// tusk::MoveGenerator, staged & lazy. The generator and result are created in start() once the filter is known.
template<Set us>
class TuskSearchMoveSource {
public:
    explicit TuskSearchMoveSource(PositionReader position) :
        m_position(position),
        m_checkInfo(position)
    {}

    TuskSearchMoveSource(const TuskSearchMoveSource&) = delete;
    TuskSearchMoveSource& operator=(const TuskSearchMoveSource&) = delete;

    [[nodiscard]] bool isChecked() const { return m_checkInfo.isChecked(); }

    void start(const MoveOrderingView* ordering, MoveTypes filter) {
        m_generator.emplace(m_position, m_checkInfo, tusk::MoveGenParams{ .ordering = ordering, .moveFilter = filter,
            .deferLosingCaptures = search_policies::enabled_policies::DeferLosingCaptures });
        m_moves.emplace(*m_generator, tusk::Stage::PV_MOVE);
    }

    [[nodiscard]] PrioritizedMove next() { return m_moves->next(); }
    [[nodiscard]] PackedMove peek() { return m_moves->peek(); }

private:
    PositionReader m_position;
    tusk::CheckInfo<us> m_checkInfo;
    std::optional<tusk::MoveGenerator<us>> m_generator;
    std::optional<tusk::MoveGenResult<us>> m_moves;    // points at *m_generator, declared after it
};

// A class rather than an alias so search.hpp can forward declare it.
template<Set us>
class SearchMoveSource : public TuskSearchMoveSource<us> {
public:
    using TuskSearchMoveSource<us>::TuskSearchMoveSource;
};
