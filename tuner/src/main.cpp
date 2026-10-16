// ElephantTuner, the offline evaluation tuner, see tuner/CMakeLists.txt.
//
//   ElephantTuner <dataset> [--limit N] [--threads N] [--leave N] [--epochs N] [--rate R] [--tune-material]
//                 [--out FILE] [--held-back N]
//
//   dataset          a text file, one position per line: a FEN and the game's result, see dataset.hpp
//   --limit          only the first N positions
//   --threads        threads to use, every logical core by default
//   --leave          cores to keep free, taken off --threads
//   --epochs         passes of the fit, 1000 by default; 0 only checks the writer
//   --rate           Adam's step, about the centipawns a number moves per pass, 1.0 by default
//   --tune-material  tune PeSTO's material too, it's pinned by default (see fit.hpp)
//   --out            where the tuned pesto_tables.hpp is written, pesto_tables.tuned.hpp by default
//   --held-back      every Nth position is kept out of the fit to check it with, 20 by default, 0 for none
//
// Phase 0: every position resolved to a quiet one with quiescence search, traced and kept as an entry.
// Phase 1: K, the scale between the evaluation and the game's result, and the current evaluation's error at it.
// Phase 2: the values changed until the evaluation predicts the results as well as it can, written out as a new
//          pesto_tables.hpp.

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstring>
#include <cmath>
#include <exception>
#include <format>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include <dataset.hpp>
#include <elephant_gambit_config.h>
#include <find_k.hpp>
#include <fit.hpp>
#include <system/static_initializer.hpp>
#include <tuner_data.hpp>
#include <write_tables.hpp>

bool g_initialized = static_initializer::initialize();

namespace {

int usage() {
    std::cerr << "usage: ElephantTuner <dataset> [--limit N] [--threads N] [--leave N] [--epochs N] [--rate R]\n"
                 "                     [--tune-material] [--out FILE] [--held-back N]\n"
                 "  dataset:         one position per line, a FEN and the game's result from white's side,\n"
                 "                   e.g. [1.0], c9 \"1/2-1/2\"; or a bare 0-1\n"
                 "  --limit:         only the first N positions\n"
                 "  --threads:       threads to use, every logical core by default\n"
                 "  --leave:         cores to keep free, taken off --threads\n"
                 "  --epochs:        passes of the fit, 1000 by default; 0 only checks the writer\n"
                 "  --rate:          Adam's step, about the centipawns a number moves per pass, 1.0 by default\n"
                 "  --tune-material: tune PeSTO's material too, it's pinned by default\n"
                 "  --out:           where the tuned pesto_tables.hpp is written, pesto_tables.tuned.hpp by default\n"
                 "  --held-back:     every Nth position is kept out of the fit to check it with, 20 by default, 0 none\n";
    return 2;
}

bool parseCount(const char* text, std::size_t& out) {
    const auto [end, error] = std::from_chars(text, text + std::strlen(text), out);
    return error == std::errc{} && *end == '\0';
}

bool parseNumber(const char* text, double& out) {
    const auto [end, error] = std::from_chars(text, text + std::strlen(text), out);
    return error == std::errc{} && *end == '\0';
}

std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Can't read " + path);
    std::stringstream text;
    text << file.rdbuf();
    return text.str();
}

} // namespace

