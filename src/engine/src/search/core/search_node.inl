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

#include <search/core/search_node.hpp>

#include <move/generation/tusk/check_info.hpp>
#include <search/core/search_constants.hpp>
#include <search/search_results.hpp>
#include <search/state/thread_context.hpp>

template<Set us, NodeKind kind>
StageResult<GateNode<us, kind>> OpenNode<us, kind>::gate() {
    ThreadContext& context = env.context;
    const PlyFrame& frame = context.frame(inputs.ply);
    PositionReader pos = context.position.read();

    // PVS searches everything off the principal variation with a null window, so a wider window means a PV node.
    // The root is always a PV node. Pruning and TT cutoffs are only safe at non-PV nodes.
    const bool isPv = inputs.beta - inputs.alpha > 1;

    // The incoming line's first move is searched first, iterative deepening hands the root its previous best move this
    // way. The line is cleared so every return below leaves an empty PV unless a move raises alpha.
    const PackedMove pvHint = env.pv->length > 0 ? env.pv->moves[0] : PackedMove::NullMove();
    env.pv->length = 0;

    // --- Draws ---
    // threefold repetition, the root & the game's earlier positions included, and the fifty move rule. Never at the
    // root, a search started in a drawn position still has to return a move. A mate given on the hundredth half move
    // would win but is scored a draw, rare enough not to generate moves for.
    if constexpr (kind == NodeKind::NORMAL) {
        if (env.context.repetitions.isRepetition(pos.hash(), frame.halfmoveClock) || frame.halfmoveClock >= 100)
            return Cut{ -c_drawConstant };
    }

    // hard ply limit, frames & pv are sized by it and asserts are compiled out. Check extensions can otherwise keep
    // depth from ever reaching 0 along a chain of checks.
    if (inputs.ply >= c_maxSearchDepth - 1)
        return Cut{ context.evaluate<us>() };


    // --- Check Extension ---
    // Search one ply deeper when in check so forcing sequences aren't cut off at the horizon. Covers direct and
    // discovered checks alike. Done before the TT probe so stored and probed depths agree.
    if (env.checkInfo.isChecked())
        inputs.depth++;

    // --- Transposition Table Probe ---
    std::optional<search_policies::TT::ProbeResult> probeResult = std::nullopt;
    if constexpr (search_policies::TT::enabled) {
        probeResult = search_policies::TT::probe(context.tt, pos.hash(), inputs.ply);

        if (probeResult.has_value()) {

            // No cutoffs at PV nodes or at validation search.
            if (!isPv
                && frame.excludedMove.isNull()
                && search_policies::TT::cuts(probeResult.value(), inputs.depth, inputs.alpha, inputs.beta)) {
                return Cut{ probeResult.value().score };
            }
        }
    }

    // --- Internal Iterative Reduction ---
    // Without a TT or PV move the ordering is a guess and the node is likely new to the search, a ply shallower is
    // cheaper and stores a move for the next visit.
    if constexpr (search_policies::enabled_policies::IIR) {
        const PackedMove ttMove = probeResult.has_value() ? probeResult->move : PackedMove::NullMove();
        if (inputs.depth >= iir_params::minDepth
            && ttMove.isNull()
            && pvHint.isNull())
            inputs.depth--;
    }

    return StageResult<GateNode<us, kind>>::carryOn(GateNode<us, kind>{ env, inputs, pvHint, isPv, probeResult });
}