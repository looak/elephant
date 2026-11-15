#include <search/worker.hpp>

#include <position/hash_zobrist.hpp>

#include <search/state/thread_context.hpp>

// Null Move Pruning: pass the move and search the opponent's reply with a reduced, null window search. If we still
// fail high after giving the opponent a free move, the position is good enough that a full search would too.
// Called at non-PV nodes that aren't in check, staticEval is the node's eval from alphaBeta.
template<Set us>
std::optional<i16> Worker::tryNullMovePrune(ThreadContext& ctx, NodeInputs node, i16 staticEval) {
    // non-PV, so the window is beta - 1 to beta and the child's scout window is the null window around beta.
    ASSERT_MSG(node.beta - node.alpha == 1, "Null move pruning at a PV node.");
    const u8 depth = node.depth;
    const i16 beta = node.beta;
    const u16 ply = node.ply;

    // never two null moves in a row, the second would just hand the move back.
    if (ctx.frame(ply - 1).nullMove)
        return std::nullopt;

    // inside a verification search the verified side doesn't null move until nmpMinPly.
    constexpr bool white = us == Set::WHITE;
    if (ply < ctx.nmpMinPly && ctx.nmpColorWhite == white)
        return std::nullopt;

    // near mate a free move can hide the mating line.
    if (beta >= c_checkmateMinScore || beta <= -c_checkmateMinScore)
        return std::nullopt;

    // with only king and pawns zugzwang is common and passing would be better than any real move.
    PositionReader pos = ctx.position.read();
    const auto& mat = pos.material();
    Bitboard pieces = mat.knights<us>() | mat.bishops<us>() | mat.rooks<us>() | mat.queens<us>();
    if (!search_policies::NMP::shouldPrune(depth, false, !pieces.empty()))
        return std::nullopt;

    // only worth trying when we're already at or above beta.
    if (staticEval < beta)
        return std::nullopt;

    // --- make null move ---
    // flip the side to move, an en passant capture isn't available to the opponent anymore.
    auto editor = ctx.position.edit();
    const u64 originalHash = pos.hash();
    const byte originalEnPassant = editor.enPassant().read();
    editor.enPassant().clear();
    editor.hash() = zobrist::updateBlackToMoveHash(editor.hash());
    ctx.repetitions.push(pos.hash());
    ctx.frame(ply).nullMove = true;

    if constexpr (search_policies::ContinuationHistoryPolicy::enabled)
        ctx.frame(ply).continuation = &heuristic_defaults::noPreviousMove;

    // passing counts as a quiet move for the fifty move rule.
    ctx.frame(ply + 1).halfmoveClock = static_cast<u16>(ctx.frame(ply).halfmoveClock + 1);

    const u8 R = search_policies::NMP::getReduction(depth, staticEval, beta);
    const u8 nullDepth = depth > R + 1 ? static_cast<u8>(depth - 1 - R) : 0;
    PVLine nullPv;
    i16 nullScore = static_cast<i16>(-alphaBeta<opposing_set<us>()>(ctx, node.makeScoutInputs(nullDepth), &nullPv));

    // --- unmake null move ---
    ctx.frame(ply).nullMove = false;
    ctx.repetitions.pop();
    if (originalEnPassant)
        editor.enPassant().write(originalEnPassant);
    editor.hash() = originalHash;
    ctx.nodeCount++;

    if (ctx.stopped || nullScore < beta)
        return std::nullopt;

    // a mate found after a free move isn't proven, cut with beta instead.
    const i16 cutScore = nullScore >= c_checkmateMinScore ? beta : nullScore;

    // shallow fail highs are trusted, as are those inside another verification.
    if (!search_policies::NMP::shouldVerify(depth) || ctx.nmpMinPly != 0)
        return cutScore;

    // --- verification ---
    // Search the same node at the null move's depth without null moves for us, so a zugzwang where passing is the
    // best "move" has to show up as a real fail high.
    ctx.nmpMinPly = static_cast<u16>(ply + 3 * nullDepth / 4);
    ctx.nmpColorWhite = white;
    PVLine verifyPv;
    i16 verifyScore = alphaBeta<us>(ctx, NodeInputs{ static_cast<i16>(beta - 1), beta, nullDepth, ply }, &verifyPv);
    ctx.nmpMinPly = 0;

    if (ctx.stopped || verifyScore < beta)
        return std::nullopt;
    return cutScore;
}

template std::optional<i16> Worker::tryNullMovePrune<Set::WHITE>(ThreadContext& ctx, NodeInputs node, i16 staticEval);
template std::optional<i16> Worker::tryNullMovePrune<Set::BLACK>(ThreadContext& ctx, NodeInputs node, i16 staticEval);
