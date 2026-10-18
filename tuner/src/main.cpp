// ElephantTuner, the offline evaluation tuner, see tuner/CMakeLists.txt and tuner/README.md.
//
//   ElephantTuner <dataset> [--limit N] [--threads N] [--leave N] [--epochs N] [--rate R] [--tune LIST]
//                 [--out DIR] [--held-back N] [--free-mobility]
//
//   dataset          a text file, one position per line: a FEN and the game's result, see dataset.hpp
//   --limit          only the first N positions
//   --threads        threads to use, every logical core by default
//   --leave          cores to keep free, taken off --threads
//   --epochs         passes of the fit, 1000 by default; 0 only checks the writer
//   --rate           Adam's step, about the centipawns a number moves per pass, 1.0 by default
//   --tune           the groups of values the fit moves, comma separated names from params::groupInfos (material,
//                    psqt, mobility, pawns_passed, pawns_structure, pawns_passed_extras), psqt,mobility by default;
//                    the others stay as they are (see fit.hpp)
//   --out            the folder the tuned pesto_tables.hpp & mobility_tables.hpp are written to, as
//                    *.tuned.hpp, the current folder by default
//   --held-back      every Nth position is kept out of the fit to check it with, 20 by default, 0 for none
//   --free-mobility  don't keep mobility's tables centred, they then shift piece values (see fit.hpp)
//
// Phase 0: every position resolved to a quiet one with quiescence search, traced and kept as an entry.
// Phase 1: K, the scale between the evaluation and the game's result, and the current evaluation's error at it.
// Phase 2: the values changed until the evaluation predicts the results as well as it can, written out as new
//          pesto_tables.hpp & mobility_tables.hpp.
//
// Exit codes: 0 done, 1 an error (or no positions), 2 a bad command line, 3 phase 0's checks failed, 4 the writer
// check failed (--epochs 0).

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstring>
#include <cmath>
#include <exception>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <thread>
#include <vector>

#include <dataset.hpp>
#include <elephant_gambit_config.h>
#include <find_k.hpp>
#include <fit.hpp>
#include <system/static_initializer.hpp>
#include <tuner_data.hpp>
#include <write_tables.hpp>

bool g_initialized = static_initializer::initialize();

