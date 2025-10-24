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

template<Set us>
u64 PerftSearch::internalDivide(int depth)
{
    if (depth <= 0) 
        return 0;

    MoveGenParams params;
    MoveGenerator<us> gen(m_context.readChessboard().readPosition(), params);
    MoveExecutor exec(m_context);

    int count = 0;
    
    while (PrioritizedMove prioritized = gen.generateNextMove()) {
        count += 1;
        exec.makeMove<true>(prioritized.move);
        count += internalDivide<opposing_set<us>()>(depth - 1);
        exec.unmakeMove();
    }

    return count;
}

template<Set us>
std::vector<DivideResult> PerftSearch::Divide(int depth)
{
    if (depth <= 0) {
        return std::vector<DivideResult>();
    }

    std::vector<DivideResult> results;
    MoveGenParams params;

    MoveGenerator<us> gen(m_context.readChessboard().readPosition(), params);
    MoveExecutor exec(m_context);

    while (PrioritizedMove prioritized = gen.generateNextMove()) {
        

        exec.makeMove<true>(prioritized.move);
        u64 count = internalDivide<opposing_set<us>()>(depth - 1);
        count = count == 0 ? 1 : count;
        exec.unmakeMove();

        results.emplace_back(prioritized.move, count);
    }

    return results;
}

std::vector<DivideResult> PerftSearch::Divide(Set toPlay, int depth)
{
    if (toPlay == Set::WHITE) {
        return Divide<Set::WHITE>(depth);
    } else {
        return Divide<Set::BLACK>(depth);
    }
}

template std::vector<DivideResult> PerftSearch::Divide<Set::WHITE>(int);
template std::vector<DivideResult> PerftSearch::Divide<Set::BLACK>(int);

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