int main(int argc, char* argv[]) {
    std::string path;
    std::size_t limit = 0;
    std::size_t threads = std::max(1u, std::thread::hardware_concurrency());
    std::size_t leave = 0;
    tuner::FitSettings settings;
    std::string outPath = "pesto_tables.tuned.hpp";
    std::size_t heldBackEvery = 20;   // 5% of the positions
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
        else if (std::strcmp(argv[i], "--epochs") == 0 && hasValue) {
            if (!parseCount(argv[++i], settings.epochs))
                return usage();
        }
        else if (std::strcmp(argv[i], "--rate") == 0 && hasValue) {
            if (!parseNumber(argv[++i], settings.rate) || settings.rate <= 0.0)
                return usage();
        }
        else if (std::strcmp(argv[i], "--tune-material") == 0) {
            settings.tuneMaterial = true;
        }
        else if (std::strcmp(argv[i], "--out") == 0 && hasValue) {
            outPath = argv[++i];
        }
        else if (std::strcmp(argv[i], "--held-back") == 0 && hasValue) {
            if (!parseCount(argv[++i], heldBackEvery))
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

    // --- Phase 2: the fit, see fit.hpp ---
    std::cout << "\nPhase 2, fit: the values changed until the evaluation predicts the results as well as it can\n";
    settings.k = search.k;
    settings.threads = report.threads;

    // positions held back from the fit, to tell learning from memorising, see splitHeldBack.
    const tuner::SplitData split = tuner::splitHeldBack(std::move(data), heldBackEvery);
    const tuner::TunerData& fitted = split.fit;
    const tuner::TunerData& heldBack = split.heldBack;
    const bool checking = !heldBack.entries.empty();
    std::cout << std::format("  positions    {} fitted on, {} held back to check the fit with (every {}th)\n",
                             fitted.entries.size(), heldBack.entries.size(), heldBackEvery);

    const std::vector<TaperedScore> before = tuner::params::current();
    tuner::Values values = tuner::Values::from(before);
    // the fit computes the taper in real numbers, the engine rounds it down to whole centipawns: this start differs
    // from phase 1's error by that rounding only.
    const double startError = tuner::fitError(fitted, values, settings.k, settings.threads);
    const double startHeldBack = checking ? tuner::fitError(heldBack, values, settings.k, settings.threads) : 0.0;
    std::cout << std::format("  start        error {:.6f}, held back {:.6f}; {} passes at rate {}, material {}\n",
                             startError, startHeldBack, settings.epochs, settings.rate,
                             settings.tuneMaterial ? "tuned" : "pinned");

    const auto fitStart = std::chrono::steady_clock::now();
    tuner::fit(fitted, values, settings, [&](std::size_t epoch, double error) {
        // the first few passes move most, then every 100th.
        if (epoch <= 5 || epoch % 100 == 0 || epoch == settings.epochs) {
            const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - fitStart).count();
            // the held back error with the values as they are now: falling with the fitted one, the fit is learning.
            const double check = checking ? tuner::fitError(heldBack, values, settings.k, settings.threads) : 0.0;
            std::cout << std::format("  pass {:5}   error {:.6f} ({:+.6f})   held back {:.6f} ({:+.6f})   {:.0f} s\n",
                                     epoch, error, error - startError, check, check - startHeldBack, seconds);
        }
    });

    // the engine keeps whole centipawns, the cost of rounding the tuned values to them.
    const std::vector<TaperedScore> after = values.rounded();
    const tuner::Values roundedValues = tuner::Values::from(after);
    const double tunedError = tuner::fitError(fitted, values, settings.k, settings.threads);
    const double roundedError = tuner::fitError(fitted, roundedValues, settings.k, settings.threads);
    std::cout << std::format("  tuned        error {:.6f}, {:.6f} rounded to whole centipawns ({:.2f}% lower than the "
                             "start)\n",
                             tunedError, roundedError, 100.0 * (startError - roundedError) / startError);
    if (checking) {
        const double roundedHeldBack = tuner::fitError(heldBack, roundedValues, settings.k, settings.threads);
        std::cout << std::format("               held back {:.6f} rounded ({:.2f}% lower than the start), the number "
                                 "that counts\n",
                                 roundedHeldBack, 100.0 * (startHeldBack - roundedHeldBack) / startHeldBack);
    }

    // how far each piece's table moved on average, to see where the tuner found something to change.
    const char* pieceNames[6] = { "pawn", "knight", "bishop", "rook", "queen", "king" };
    std::cout << "  moved        average change per square, midgame / endgame centipawns\n";
    for (u32 pieceId = 0; pieceId < 6; ++pieceId) {
        double mgMoved = 0.0, egMoved = 0.0;
        for (u32 sqr = 0; sqr < 64; ++sqr) {
            const u32 index = tuner::params::psqt(pieceId, sqr);
            mgMoved += std::abs(mg_score(after[index]) - mg_score(before[index]));
            egMoved += std::abs(eg_score(after[index]) - eg_score(before[index]));
        }
        std::cout << std::format("    {:7} {:5.1f} / {:5.1f}\n", pieceNames[pieceId], mgMoved / 64.0, egMoved / 64.0);
    }

    // written as a new pesto_tables.hpp: the engine's file with only the numbers in its tables replaced.
    const std::string headerPath = std::string(ROOT_PATH) + "/src/engine/inc/eval/pesto_tables.hpp";
    std::string header;
    std::string rewritten;
    try {
        header = readFile(headerPath);
        rewritten = tuner::rewritePestoTables(header, after);
        std::ofstream out(outPath, std::ios::binary);
        out << rewritten;
        if (!out)
            throw std::runtime_error("Can't write " + outPath);
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::cout << std::format("  written      {}, from {}\n", outPath, headerPath);

    // no passes means the values went out as they came in: the file has to be the engine's, byte for byte.
    if (settings.epochs == 0) {
        const bool same = rewritten == header;
        std::cout << std::format("  writer check {}\n", same ? "the untuned tables were written back unchanged"
                                                               : "FAILED, the untuned tables came out different");
        return same ? 0 : 4;
    }
    return 0;
}
