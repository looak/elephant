// ElephantTuner, the offline evaluation tuner, see tuner/CMakeLists.txt.
//
//   ElephantTuner <dataset> [--limit N] [--threads N] [--leave N]
//
//   dataset     a text file, one position per line: a FEN and the game's result, see dataset.hpp
//   --limit     only the first N positions
//   --threads   threads to use, every logical core by default
//   --leave     cores to keep free, taken off --threads
//
// Phase 0: every position resolved to a quiet one with quiescence search, traced and kept as an entry.
// Phase 1: K, the scale between the evaluation and the game's result, and the current evaluation's error at it.
// Then fitting the values.

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstring>
#include <exception>
#include <format>
#include <iostream>
#include <string>
#include <thread>

#include <dataset.hpp>
#include <find_k.hpp>
#include <system/static_initializer.hpp>
#include <tuner_data.hpp>

bool g_initialized = static_initializer::initialize();

namespace {

int usage() {
    std::cerr << "usage: ElephantTuner <dataset> [--limit N] [--threads N] [--leave N]\n"
                 "  dataset:   one position per line, a FEN and the game's result from white's side,\n"
                 "             e.g. [1.0], c9 \"1/2-1/2\"; or a bare 0-1\n"
                 "  --limit:   only the first N positions\n"
                 "  --threads: threads to use, every logical core by default\n"
                 "  --leave:   cores to keep free, taken off --threads\n";
    return 2;
}

bool parseCount(const char* text, std::size_t& out) {
    const auto [end, error] = std::from_chars(text, text + std::strlen(text), out);
    return error == std::errc{} && *end == '\0';
}

} // namespace

int main(int argc, char* argv[]) {
    std::string path;
    std::size_t limit = 0;
    std::size_t threads = std::max(1u, std::thread::hardware_concurrency());
    std::size_t leave = 0;
    for (int i = 1; i < argc; ++i) {
        const bool hasValue = i + 1 < argc;
        if (std::strcmp(argv[i], "--limit") == 0 && hasValue) {
            if (!parseCount(argv[++i], limit))
                return usage();
        }
        else if (std::strcmp(argv[i], "--threads") == 0 && hasValue) {
            if (!parseCount(argv[++i], threads) || threads == 0)
                return usage();
        }
        else if (std::strcmp(argv[i], "--leave") == 0 && hasValue) {
            if (!parseCount(argv[++i], leave))
                return usage();
        }
        else if (path.empty() && argv[i][0] != '-') {
            path = argv[i];
        }
        else {
            return usage();
        }
    }
    if (path.empty())
        return usage();
    threads = threads > leave ? threads - leave : 1;

    tuner::LoadReport load;
    tuner::PrepareReport report;
    tuner::TunerData data;
    try {
        data = tuner::prepare(path, limit, threads, load, report);
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    std::size_t whiteWins = 0, draws = 0, blackWins = 0;
    for (const tuner::TunerEntry& entry : data.entries) {
        if (entry.result == 1.0f)
            ++whiteWins;
        else if (entry.result == 0.5f)
            ++draws;
        else
            ++blackWins;
    }

    const double perLine = report.resolved == 0 ? 0.0 : static_cast<double>(report.lineMoves) / static_cast<double>(report.resolved);
    std::cout << std::format("{}: {} lines read, {} positions with a valid FEN\n", path, load.lines, report.positions);
    std::cout << std::format("  not used     {} empty or comment, {} without a result, {} invalid FEN, {} decided (mate)\n",
                             load.skipped, load.noResult, load.badFen, report.decided);
    for (const tuner::LoadError& error : load.firstErrors)
        std::cout << std::format("    line {}: {}\n", error.line, error.reason);
    std::cout << std::format("  quiescence   {} already quiet, {} resolved ({:.2f} moves on average, longest {})\n",
                             data.entries.size() - report.resolved, report.resolved, perLine, report.longestLine);
    std::cout << std::format("  entries      {}  white {}  draw {}  black {}\n", data.entries.size(), whiteWins, draws,
                             blackWins);
    std::cout << std::format("               {} coefficients, {:.1f} MB, {} with unfitted terms (mop-up), checksum {:016x}\n",
                             data.coefficients.size(), static_cast<double>(data.bytes()) / (1024.0 * 1024.0),
                             report.withOffset, data.checksum());
    std::cout << std::format("  checks       {} traced sums differ from the incremental ones, {} rebuilt scores differ "
                             "from the engine's\n",
                             report.accumulatorMismatch, report.rebuildMismatch);
    std::cout << std::format("  time         {:.1f} s on {} threads, {:.0f} positions/s\n", report.seconds,
                             report.threads, static_cast<double>(report.positions) / std::max(report.seconds, 1e-9));

    if (data.entries.empty())
        return 1;
    if (report.accumulatorMismatch != 0 || report.rebuildMismatch != 0)
        return 3;

    // --- Phase 1: K, see find_k.hpp ---
    const auto kStart = std::chrono::steady_clock::now();
    const tuner::KSearch search = tuner::findK(data, report.threads);
    const double kSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - kStart).count();

    std::cout << "\nPhase 1, K: the scale that turns an evaluation into an expected result\n";

    // the first 31 tries are the coarse pass, K = 0.0 to 3.0, they show the error's valley.
    std::cout << "  error by K (coarse pass)\n";
    for (std::size_t i = 0; i < search.tried.size() && i < 31; i += 3) {
        const auto [k, error] = search.tried[i];
        std::cout << std::format("    K {:.1f}  error {:.6f}\n", k, error);
    }

    // a prediction that knows nothing says 0.5 for every position, its error is the bar any evaluation has to clear.
    double knowNothing = 0.0;
    for (const tuner::TunerEntry& entry : data.entries)
        knowNothing += (entry.result - 0.5) * (entry.result - 0.5);
    knowNothing /= static_cast<double>(data.entries.size());

    std::cout << std::format("  best K       {:.4f}, {} errors computed in {:.1f} s\n", search.k, search.tried.size(),
                             kSeconds);
    std::cout << std::format("  error        {:.6f} with the current evaluation, the baseline tuning has to beat\n",
                             search.error);
    std::cout << std::format("               {:.6f} predicting 0.5 for every position, knowing nothing\n", knowNothing);
    std::cout << std::format("  meaning      at this K white is expected to score");
    for (int eval : { 50, 100, 200, 400 })
        std::cout << std::format("  {:.0f}% at +{}", 100.0 * tuner::expectedResult(eval, search.k), eval);
    std::cout << '\n';
    return 0;
}
