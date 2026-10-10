#include <search/perft_search.hpp>

PerftSearch::PerftSearch(GameContext& context)
    : m_context(context)
{

}

template<typename TMoveGen>
PerftResult PerftSearch::Run(int depth)
{
    if (depth <= 0) {
        return PerftResult();
    }

    auto accumulator = [&](PackedMove move, PerftResult& result, bool leaf) {
        result.Nodes += static_cast<u64>(leaf);
        result.AccNodes++;

        if (move.isCapture()) {
            result.Captures++;
        } else if (move.isEnPassant()) {
            result.EnPassants++;
        } else if (move.isCastling()) {
            result.Castles++;
        } else if (move.isPromotion()) {
            result.Promotions++;
        }
        // else if (move.isCheck()) {
        //     result.Checks++;
        // } else if (move.isCheckmate()) {
        //     result.Checkmates++;
        // }

        // Additional counting logic can be added here
    };


    return internalRunEntryPoint<TMoveGen, PerftResult>(depth, accumulator);
}

template PerftResult PerftSearch::Run<move_gen_policy::Legacy>(int);
template PerftResult PerftSearch::Run<move_gen_policy::Tusk>(int);

PerftResult PerftSearch::Deepen()
{
    // MoveGenerator<Set::WHITE> gen(m_context.readChessboard().readPosition());
    // gen.GenerateMoves();

    return PerftResult();
}

template<typename TMoveGen>
std::vector<DivideResult> PerftSearch::Divide(int depth)
{
    if (depth <= 0) {
        return std::vector<DivideResult>();
    }

    std::vector<DivideResult> results;

    typedef std::function<void(PackedMove, DivideResult::inner&, bool)> t_accFunction;

    t_accFunction accumulator = [](PackedMove, DivideResult::inner& result, bool leaf) {
        result.Nodes += (u64)leaf;
        result.AccNodes++;
    };

    auto forEachMoveLambda = [&results, depth, &accumulator, this] (PackedMove move) {
        m_context.MakeMove<true>(move);
        auto inner = internalRunEntryPoint<TMoveGen, DivideResult::inner, t_accFunction>(depth - 1, accumulator);
        inner.Nodes = inner.Nodes == 0 ? 1 : inner.Nodes;
        m_context.UnmakeMove();

        results.emplace_back(move, inner);
    };

    const PositionReader position = m_context.readChessboard().readPosition();
    if (m_context.readToPlay() == Set::WHITE) {
        TMoveGen::template forEachMove<Set::WHITE>(position, forEachMoveLambda);
    } else {
        TMoveGen::template forEachMove<Set::BLACK>(position, forEachMoveLambda);
    }

    return results;
}

template std::vector<DivideResult> PerftSearch::Divide<move_gen_policy::Legacy>(int);
template std::vector<DivideResult> PerftSearch::Divide<move_gen_policy::Tusk>(int);
