#include <search/perft_search.hpp>
#include <core/game_context.hpp>
#include <move/generation/move_generator.hpp>
#include <move/move_executor.hpp>

PerftSearch::PerftSearch(GameContext& context)
    : m_context(context), m_depth(0)
{

}

template<Set us>
PerftResult PerftSearch::Run(int depth)
{
    if (depth <= 0) {
        return PerftResult();
    }
    PerftResult result;
    MoveGenParams params;

    // perft tests should always run from white's perspective
    MoveGenerator<us> gen(m_context.readChessboard().readPosition(), params);
    MoveExecutor exec(m_context);

    while (PrioritizedMove prioritized = gen.generateNextMove()) {

        count(prioritized.move, result);

        exec.makeMove<true>(prioritized.move);
        result += Run<opposing_set<us>()>(depth - 1);
        exec.unmakeMove();
    }

    return result;
}

template PerftResult PerftSearch::Run<Set::WHITE>(int);
template PerftResult PerftSearch::Run<Set::BLACK>(int);

PerftResult PerftSearch::Deepen()
{
    // MoveGenerator<Set::WHITE> gen(m_context.readChessboard().readPosition());
    // gen.GenerateMoves();

    return PerftResult();
}


std::vector<u32> PerftSearch::Divide(int atDepth)
{
    return std::vector<u32>();
}

void PerftSearch::count(PackedMove move, PerftResult& result) {
    // Count the move in the perft result
    result.Nodes++;

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
}