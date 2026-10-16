#include <tuner_data.hpp>

#include <algorithm>
#include <chrono>
#include <exception>
#include <iostream>

#include <core/chessboard.hpp>
#include <eval/evaluator.hpp>
#include <eval/pesto_accumulator.hpp>
#include <io/fen_parser.hpp>
#include <quiet.hpp>
#include <search/search.hpp>
#include <search/search_threadcontext.hpp>
#include <system/time_manager.hpp>

namespace tuner {

TunerData prepare(const std::string& path, std::size_t limit, LoadReport& load, PrepareReport& report) {
    const auto start = std::chrono::steady_clock::now();
    const std::vector<TaperedScore> current = params::current();

    // one context for every position, reset to each in turn. Never stopped, quiescence runs to its own end.
    const SearchParameters noLimits{};
    const TimeManager clock(noLimits, Set::WHITE);
    ThreadSearchContext context(Position{}, true, clock);
    context.stopEnabled = false;

    TunerData data;
    if (limit != 0) {
        data.entries.reserve(limit);
        data.coefficients.reserve(limit * 16);  // about 15 per position, mirrored pieces cancel
    }
    PestoTrace trace;

    forEachPosition(path, load, limit, [&](const TuningPosition& position, std::size_t) {
        Chessboard board;
        try {
            if (!io::fen_parser::deserialize(position.fen, board))
                return false;
        }
        catch (const std::exception&) {
            return false;
        }
        ++report.positions;

        context.position = board.readPosition().copy();
        context.accumulator = PestoAccumulator::computeFromScratch(context.position.read().material());
        context.gameState.whiteToMove = board.readToPlay() == Set::WHITE;

        bool decided = false;
        const QuietLine line = resolveToQuiet(context, decided);
        if (decided) {
            ++report.decided;
            return true;
        }
        if (line.length > 0) {
            ++report.resolved;
            report.lineMoves += line.length;
            report.longestLine = std::max<std::size_t>(report.longestLine, line.length);
        }

        // the quiet position's terms, traced from scratch, and the incremental sums the line's moves left behind.
        trace.clear();
        const PestoAccumulator traced = PestoAccumulator::computeFromScratch(context.position.read().material(), trace);
        if (!(traced == context.accumulator))
            ++report.accumulatorMismatch;

        Evaluator evaluator(context.position.read(), traced);
        const i32 staticEval = evaluator.Evaluate();
        const i32 pesto = traced.taperedScore();

        TunerEntry entry{};
        entry.result = static_cast<float>(position.result);
        entry.staticEval = static_cast<i16>(staticEval);
        entry.fixedOffset = static_cast<i16>(staticEval - pesto);
        entry.phase = static_cast<u8>(std::min(traced.phase, evaluator_data::maxGamePhase));
        entry.coefficientStart = static_cast<u32>(data.coefficients.size());
        for (const Coefficient& c : trace.coefficients())
            data.coefficients.push_back(c);
        entry.coefficientCount = static_cast<u8>(data.coefficients.size() - entry.coefficientStart);

        if (entry.fixedOffset != 0)
            ++report.withOffset;
        if (data.fittedScore(entry, current) != pesto)
            ++report.rebuildMismatch;

        data.entries.push_back(entry);
        if (report.positions % 500000 == 0)
            std::cerr << "\r  " << report.positions << " positions" << std::flush;
        return true;
    });
    std::cerr << "\r";
    // vectors grow by doubling, give back what wasn't used.
    data.entries.shrink_to_fit();
    data.coefficients.shrink_to_fit();

    report.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    return data;
}

} // namespace tuner
