#include <search/perft_search.hpp>

PerftSearch::PerftSearch(GameContext& context)
    : m_context(context)
{

}

template<typename TMoveGen>
PerftResult PerftSearch::Run(int depth)
{
    return { internalRunEntryPoint<TMoveGen>(depth) };
}

template PerftResult PerftSearch::Run<move_gen_policy::Tusk>(int);

PerftResult PerftSearch::Deepen()
{

    return PerftResult();
}

template<typename TMoveGen>
std::vector<DivideResult> PerftSearch::Divide(int depth)
{
    if (depth <= 0) {
        return std::vector<DivideResult>();
    }

    std::vector<DivideResult> results;

    auto forEachMoveLambda = [&results, depth, this] (PackedMove move) {
        // at depth 1 the move itself is the one node. Deeper, a move into mate or stalemate has none.
        m_context.MakeMove(move);
        const u64 nodes = depth == 1 ? 1 : internalRunEntryPoint<TMoveGen>(depth - 1);
        m_context.UnmakeMove();

        results.push_back({ move, nodes });
    };

    const PositionReader position = m_context.readChessboard().readPosition();
    if (m_context.readToPlay() == Set::WHITE) {
        TMoveGen::template forEachMove<Set::WHITE>(position, forEachMoveLambda);
    } else {
        TMoveGen::template forEachMove<Set::BLACK>(position, forEachMoveLambda);
    }

    return results;
}

template std::vector<DivideResult> PerftSearch::Divide<move_gen_policy::Tusk>(int);
