#include <search/quiescence_probe.hpp>

#include <cstdlib>

#include <core/chessboard.hpp>
#include <move/move_executor.hpp>
#include <search/core/search_constants.hpp>
#include <search/worker.hpp>
#include <search/state/thread_context.hpp>
#include <search/state/transposition_table.hpp>
#include <system/time_manager.hpp>

// What a ThreadContext asks for. Quiescence never touches the TT, the smallest table satisfies the context. The clock is
// never read either, stopEnabled is off.
struct QuiescenceProbe::Impl {
    Impl()
        : clock(SearchLimits{}, Set::WHITE)
        , tt(1)
        , context(request, clock, tt, worker.m_heuristics, worker.m_pawnHash)
    {
        context.stopEnabled = false;
    }

    SearchRequest request;
    TimeManager clock;
    TranspositionTable tt;
    Worker worker;
    ThreadContext context;
};

QuiescenceProbe::QuiescenceProbe() : m_impl(std::make_unique<Impl>()) {}
QuiescenceProbe::~QuiescenceProbe() = default;

QuiescenceResult QuiescenceProbe::search(const Chessboard& board)
{
    ThreadContext& context = m_impl->context;
    context.position = board.readPosition().copy();
    context.accumulator = PestoAccumulator::computeFromScratch(context.position.read().material());

    QuiescenceResult result;
    result.score = board.readToPlay() == Set::WHITE ? Worker::quiescenceLine<Set::WHITE>(context, result.line)
                                                    : Worker::quiescenceLine<Set::BLACK>(context, result.line);
    result.decided = std::abs(result.score) >= c_checkmateMinScore;
    return result;
}

void QuiescenceProbe::playLine(const PVLine& line)
{
    // the undo units are dropped, and the fifty move counter isn't the probe's to keep: search starts over from a board.
    u16 fiftyMoveCounter = 0;
    for (u32 i = 0; i < line.length; ++i) {
        MoveUndoUnit undo;
        m_impl->context.makeExecutor().makeMove(line.moves[i], undo, fiftyMoveCounter);
    }
}

PositionReader QuiescenceProbe::position() const
{
    return m_impl->context.position.read();
}

const PestoAccumulator& QuiescenceProbe::accumulator() const
{
    return m_impl->context.accumulator;
}