namespace {

constexpr int c_exitError = 1;
constexpr int c_exitUsage = 2;
constexpr int c_exitChecks = 3;
constexpr int c_exitWriterCheck = 4;

constexpr const char* c_pieceNames[6] = { "pawn", "knight", "bishop", "rook", "queen", "king" };

// --- The command line ---

struct Options {
    std::string path;
    std::size_t limit = 0;
    std::size_t threads = std::max(1u, std::thread::hardware_concurrency());   // --leave already taken off
    std::size_t heldBackEvery = 20;   // 5% of the positions
    std::string outDir = ".";
    tuner::FitSettings settings;      // k & threads are filled in by the phases
};

int usage() {
    std::cerr << "usage: ElephantTuner <dataset> [--limit N] [--threads N] [--leave N] [--epochs N] [--rate R]\n"
                 "                     [--tune LIST] [--out DIR] [--held-back N] [--free-mobility]\n"
                 "  dataset:         one position per line, a FEN and the game's result from white's side,\n"
                 "                   e.g. [1.0], c9 \"1/2-1/2\"; or a bare 0-1\n"
                 "  --limit:         only the first N positions\n"
                 "  --threads:       threads to use, every logical core by default\n"
                 "  --leave:         cores to keep free, taken off --threads\n"
                 "  --epochs:        passes of the fit, 1000 by default; 0 only checks the writer\n"
                 "  --rate:          Adam's step, about the centipawns a number moves per pass, 1.0 by default\n"
                 "  --tune:          groups the fit moves, comma separated: material, psqt, mobility, pawns_passed,\n"
                 "                   pawns_structure, pawns_passed_extras; psqt,mobility by default\n"
                 "  --out:           folder for the tuned *.tuned.hpp files, the current folder by default\n"
                 "  --held-back:     every Nth position is kept out of the fit to check it with, 20 by default, 0 none\n"
                 "  --free-mobility: don't keep mobility's tables centred (they then shift piece values, see fit.hpp)\n";
    return c_exitUsage;
}

bool parseCount(const char* text, std::size_t& out) {
    const auto [end, error] = std::from_chars(text, text + std::strlen(text), out);
    return error == std::errc{} && *end == '\0';
}

bool parseNumber(const char* text, double& out) {
    const auto [end, error] = std::from_chars(text, text + std::strlen(text), out);
    return error == std::errc{} && *end == '\0';
}

// a comma separated list of groups, e.g. "mobility" or "psqt,mobility". False when it names none.
bool parseTune(const std::string& list, tuner::FitSettings& settings) {
    settings.tuned = 0;
    std::string_view rest = list;
    while (!rest.empty()) {
        const std::size_t comma = rest.find(',');
        const std::string_view name = rest.substr(0, comma);
        const auto info = std::ranges::find(tuner::params::groupInfos, name, &tuner::params::GroupInfo::name);
        if (info == tuner::params::groupInfos.end())
            return false;
        settings.tuned |= tuner::params::bit(info->group);
        rest = comma == std::string_view::npos ? std::string_view{} : rest.substr(comma + 1);
    }
    return settings.tuned != 0;
}

// the command line's options, nullopt when it's wrong.
std::optional<Options> parseOptions(int argc, char* argv[]) {
    Options options;
    tuner::FitSettings& settings = options.settings;
    std::size_t leave = 0;
    for (int i = 1; i < argc; ++i) {
        const bool hasValue = i + 1 < argc;
        bool valid = true;
        if (std::strcmp(argv[i], "--limit") == 0 && hasValue)
            valid = parseCount(argv[++i], options.limit);
        else if (std::strcmp(argv[i], "--threads") == 0 && hasValue)
            valid = parseCount(argv[++i], options.threads) && options.threads != 0;
        else if (std::strcmp(argv[i], "--leave") == 0 && hasValue)
            valid = parseCount(argv[++i], leave);
        else if (std::strcmp(argv[i], "--epochs") == 0 && hasValue)
            valid = parseCount(argv[++i], settings.epochs);
        else if (std::strcmp(argv[i], "--rate") == 0 && hasValue)
            valid = parseNumber(argv[++i], settings.rate) && settings.rate > 0.0;
        else if (std::strcmp(argv[i], "--tune") == 0 && hasValue)
            valid = parseTune(argv[++i], settings);
        else if (std::strcmp(argv[i], "--out") == 0 && hasValue)
            options.outDir = argv[++i];
        else if (std::strcmp(argv[i], "--free-mobility") == 0)
            settings.centreMobility = false;
        else if (std::strcmp(argv[i], "--held-back") == 0 && hasValue)
            valid = parseCount(argv[++i], options.heldBackEvery);
        else if (options.path.empty() && argv[i][0] != '-')
            options.path = argv[i];
        else
            valid = false;
        if (!valid)
            return std::nullopt;
    }
    if (options.path.empty())
        return std::nullopt;
    options.threads = options.threads > leave ? options.threads - leave : 1;
    return options;
}

std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Can't read " + path);
    std::stringstream text;
    text << file.rdbuf();
    return text.str();
}

