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
#include <search/search_results.hpp>
#include <search/state/thread_context.hpp>

template<Set us>
StageResult<GateNode<us>> OpenNode<us>::gate() {
    GateNode<us> gateNode{env, inputs, PackedMove::NullMove(), false, std::nullopt};
    
    // PVS searches everything off the principal variation with a null window, so a wider window means a PV node.
    // The root is always a PV node. Pruning and TT cutoffs are only safe at non-PV nodes.
    gateNode.isPv = inputs.beta - inputs.alpha > 1;

    // The incoming line's first move is searched first, iterative deepening hands the root its previous best move this
    // way. The line is cleared so every return below leaves an empty PV unless a move raises alpha.
    gateNode.pvHint = env.pv->length > 0 ? env.pv->moves[0] : PackedMove::NullMove();
    env.pv->length = 0;

    return StageResult<GateNode<us>>::carryOn(gateNode);
}

template StageResult<GateNode<Set::WHITE>> OpenNode<Set::WHITE>::gate();
template StageResult<GateNode<Set::BLACK>> OpenNode<Set::BLACK>::gate();

