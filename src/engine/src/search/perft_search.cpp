#include <search/perft_search.hpp>

PerftSearch::PerftSearch(GameContext& context)
    : m_context(context), m_depth(0)
{

}

PerftResult PerftSearch::Run(int depth)
{
    if (depth <= 0) {
        return PerftResult();
    }
    PerftResult result;
    typedef std::function<void(PackedMove, PerftResult&, bool)> t_accFunction;

    t_accFunction accumulator = [&](PackedMove move, PerftResult& result, bool leaf) {
        result.Nodes += (int)leaf;
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

    MoveGenParams params;

    return internalRunEntryPoint<PerftResult, t_accFunction>(depth, accumulator);    
}

PerftResult PerftSearch::Deepen()
{
    // MoveGenerator<Set::WHITE> gen(m_context.readChessboard().readPosition());
    // gen.GenerateMoves();

    return PerftResult();
}


std::vector<DivideResult> PerftSearch::Divide(int depth)
{
    if (depth <= 0) {
        return std::vector<DivideResult>();
    }

    std::vector<DivideResult> results;
    MoveGenParams params;

    typedef std::function<void(PackedMove, DivideResult::inner&, bool)> t_accFunction;

    t_accFunction accumulator = [](PackedMove, DivideResult::inner& result, bool leaf) {
        result.Nodes += (u64)leaf;
        result.AccNodes++;
    };

    auto forEachMoveLambda = [&results, depth, &accumulator, this] (PackedMove move) {        
        m_context.MakeMove<true>(move);
        auto inner = internalRunEntryPoint<DivideResult::inner, t_accFunction>(depth - 1, accumulator);
        inner.Nodes = inner.Nodes == 0 ? 1 : inner.Nodes;
        m_context.UnmakeMove();

        results.emplace_back(move, inner);
    };

    
    if (m_context.readToPlay() == Set::WHITE) {
        MoveGenerator<Set::WHITE> gen(m_context.readChessboard().readPosition(), params);
        while (PrioritizedMove prioritized = gen.generateNextMove()) {
            forEachMoveLambda(prioritized.move);
        }
    } else {
        MoveGenerator<Set::BLACK> gen(m_context.readChessboard().readPosition(), params);
        while (PrioritizedMove prioritized = gen.generateNextMove()) {
            forEachMoveLambda(prioritized.move);
        }
    }

    return results;
}