double secondsSince(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

// --- Phase 0: the data, see tuner_data.hpp ---

void printPrepared(const std::string& path, const tuner::LoadReport& load, const tuner::PrepareReport& report,
                   const tuner::TunerData& data) {
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
}

// --- Phase 1: K, see find_k.hpp ---

// a prediction that knows nothing says 0.5 for every position, its error is the bar any evaluation has to clear.
double knowNothingError(const tuner::TunerData& data) {
    double error = 0.0;
    for (const tuner::TunerEntry& entry : data.entries)
        error += (entry.result - 0.5) * (entry.result - 0.5);
    return error / static_cast<double>(data.entries.size());
}

tuner::KSearch searchK(const tuner::TunerData& data, std::size_t threads) {
    const auto start = std::chrono::steady_clock::now();
    const tuner::KSearch search = tuner::findK(data, threads);
    const double seconds = secondsSince(start);

    std::cout << "\nPhase 1, K: the scale that turns an evaluation into an expected result\n";

    // the first 31 tries are the coarse pass, K = 0.0 to 3.0, they show the error's valley.
    std::cout << "  error by K (coarse pass)\n";
    for (std::size_t i = 0; i < search.tried.size() && i < 31; i += 3) {
        const auto [k, error] = search.tried[i];
        std::cout << std::format("    K {:.1f}  error {:.6f}\n", k, error);
    }

    std::cout << std::format("  best K       {:.4f}, {} errors computed in {:.1f} s\n", search.k, search.tried.size(),
                             seconds);
    std::cout << std::format("  error        {:.6f} with the current evaluation, the baseline tuning has to beat\n",
                             search.error);
    std::cout << std::format("               {:.6f} predicting 0.5 for every position, knowing nothing\n",
                             knowNothingError(data));
    std::cout << std::format("  meaning      at this K white is expected to score");
    for (int eval : { 50, 100, 200, 400 })
        std::cout << std::format("  {:.0f}% at +{}", 100.0 * tuner::expectedResult(eval, search.k), eval);
    std::cout << '\n';
    return search;
}

// --- Phase 2: the fit, see fit.hpp ---

// the groups the fit moves, e.g. "psqt, mobility".
std::string tunedGroups(const tuner::FitSettings& settings) {
    std::string tuning;
    for (auto [group, name] : tuner::params::groupInfos) {
        if (!settings.tunes(group))
            continue;
        if (!tuning.empty())
            tuning += ", ";
        tuning += name;
    }
    return tuning;
}

/**
 * Fits the values, starting from before, on data less the positions held back, printing the error on both as it goes.
 * Returns the tuned values rounded to whole centipawns, the form the engine keeps them in.    */
std::vector<TaperedScore> runFit(tuner::TunerData&& data, const std::vector<TaperedScore>& before,
                                 const tuner::FitSettings& settings, std::size_t heldBackEvery) {
    std::cout << "\nPhase 2, fit: the values changed until the evaluation predicts the results as well as it can\n";

    // positions held back from the fit, to tell learning from memorising, see splitHeldBack.
    const tuner::SplitData split = tuner::splitHeldBack(std::move(data), heldBackEvery);
    const tuner::TunerData& fitted = split.fit;
    const tuner::TunerData& heldBack = split.heldBack;
    const bool checking = !heldBack.entries.empty();
    const auto heldBackError = [&](const tuner::Values& checked) {
        return checking ? tuner::fitError(heldBack, checked, settings.k, settings.threads) : 0.0;
    };
    std::cout << std::format("  positions    {} fitted on, {} held back to check the fit with (every {}th)\n",
                             fitted.entries.size(), heldBack.entries.size(), heldBackEvery);

    tuner::Values values = tuner::Values::from(before);
    // the fit computes the taper in real numbers, the engine rounds it down to whole centipawns: this start differs
    // from phase 1's error by that rounding only.
    const double startError = tuner::fitError(fitted, values, settings.k, settings.threads);
    const double startHeldBack = heldBackError(values);
    std::cout << std::format("  start        error {:.6f}, held back {:.6f}; {} passes at rate {}, tuning {}, the "
                             "rest kept\n",
                             startError, startHeldBack, settings.epochs, settings.rate, tunedGroups(settings));

    const auto fitStart = std::chrono::steady_clock::now();
    tuner::fit(fitted, values, settings, [&](std::size_t epoch, double error) {
        // the first few passes move most, then every 100th.
        if (epoch <= 5 || epoch % 100 == 0 || epoch == settings.epochs) {
            // the held back error with the values as they are now: falling with the fitted one, the fit is learning.
            const double check = heldBackError(values);
            std::cout << std::format("  pass {:5}   error {:.6f} ({:+.6f})   held back {:.6f} ({:+.6f})   {:.0f} s\n",
                                     epoch, error, error - startError, check, check - startHeldBack,
                                     secondsSince(fitStart));
        }
    });

    // the engine keeps whole centipawns, the cost of rounding the tuned values to them.
    std::vector<TaperedScore> after = values.rounded();
    const tuner::Values roundedValues = tuner::Values::from(after);
    const double tunedError = tuner::fitError(fitted, values, settings.k, settings.threads);
    const double roundedError = tuner::fitError(fitted, roundedValues, settings.k, settings.threads);
    std::cout << std::format("  tuned        error {:.6f}, {:.6f} rounded to whole centipawns ({:.2f}% lower than the "
                             "start)\n",
                             tunedError, roundedError, 100.0 * (startError - roundedError) / startError);
    if (checking) {
        const double roundedHeldBack = heldBackError(roundedValues);
        std::cout << std::format("               held back {:.6f} rounded ({:.2f}% lower than the start), the number "
                                 "that counts\n",
                                 roundedHeldBack, 100.0 * (startHeldBack - roundedHeldBack) / startHeldBack);
    }
    return after;
}

// how far each piece's table moved on average, to see where the tuner found something to change.
void printMoved(const std::vector<TaperedScore>& before, const std::vector<TaperedScore>& after) {
    std::cout << "  moved        average change per square, midgame / endgame centipawns\n";
    for (u32 pieceId = 0; pieceId < 6; ++pieceId) {
        double mgMoved = 0.0, egMoved = 0.0;
        for (u32 sqr = 0; sqr < 64; ++sqr) {
            const u32 index = tuner::params::psqt(pieceId, sqr);
            mgMoved += std::abs(mg_score(after[index]) - mg_score(before[index]));
            egMoved += std::abs(eg_score(after[index]) - eg_score(before[index]));
        }
        std::cout << std::format("    {:7} {:5.1f} / {:5.1f}\n", c_pieceNames[pieceId], mgMoved / 64.0, egMoved / 64.0);
    }
}

// mobility's values as tuned, by safe squares: how much a piece gains as it gets freer.
void printMobility(const std::vector<TaperedScore>& after) {
    std::cout << "  mobility     midgame / endgame centipawns by safe squares, 0 first\n";
    for (u32 pieceId = 1; pieceId <= 4; ++pieceId) {
        std::cout << std::format("    {:7}", c_pieceNames[pieceId]);
        for (u32 safe = 0; safe < tuner::params::mobilityCounts[pieceId]; ++safe) {
            const TaperedScore score = after[tuner::params::mobility(pieceId, safe)];
            std::cout << std::format(" {}/{}", mg_score(score), eg_score(score));
        }
        std::cout << '\n';
    }
}

// how many positions use each pawn value, counted before the fit: a value few positions use is fitted on noise.
std::vector<std::size_t> pawnUsage(const tuner::TunerData& data) {
    std::vector<std::size_t> used(tuner::params::count, 0);
    for (const tuner::Coefficient& c : data.coefficients)
        ++used[c.index()];
    return used;
}

// the pawn values as tuned, with how many positions use each.
void printPawns(const std::vector<TaperedScore>& after, const std::vector<std::size_t>& used) {
    struct Row {
        const char* name;
        u32 size;
        u32 (*index)(u32);
        u32 first;
    };
    const Row rows[10] = { { "passed", 6, tuner::params::passed, 1 },
                           { "isolated", 4, tuner::params::isolated, 0 },
                           { "doubled", 4, tuner::params::doubled, 0 },
                           { "backward", 4, tuner::params::backward, 0 },
                           { "connected", 6, tuner::params::connected, 1 },
                           { "own king", 8, tuner::params::passedOwnKing, 0 },
                           { "enemy king", 8, tuner::params::passedEnemyKing, 0 },
                           { "blocked", 6, tuner::params::passedBlocked, 1 },
                           { "free path", 6, tuner::params::passedFreePath, 1 },
                           { "rook behind", 2, tuner::params::passedRookBehind, 0 } };
    std::cout << "  pawns        midgame / endgame centipawns (positions using it), by rank 2-7, file a&h-d&e, king\n"
                 "               distance 0-7, or own & enemy rook\n";
    for (const Row& row : rows) {
        std::cout << std::format("    {:11}", row.name);
        for (u32 i = 0; i < row.size; ++i) {
            const u32 index = row.index(row.first + i);
            std::cout << std::format(" {}/{} ({})", mg_score(after[index]), eg_score(after[index]), used[index]);
        }
        std::cout << '\n';
    }
}

/**
 * Writes the values as new headers to outDir: the engine's own files with only the numbers in their tables replaced.
 * Returns whether every file came out the same as the engine's. Throws when a file can't be read or written.    */
bool writeTables(const std::string& outDir, const std::vector<TaperedScore>& after) {
    struct Written {
        const char* file;
        std::string (*rewrite)(const std::string&, const std::vector<TaperedScore>&);
    };
    const Written files[3] = { { "pesto_tables", tuner::rewritePestoTables },
                               { "mobility_tables", tuner::rewriteMobilityTables },
                               { "pawn_tables", tuner::rewritePawnTables } };
    bool unchanged = true;
    for (const Written& file : files) {
        const std::string from = std::format("{}/src/engine/inc/eval/{}.hpp", ROOT_PATH, file.file);
        const std::string to = std::format("{}/{}.tuned.hpp", outDir, file.file);
        const std::string header = readFile(from);
        const std::string rewritten = file.rewrite(header, after);
        unchanged = unchanged && rewritten == header;
        std::ofstream out(to, std::ios::binary);
        out << rewritten;
        if (!out)
            throw std::runtime_error("Can't write " + to);
        std::cout << std::format("  written      {}, from {}\n", to, from);
    }
    return unchanged;
}

// the three phases in turn, see the top of this file. Returns the exit code.
int run(Options& options) {
    tuner::LoadReport load;
    tuner::PrepareReport report;
    tuner::TunerData data = tuner::prepare(options.path, options.limit, options.threads, load, report);
    printPrepared(options.path, load, report, data);
    if (data.entries.empty())
        return c_exitError;
    if (report.accumulatorMismatch != 0 || report.rebuildMismatch != 0)
        return c_exitChecks;

    tuner::FitSettings& settings = options.settings;
    settings.k = searchK(data, report.threads).k;
    settings.threads = report.threads;

    const std::vector<TaperedScore> before = tuner::params::current();
    const std::vector<std::size_t> used = pawnUsage(data);
    const std::vector<TaperedScore> after = runFit(std::move(data), before, settings, options.heldBackEvery);
    printMoved(before, after);
    if (settings.tunes(tuner::params::Group::Mobility))
        printMobility(after);
    if (settings.tunes(tuner::params::Group::Pawns_Passed) || settings.tunes(tuner::params::Group::Pawns_Structure)
        || settings.tunes(tuner::params::Group::Pawns_PassedExtras))
        printPawns(after, used);
    const bool unchanged = writeTables(options.outDir, after);

    // no passes means the values went out as they came in: the files have to be the engine's, byte for byte.
    if (settings.epochs == 0) {
        std::cout << std::format("  writer check {}\n", unchanged ? "the untuned tables were written back unchanged"
                                                                    : "FAILED, the untuned tables came out different");
        return unchanged ? 0 : c_exitWriterCheck;
    }
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    std::optional<Options> options = parseOptions(argc, argv);
    if (!options)
        return usage();
    try {
        return run(*options);
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return c_exitError;
    }
}
