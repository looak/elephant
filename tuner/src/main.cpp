// ElephantTuner, the offline evaluation tuner, see tuner/CMakeLists.txt.
//
//   ElephantTuner <dataset> [--limit N]
//
//   dataset   a text file, one position per line: a FEN and the game's result, see dataset.hpp
//   --limit   only the first N positions
//
// Phase 0 so far: every position resolved to a quiet one with quiescence search, traced and kept as an entry. Then
// finding K and fitting the values.

#include <charconv>
#include <cstring>
#include <exception>
#include <format>
#include <iostream>
#include <string>

#include <dataset.hpp>
#include <system/static_initializer.hpp>
#include <tuner_data.hpp>

bool g_initialized = static_initializer::initialize();

namespace {

int usage() {
    std::cerr << "usage: ElephantTuner <dataset> [--limit N]\n"
                 "  dataset: one position per line, a FEN and the game's result from white's side,\n"
                 "           e.g. [1.0], c9 \"1/2-1/2\"; or a bare 0-1\n"
                 "  --limit: only the first N positions\n";
    return 2;
}

} // namespace

int main(int argc, char* argv[]) {
    std::string path;
    std::size_t limit = 0;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--limit") == 0 && i + 1 < argc) {
            const char* value = argv[++i];
            const auto [end, error] = std::from_chars(value, value + std::strlen(value), limit);
            if (error != std::errc{} || *end != '\0')
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

    tuner::LoadReport load;
    tuner::PrepareReport report;
    tuner::TunerData data;
    try {
        data = tuner::prepare(path, limit, load, report);
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
    for (const std::string& error : load.firstErrors)
        std::cout << "    line " << error << '\n';
    std::cout << std::format("  quiescence   {} already quiet, {} resolved ({:.2f} moves on average, longest {})\n",
                             data.entries.size() - report.resolved, report.resolved, perLine, report.longestLine);
    std::cout << std::format("  entries      {}  white {}  draw {}  black {}\n", data.entries.size(), whiteWins, draws,
                             blackWins);
    std::cout << std::format("               {} coefficients, {:.1f} MB, {} with unfitted terms (mop-up)\n",
                             data.coefficients.size(), static_cast<double>(data.bytes()) / (1024.0 * 1024.0),
                             report.withOffset);
    std::cout << std::format("  checks       {} traced sums differ from the incremental ones, {} rebuilt scores differ "
                             "from the engine's\n",
                             report.accumulatorMismatch, report.rebuildMismatch);
    std::cout << std::format("  time         {:.1f} s, {:.0f} positions/s\n", report.seconds,
                             static_cast<double>(report.positions) / std::max(report.seconds, 1e-9));

    if (data.entries.empty())
        return 1;
    return report.accumulatorMismatch == 0 && report.rebuildMismatch == 0 ? 0 : 3;
}
