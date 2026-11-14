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
#include <search/worker.hpp>

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

template<Set us, NodeKind kind>
template<typename Moves>
StageResult<PlanNode<us, kind>> GateNode<us, kind>::examine(Worker& worker, Moves& moves) {
    NodePlan plan{};
    ThreadContext& context = env.context;
    PlyFrame& frame = context.frame(inputs.ply);
    const bool inCheck = env.checkInfo.isChecked();

    // a validation search (singular extension's, with a move excluded) must not cut on what it finds here.
    const bool notExcludedAllowCut = frame.excludedMove.isNull();

    // --- Singular Extension ---
    // the TT move with a deep enough lower bound: if every other move fails low against a window a margin below its
    // score, at about half depth, it's the only good move here and is searched a ply deeper.
    if constexpr (search_policies::SingularExtensions::enabled && kind == NodeKind::NORMAL) {
        if (probeResult.has_value() && notExcludedAllowCut && search_policies::SingularExtensions::tryNode(*probeResult, inputs.depth)) {
            const i16 singularBeta = static_cast<i16>(probeResult->score - singular_params::marginPerDepth * inputs.depth);
            const u8 singularDepth = static_cast<u8>((inputs.depth - 1) / 2);

            frame.excludedMove = probeResult->move;
            PVLine ignoredPv;
            const i16 score = worker.alphaBeta<us>(context, NodeInputs{ static_cast<i16>(singularBeta - 1), singularBeta, singularDepth, inputs.ply }, &ignoredPv);
            frame.excludedMove = PackedMove::NullMove();

            if (score < singularBeta)
                plan.singularExtension = 1;
        }
    }

    // --- prime move ordering ---
    plan.orderingView.ttMove = probeResult.has_value() ? probeResult->move : PackedMove::NullMove();
    plan.orderingView.pvMove = pvHint;
    search_policies::MoveOrdering::prime(context.heuristics, context.frames, plan.orderingView, inputs.ply);

    moves.beginGeneration(&plan.orderingView, MoveTypes::ALL);

    // --- Terminal Node ---
    if (moves.peek().isNull()) {
        if (inCheck) {
            ASSERT(inputs.ply < c_checkmateMaxDistance);
            return Cut{ static_cast<i16>(-c_checkmateConstant + inputs.ply) }; // Mate score adjusted by ply
        }
        return Cut{ -c_drawConstant }; // Stalemate
    }

    // --- Static Eval ---
    // Only the pruning at non-PV nodes uses it, and it means nothing in check.
    const i16 staticEval = (isPv || inCheck) ? -c_infinity : context.evaluate<us>();

    // --- Reverse Futility Pruning ---
    if constexpr (search_policies::RFP::enabled) {
        if (search_policies::RFP::shouldPrune(isPv, inCheck, inputs.depth, staticEval, inputs.beta) && notExcludedAllowCut)
            return Cut{ staticEval };
    }

    // --- Null Move Pruning ---
    if constexpr (search_policies::NMP::enabled) {
        if (!isPv && !inCheck && notExcludedAllowCut) {
            if (std::optional<i16> nullScore = worker.tryNullMovePrune<us>(context, inputs, staticEval))
                return Cut{ nullScore.value() };
        }
    }

    // --- Futility Pruning ---
    // decided per node, searchMoves skips the quiet moves.
    plan.futile = false;
    if constexpr (search_policies::Futility::enabled)
        plan.futile = search_policies::Futility::isNodeFutile(isPv, inCheck, inputs.depth, staticEval, inputs.alpha);

    return StageResult<PlanNode<us, kind>>::carryOn(PlanNode<us, kind>{ env, inputs, plan });
}