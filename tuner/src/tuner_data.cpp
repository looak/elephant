#include <tuner_data.hpp>

#include <algorithm>
#include <atomic>
#include <bit>
#include <chrono>
#include <exception>
#include <iostream>
#include <thread>

#include <core/chessboard.hpp>
#include <eval/evaluator.hpp>
#include <eval/mobility.hpp>
#include <eval/pawn_structure.hpp>
#include <eval/pesto_accumulator.hpp>
#include <io/fen_parser.hpp>
#include <quiet.hpp>
#include <search/search.hpp>
#include <search/search_threadcontext.hpp>
#include <system/time_manager.hpp>

namespace tuner {

void TunerData::append(TunerData&& next) {
    const u32 shift = static_cast<u32>(coefficients.size());
    entries.reserve(entries.size() + next.entries.size());
    for (TunerEntry entry : next.entries) {
        entry.coefficientStart += shift;
        entries.push_back(entry);
    }
    coefficients.insert(coefficients.end(), next.coefficients.begin(), next.coefficients.end());
    next = TunerData{};
}

std::uint64_t TunerData::checksum() const {
    // FNV-1a over the fields, not the bytes, padding isn't part of the data.
    std::uint64_t hash = 14695981039346656037ull;
    auto mix = [&hash](std::uint64_t value) {
        for (int i = 0; i < 8; ++i) {
            hash ^= (value >> (i * 8)) & 0xff;
            hash *= 1099511628211ull;
        }
    };
    for (const TunerEntry& entry : entries) {
        mix(std::bit_cast<u32>(entry.result));
        mix(static_cast<u16>(entry.staticEval));
        mix(static_cast<u16>(entry.fixedOffset));
        mix(entry.coefficientStart);
        mix(entry.coefficientCount);
        mix(entry.phase);
    }
    for (const Coefficient& c : coefficients) {
        mix(c.index());
        mix(static_cast<u32>(c.value()));
    }
    return hash;
}

SplitData splitHeldBack(TunerData&& data, std::size_t everyNth) {
    SplitData split;
    if (everyNth == 0) {
        split.fit = std::move(data);
        return split;
    }

    // copies each entry & its coefficients to one side or the other, coefficient starts point into the new side.
    auto copyEntry = [&data](TunerData& to, const TunerEntry& entry) {
        TunerEntry copy = entry;
        copy.coefficientStart = static_cast<u32>(to.coefficients.size());
        to.coefficients.insert(to.coefficients.end(), data.coefficients.begin() + entry.coefficientStart,
                               data.coefficients.begin() + entry.coefficientStart + entry.coefficientCount);
        to.entries.push_back(copy);
    };

    const std::size_t heldBack = (data.entries.size() + everyNth - 1) / everyNth;
    split.heldBack.entries.reserve(heldBack);
    split.fit.entries.reserve(data.entries.size() - heldBack);
    // a little over each side's share, coefficients per position vary.
    split.heldBack.coefficients.reserve(data.coefficients.size() / everyNth + data.coefficients.size() / 100);
    split.fit.coefficients.reserve(data.coefficients.size());
    for (std::size_t i = 0; i < data.entries.size(); ++i)
        copyEntry(i % everyNth == 0 ? split.heldBack : split.fit, data.entries[i]);

    data = TunerData{};
    return split;
}

void PrepareReport::append(const PrepareReport& next) {
    positions += next.positions;
    decided += next.decided;
    resolved += next.resolved;
    lineMoves += next.lineMoves;
    longestLine = std::max(longestLine, next.longestLine);
    withOffset += next.withOffset;
    accumulatorMismatch += next.accumulatorMismatch;
    rebuildMismatch += next.rebuildMismatch;
}

namespace {

// what a thread produces from its range, joined with the others' once all are done.
struct Part {
    TunerData data;
    LoadReport load;
    PrepareReport report;
    std::exception_ptr error;
};

// the work of one thread: every position in range, with its own context & trace.
void prepareRange(const std::string& path, ByteRange range, Part& part, std::atomic<std::size_t>& progress) {
    const std::vector<TaperedScore> current = params::current();

    // one context for every position in range, reset to each in turn. Never stopped, quiescence runs to its own end.
    const SearchParameters noLimits{};
    const TimeManager clock(noLimits, Set::WHITE);
    ThreadSearchContext context(Position{}, true, clock);
    context.stopEnabled = false;

    TunerData& data = part.data;
    PrepareReport& report = part.report;
    // about 65 bytes a line and 15 coefficients a position in the datasets seen so far.
    const std::size_t expected = static_cast<std::size_t>((range.end - range.begin) / 60);
    data.entries.reserve(expected);
    data.coefficients.reserve(expected * 16);

    EvalTrace trace;
    std::size_t unreported = 0;
    forEachPosition(path, range, part.load, [&](const TuningPosition& position) {
        Chessboard board;
        try {
            if (!io::fen_parser::deserialize(position.fen, board))
                return false;
        }
        catch (const std::exception&) {
            return false;
        }
        ++report.positions;
        if (++unreported == 4096) {
            progress.fetch_add(unreported, std::memory_order_relaxed);
            unreported = 0;
        }

        context.position = board.readPosition().copy();
        context.accumulator = PestoAccumulator::computeFromScratch(context.position.read().material());
        context.gameState.whiteToMove = board.readToPlay() == Set::WHITE;

        bool decided = false;
        const PVLine line = resolveToQuiet(context, decided);
        if (decided) {
            ++report.decided;
            return true;
        }
        if (line.length > 0) {
            ++report.resolved;
            report.lineMoves += line.length;
            report.longestLine = std::max<std::size_t>(report.longestLine, line.length);
        }

        // the quiet position's terms, traced from scratch: PeSTO's, compared with the incremental sums the line's moves
        // left behind, mobility's and pawn structure's.
        trace.clear();
        const PestoAccumulator traced = PestoAccumulator::computeFromScratch(context.position.read().material(), trace);
        if (!(traced == context.accumulator))
            ++report.accumulatorMismatch;
        (void)mobilityScore(context.position.read().material(), trace);
        (void)pawnScore(context.position.read().material(), trace);

        // the engine's own numbers: the whole static evaluation, and the part made of the terms that are fitted.
        Evaluator evaluator(context.position.read(), traced);
        const i32 staticEval = evaluator.Evaluate();
        const i32 tapered = evaluator.EvaluateTapered();

        TunerEntry entry{};
        entry.result = static_cast<float>(position.result);
        entry.staticEval = static_cast<i16>(staticEval);
        entry.fixedOffset = static_cast<i16>(staticEval - tapered);
        entry.phase = static_cast<u8>(std::min(traced.phase, evaluator_data::maxGamePhase));
        entry.coefficientStart = static_cast<u32>(data.coefficients.size());
        for (const Coefficient& c : trace.coefficients())
            data.coefficients.push_back(c);
        entry.coefficientCount = static_cast<u8>(data.coefficients.size() - entry.coefficientStart);

        if (entry.fixedOffset != 0)
            ++report.withOffset;
        if (data.fittedScore(entry, current) != tapered)
            ++report.rebuildMismatch;

        data.entries.push_back(entry);
        return true;
    });
    progress.fetch_add(unreported, std::memory_order_relaxed);
}

} // namespace

TunerData prepare(const std::string& path, std::size_t limit, std::size_t threads, LoadReport& load,
                  PrepareReport& report) {
    const auto start = std::chrono::steady_clock::now();
    const std::vector<ByteRange> ranges = splitDataset(path, std::max<std::size_t>(threads, 1), limit);

    std::vector<Part> parts(ranges.size());
    std::atomic<std::size_t> progress{ 0 };
    std::atomic<std::size_t> finished{ 0 };
    {
        std::vector<std::jthread> workers;
        workers.reserve(ranges.size());
        for (std::size_t i = 0; i < ranges.size(); ++i) {
            workers.emplace_back([&, i] {
                try {
                    prepareRange(path, ranges[i], parts[i], progress);
                }
                catch (...) {
                    parts[i].error = std::current_exception();
                }
                finished.fetch_add(1, std::memory_order_release);
            });
        }
        while (finished.load(std::memory_order_acquire) < ranges.size()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            std::cerr << "\r  " << progress.load(std::memory_order_relaxed) << " positions" << std::flush;
        }
    }   // joins the workers
    std::cerr << "\r                                        \r";

    std::size_t entries = 0, coefficients = 0;
    for (const Part& part : parts) {
        if (part.error)
            std::rethrow_exception(part.error);
        entries += part.data.entries.size();
        coefficients += part.data.coefficients.size();
    }

    // the first part is the base, the others are copied after it and freed one by one.
    TunerData data;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        load.append(parts[i].load);
        report.append(parts[i].report);
        if (i == 0) {
            data = std::move(parts[0].data);
            data.entries.reserve(entries);
            data.coefficients.reserve(coefficients);
        }
        else {
            data.append(std::move(parts[i].data));
        }
    }
    report.threads = ranges.size();
    report.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    return data;
}

} // namespace tuner
