#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <format>
#include <iomanip>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include "elephant_gambit_config.h"
#include "core/game_context.hpp"
#include "io/fen_parser.hpp"
#include "io/san_parser.hpp"
#include "search/search.hpp"
#include "search/transposition_table.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <system/time_manager.hpp>

// EPD suites measure engine strength rather than correctness, so a few misses are expected. Each suite runs every
// position and passes as long as enough of them are solved. They come in two tiers:
//
//   Smoke*     the local sanity check:  ElephantSuite --gtest_filter=PerftFixture.*:EpdSuite.Smoke*
//   Extended*  the same suites with more budget, what CI runs:  ElephantSuite --gtest_filter=EpdSuite.Extended*
//
// Every tier searches a fixed number of nodes per position. With one thread that's deterministic and independent of
// the machine, the solved count is the same on every run and every computer, so the minimums sit just below it and a
// drop means the engine's behaviour changed. Only the multi threaded suite varies between runs.
//
// The environment overrides a suite's settings for experiments:
//   EG_EPD_NODES=200000    nodes per position, all threads together
//   EG_EPD_DEPTH=7         fixed depth instead of nodes
//   EG_EPD_MOVETIME=1000   milliseconds per position instead of nodes
//   EG_EPD_THREADS=4       search threads
//   EG_EPD_HASH=64         transposition table MB
//   EG_EPD_MIN_PASSED=280  the minimum number of solved positions
//
// Every run appends a line to logs/epd_results.csv: version, suite, settings, solved, total & seconds, to follow the
// scores from version to version.

struct EpdTestCase {
    std::string id;
    std::string fen;
    std::vector<std::string> bestMoves;   // bm, SAN, one of them has to be played
    std::vector<std::string> avoidMoves;  // am, SAN, none of them may be played
    // STS style scoring from c0 "f5=10, Be5+=2, Bf2=3": the points a move earns, every other move earns 0.
    std::vector<std::pair<std::string, int>> moveScores;

    int maxScore() const {
        int best = 0;
        for (const auto& [move, points] : moveScores)
            best = std::max(best, points);
        return best;
    }
};

std::ostream& operator<<(std::ostream& os, const EpdTestCase& tc) {
    return os << "\n  ID:       " << tc.id << "\n  FEN:      " << tc.fen;
}

namespace {

std::string trim(const std::string& text) {
    const size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";
    return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}

std::vector<std::string> splitWords(const std::string& text) {
    std::vector<std::string> words;
    std::istringstream stream(text);
    for (std::string word; stream >> word;)
        words.push_back(word);
    return words;
}

std::string joined(const std::vector<std::string>& words) {
    std::string result;
    for (const std::string& word : words)
        result += (result.empty() ? "" : " ") + word;
    return result;
}

} // namespace

// e2e4 or e7e8q, compared with the played move as is, the SAN parser doesn't read it.
bool isCoordinateMove(const std::string& text) {
    auto file = [](char c) { return c >= 'a' && c <= 'h'; };
    auto rank = [](char c) { return c >= '1' && c <= '8'; };
    return (text.size() == 4 || (text.size() == 5 && std::string("qrbn").find(text[4]) != std::string::npos))
        && file(text[0]) && rank(text[1]) && file(text[2]) && rank(text[3]);
}

// "f5=10, Be5+=2, Bf2=3" as move & points, empty when the comment isn't a list of scored moves. Every move=points pair
// is read, a few STS lines miss a comma ("Bd4=10Qb8=6", "Rg3=7 Rd5=9"). Text left over that isn't a separator makes it
// an ordinary comment, "...topic_show.pl?tid=18253" isn't a move.
std::vector<std::pair<std::string, int>> parseMoveScores(const std::string& text) {
    // SAN with an optional promotion (exf8=Q+) or coordinate notation (b8a8, e7e8q), then the points.
    static const std::regex pair(R"(((?:[KQRBN]?[a-h]?[1-8]?x?[a-h][1-8](?:=[QRBN])?|O-O-O|O-O|[a-h][1-8][a-h][1-8][qrbn]?)[+#]?)=(-?\d+))");
    std::vector<std::pair<std::string, int>> scores;
    std::string leftover;
    size_t last = 0;
    for (auto it = std::sregex_iterator(text.begin(), text.end(), pair); it != std::sregex_iterator(); ++it) {
        leftover += text.substr(last, it->position() - last);
        last = it->position() + it->length();
        scores.emplace_back((*it)[1].str(), std::stoi((*it)[2].str()));
    }
    leftover += text.substr(last);
    if (leftover.find_first_not_of(" ,\"") != std::string::npos)
        return {};
    return scores;
}

// An EPD line is the first four FEN fields, board, side, castling & en passant, then operations "opcode operands;".
// bm (best moves), am (avoid moves), id and an STS score list in c0 are read, the rest is ignored. Lines without bm
// or am are skipped.
std::optional<EpdTestCase> parseEpdLine(const std::string& line) {
    std::istringstream stream(line);
    std::vector<std::string> fields(4);
    for (std::string& field : fields) {
        if (!(stream >> field))
            return std::nullopt;
    }

    EpdTestCase tc;
    // EPD has no clocks, the FEN parser wants them.
    tc.fen = joined(fields) + " 0 1";

    std::string rest;
    std::getline(stream, rest);
    std::istringstream operations(rest);
    for (std::string operation; std::getline(operations, operation, ';');) {
        const std::vector<std::string> words = splitWords(trim(operation));
        if (words.empty())
            continue;
        const std::vector<std::string> operands(words.begin() + 1, words.end());
        if (words[0] == "bm")
            tc.bestMoves = operands;
        else if (words[0] == "am")
            tc.avoidMoves = operands;
        else if (words[0] == "id") {
            tc.id = joined(operands);
            tc.id.erase(std::remove(tc.id.begin(), tc.id.end(), '"'), tc.id.end());
        }
        else if (words[0] == "c0") {
            tc.moveScores = parseMoveScores(joined(operands));
        }
    }
    if (tc.bestMoves.empty() && tc.avoidMoves.empty())
        return std::nullopt;
    return tc;
}

std::vector<EpdTestCase> loadEpdFile(const std::string& filePath) {
    std::vector<EpdTestCase> cases;
    std::ifstream file(filePath);
    for (std::string line; std::getline(file, line);) {
        if (auto tc = parseEpdLine(trim(line)))
            cases.push_back(*tc);
    }
    return cases;
}

// What a suite searches with, a tier's defaults overridden by the environment.
struct EpdSettings {
    u64 nodes = 0;
    u8 depth = 0;
    u32 moveTime = 0;
    u16 threads = 1;
    size_t hashMb = 64;

    std::string describe() const {
        std::string limit = nodes ? std::format("nodes {}", nodes) : depth ? std::format("depth {}", depth)
                                                                           : std::format("movetime {}", moveTime);
        return std::format("{}, threads {}, hash {}", limit, threads, hashMb);
    }
};

// Two levels per suite: below the baseline, the count measured when the suite was last tuned, is a warning that names
// the drop, below the minimum is a failure. A search change can lose a few solves and still gain Elo, SPRT decides
// that, the minimum only catches drops too large to be anything but a bug. Settings from the environment are an
// experiment, neither level applies then unless EG_EPD_MIN_PASSED sets a minimum.
// Multi threaded suites aren't repeatable, WAC with 4 threads went 282 then 287, the baseline only documents a typical
// count and only the minimum, crashes, hangs & gross regressions, is checked.
struct Thresholds {
    long long baseline = 0;
    long long minimum = 0;
    bool repeatable = true;
};

class EpdSuite : public ::testing::Test {
protected:
    static std::string timestamp(const char* format) {
        auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm, format);
        return oss.str();
    }

    static void initializeLogger(const std::string& suiteName) {
        std::filesystem::create_directories("logs");
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            std::format("logs/{}_{}.log", suiteName, timestamp("%Y%m%d_%H%M")), true);
        auto logger = std::make_shared<spdlog::logger>("engine", file_sink);
        spdlog::set_default_logger(logger);
        spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v");
        spdlog::set_level(spdlog::level::debug);
    }

    static bool environmentOverrides() {
        for (const char* name : { "EG_EPD_NODES", "EG_EPD_DEPTH", "EG_EPD_MOVETIME", "EG_EPD_THREADS", "EG_EPD_HASH" }) {
            const char* value = std::getenv(name);
            if (value != nullptr && *value != '\0')
                return true;
        }
        return false;
    }

    // Prints the warning or note for value against the suite's levels and fails below the minimum.
    static void checkThresholds(const std::string& suiteName, long long value, Thresholds levels, const std::string& summary) {
        if (environmentOverrides())
            levels = {};
        if (const char* minimumEnv = std::getenv("EG_EPD_MIN_PASSED"))
            levels.minimum = std::atoll(minimumEnv);

        if (value < levels.minimum) {
            ADD_FAILURE() << std::format("FAIL {}: {} is below the minimum {}", suiteName, value, levels.minimum) << "\n"
                          << summary;
            return;
        }
        if (levels.baseline == 0 || !levels.repeatable)
            return;
        std::string message;
        if (value < levels.baseline)
            message = std::format("WARNING {}: {} is {} below the baseline {}, minimum {}", suiteName, value,
                                  levels.baseline - value, levels.baseline, levels.minimum);
        else if (value > levels.baseline)
            message = std::format("NOTE {}: {} is {} above the baseline {}, raise it if the change is meant to stay",
                                  suiteName, value, value - levels.baseline, levels.baseline);
        if (!message.empty()) {
            spdlog::warn(message);
            std::cout << message << "\n";
        }
    }

    static EpdSettings applyEnvironment(EpdSettings settings) {
        auto number = [](const char* name) -> std::optional<long long> {
            const char* value = std::getenv(name);
            if (value == nullptr || *value == '\0')
                return std::nullopt;
            return std::atoll(value);
        };
        // a limit from the environment replaces the tier's, nodes, depth and movetime exclude each other.
        if (auto nodes = number("EG_EPD_NODES"))
            settings = EpdSettings{ .nodes = static_cast<u64>(*nodes), .threads = settings.threads, .hashMb = settings.hashMb };
        if (auto depth = number("EG_EPD_DEPTH"))
            settings = EpdSettings{ .depth = static_cast<u8>(*depth), .threads = settings.threads, .hashMb = settings.hashMb };
        if (auto moveTime = number("EG_EPD_MOVETIME"))
            settings = EpdSettings{ .moveTime = static_cast<u32>(*moveTime), .threads = settings.threads, .hashMb = settings.hashMb };
        if (auto threads = number("EG_EPD_THREADS"))
            settings.threads = static_cast<u16>(std::clamp<long long>(*threads, 1, 64));
        if (auto hash = number("EG_EPD_HASH"))
            settings.hashMb = static_cast<size_t>(std::max<long long>(*hash, 1));
        return settings;
    }

    static std::vector<PackedMove> parseSanMoves(const GameContext& context, const std::vector<std::string>& sanMoves,
                                                 const EpdTestCase& tc) {
        std::vector<PackedMove> moves;
        for (const std::string& san : sanMoves) {
            try {
                moves.push_back(io::san_parser::deserialize(context.readChessPosition(),
                                                            context.readToPlay() == Set::WHITE, san));
            }
            catch (...) {
                // a broken test case is a bug in the suite or the parser, never an expected miss
                ADD_FAILURE() << "SAN parser failed on " << san << " in " << tc.id;
            }
        }
        return moves;
    }

    // A c0 alternative is the suite author's extra, not the case itself: one the SAN parser can't read, STS lists a few
    // illegal moves, is skipped with a note rather than failing the suite like a broken bm or am does.
    static bool scoredMoveMatches(const GameContext& context, const std::string& san, const std::string& played,
                                  const EpdTestCase& tc) {
        try {
            const PackedMove move = io::san_parser::deserialize(context.readChessPosition(),
                                                                context.readToPlay() == Set::WHITE, san);
            return !move.isNull() && move.toString() == played;
        }
        catch (...) {
            const std::string message = std::format("NOTE {}: c0 move {} can't be parsed, it scores nothing", tc.id, san);
            spdlog::warn(message);
            std::cout << message << "\n";
            return false;
        }
    }

    struct CaseResult {
        bool solved = false;
        int points = 0;   // scored suites, the points of the move played
    };

    // Searches the position, solved when the engine played one of the best moves and none of the avoid moves. A scored
    // position also earns the points of the move played. Misses are printed unless printMisses is off.
    CaseResult runCase(GameContext& context, const EpdTestCase& tc, const EpdSettings& settings, bool printMisses = true) {
        context.Reset();
        context.editTranspositionTable().clear();
        if (!io::fen_parser::deserialize(tc.fen.c_str(), context.editChessboard())) {
            ADD_FAILURE() << "FEN parser failed on " << tc;
            return {};
        }

        SearchParameters params;
        params.SearchDepth = settings.depth;
        params.MoveTime = settings.moveTime;
        params.NodeLimit = settings.nodes;
        params.ThreadCount = settings.threads;

        Search searcher(context);
        SearchResult result;
        TimeManager timeManager(params, context.readToPlay());
        if (context.readToPlay() == Set::WHITE) {
            timeManager.applyTimeSettings(params, Set::WHITE);
            result = searcher.go<Set::WHITE>(params, timeManager);
        }
        else {
            timeManager.applyTimeSettings(params, Set::BLACK);
            result = searcher.go<Set::BLACK>(params, timeManager);
        }

        const std::string played = result.move().toString();
        auto contains = [&played](const std::vector<PackedMove>& moves) {
            return std::any_of(moves.begin(), moves.end(), [&](PackedMove move) { return move.toString() == played; });
        };
        const std::vector<PackedMove> best = parseSanMoves(context, tc.bestMoves, tc);
        const std::vector<PackedMove> avoid = parseSanMoves(context, tc.avoidMoves, tc);
        const bool solved = (tc.bestMoves.empty() || contains(best)) && !contains(avoid);
        int points = 0;
        for (const auto& [move, value] : tc.moveScores) {
            if (isCoordinateMove(move) ? move == played : scoredMoveMatches(context, move, played, tc))
                points = value;
        }

        const std::string expected = tc.bestMoves.empty() ? "not " + joined(tc.avoidMoves) : joined(tc.bestMoves);
        if (solved) {
            spdlog::debug("{} solved, expected {} | engine {} | points {}", tc.id, expected, played, points);
        }
        else {
            spdlog::error("{} missed, expected {} | engine {} | points {}", tc.id, expected, played, points);
            if (printMisses)
                std::cout << std::format("  miss {:<14} expected {:<14} engine {:<6} score {:>6} nodes {:>10}  pv {}\n",
                    tc.id, expected, played, result.score, result.count, result.pvLine.toString());
        }
        return { solved, points };
    }

    // score & maxScore only for scored suites, 0 otherwise.
    static void appendResult(const std::string& suiteName, const EpdSettings& settings, size_t solved, size_t total,
                             double seconds, long long score = 0, long long maxScore = 0) {
        const std::filesystem::path path = std::filesystem::path(ROOT_PATH) / "logs" / "epd_results.csv";
        std::filesystem::create_directories(path.parent_path());
        const bool header = !std::filesystem::exists(path);
        std::ofstream csv(path, std::ios::app);
        if (header)
            csv << "date,version,suite,nodes,depth,movetime,threads,hash,solved,total,score,max_score,seconds\n";
        csv << std::format("{},{},{},{},{},{},{},{},{},{},{},{},{:.1f}\n", timestamp("%Y-%m-%d %H:%M"),
            ELEPHANT_GAMBIT_VERSION_FULL, suiteName, settings.nodes, settings.depth, settings.moveTime,
            settings.threads, settings.hashMb, solved, total, score, maxScore, seconds);
    }

    // "STS(v2.0) Open Files and Diagonals.001" is the theme "Open Files and Diagonals".
    static std::string themeName(std::string id) {
        id = id.substr(0, id.rfind('.'));
        const size_t prefix = id.find(") ");
        if (prefix != std::string::npos)
            return id.substr(prefix + 2);
        if (id.rfind("STS: ", 0) == 0)
            return id.substr(5);
        return id;
    }

    // Scored suites like STS: every file is a theme, a position earns the points of the move played. The levels apply to
    // the total points, the score is reported per theme.
    void runScoredSuite(const std::string& suiteName, const std::vector<std::string>& epdFiles, EpdSettings tierSettings,
                        Thresholds levels) {
        const EpdSettings settings = applyEnvironment(tierSettings);

        initializeLogger(suiteName);
        GameContext context;
        if (settings.hashMb != context.editTranspositionTable().size_mb())
            context.editTranspositionTable().resize(settings.hashMb);

        const auto start = std::chrono::steady_clock::now();
        size_t solved = 0, total = 0;
        long long score = 0, maxScore = 0;
        for (const std::string& epdFile : epdFiles) {
            const std::vector<EpdTestCase> cases = loadEpdFile(std::format("{}/res/{}", ROOT_PATH, epdFile));
            if (cases.empty()) {
                ADD_FAILURE() << "No positions found in res/" << epdFile;
                continue;
            }
            long long themeScore = 0, themeMax = 0;
            size_t themeSolved = 0;
            for (const auto& tc : cases) {
                const CaseResult result = runCase(context, tc, settings, false);
                themeSolved += result.solved ? 1 : 0;
                // a position without a score list earns 10 for its best move, as STS scores them.
                const bool unscored = tc.moveScores.empty();
                themeScore += unscored ? (result.solved ? 10 : 0) : result.points;
                themeMax += unscored ? 10 : tc.maxScore();
            }
            const std::string theme = themeName(cases.front().id);
            const std::string line = std::format("  {:<9} {:<45.45} {:>5}/{:<5} ({:>5.1f}%)  best move {:>3}/{}", epdFile,
                theme, themeScore, themeMax, themeMax ? 100.0 * themeScore / themeMax : 0.0, themeSolved, cases.size());
            spdlog::info(line);
            std::cout << line << "\n";
            solved += themeSolved;
            total += cases.size();
            score += themeScore;
            maxScore += themeMax;
        }
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

        const std::string summary = std::format("{}: {}/{} points ({:.1f}%), best move {}/{}, baseline {}, minimum {}, {}, "
            "{:.0f} s", suiteName, score, maxScore, maxScore ? 100.0 * score / maxScore : 0.0, solved, total,
            levels.baseline, levels.minimum, settings.describe(), seconds);
        spdlog::info(summary);
        std::cout << summary << "\n";
        appendResult(suiteName, settings, solved, total, seconds, score, maxScore);
        checkThresholds(suiteName, score, levels, summary);
    }

    // Runs every position in res/epdFile, the levels apply to the number solved.
    void runSuite(const std::string& suiteName, const std::string& epdFile, EpdSettings tierSettings, Thresholds levels) {
        const std::vector<EpdTestCase> cases = loadEpdFile(std::format("{}/res/{}", ROOT_PATH, epdFile));
        if (cases.empty()) {
            GTEST_SKIP() << "No positions found in res/" << epdFile;
        }
        const EpdSettings settings = applyEnvironment(tierSettings);

        initializeLogger(suiteName);
        GameContext context;
        if (settings.hashMb != context.editTranspositionTable().size_mb())
            context.editTranspositionTable().resize(settings.hashMb);

        const auto start = std::chrono::steady_clock::now();
        size_t solved = 0;
        for (const auto& tc : cases) {
            if (runCase(context, tc, settings).solved)
                ++solved;
        }
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

        const std::string summary = std::format("{}: {}/{} solved ({:.1f}%), baseline {}, minimum {}, {}, {:.0f} s",
            suiteName, solved, cases.size(), 100.0 * solved / cases.size(), levels.baseline, levels.minimum,
            settings.describe(), seconds);
        spdlog::info(summary);
        std::cout << summary << "\n";
        appendResult(suiteName, settings, solved, cases.size(), seconds);
        checkThresholds(suiteName, static_cast<long long>(solved), levels, summary);
    }
};

// Strategic Test Suite, positional judgement rather than tactics. Each position scores the move played, 10 for the best.
std::vector<std::string> stsFiles() {
    std::vector<std::string> files;
    for (int theme = 1; theme <= 15; ++theme) {
        if (std::filesystem::exists(std::format("{}/res/STS{}.epd", ROOT_PATH, theme)))
            files.push_back(std::format("STS{}.epd", theme));
    }
    return files;
}

// --- Smoke tier, the local sanity check ---
// Baselines are the counts measured at 1.0.0-herd.7. The minimums allow a few percent, the multi threaded suites
// differ from run to run and get more room, for them the minimum mostly catches crashes & hangs.

TEST_F(EpdSuite, Smoke_WinAtChess) {
    runSuite("Smoke_WinAtChess", "wac_new.epd", { .nodes = 2'000'000 }, { .baseline = 282, .minimum = 272 });
}

TEST_F(EpdSuite, Smoke_WinAtChess_4Threads) {
    runSuite("Smoke_WinAtChess_4Threads", "wac_new.epd", { .nodes = 8'000'000, .threads = 4 }, { .baseline = 282, .minimum = 265, .repeatable = false });
}

TEST_F(EpdSuite, Smoke_Arasan2024_4Threads) {
    runSuite("Smoke_Arasan2024_4Threads", "arasan2024.epd", { .nodes = 16'000'000, .threads = 4 }, { .baseline = 11, .minimum = 7, .repeatable = false });
}

TEST_F(EpdSuite, Smoke_STS) {
    if (stsFiles().empty())
        GTEST_SKIP() << "No STS files in res/";
    runScoredSuite("Smoke_STS", stsFiles(), { .nodes = 200'000 }, { .baseline = 7837, .minimum = 7600 });
}

// --- Extended tier, what CI runs: the smoke suites with more budget ---

TEST_F(EpdSuite, Extended_WinAtChess) {
    runSuite("Extended_WinAtChess", "wac_new.epd", { .nodes = 4'000'000 }, { .baseline = 288, .minimum = 282 });
}

TEST_F(EpdSuite, Extended_WinAtChess_4Threads) {
    runSuite("Extended_WinAtChess_4Threads", "wac_new.epd", { .nodes = 16'000'000, .threads = 4 }, { .baseline = 290, .minimum = 278, .repeatable = false });
}

// Arasan at the same nodes per thread with 1 and 4 threads, the difference is what Lazy SMP adds, 12 against 16.
TEST_F(EpdSuite, Extended_Arasan2024) {
    runSuite("Extended_Arasan2024", "arasan2024.epd", { .nodes = 8'000'000 }, { .baseline = 12, .minimum = 9 });
}

TEST_F(EpdSuite, Extended_Arasan2024_4Threads) {
    runSuite("Extended_Arasan2024_4Threads", "arasan2024.epd", { .nodes = 32'000'000, .threads = 4 }, { .baseline = 16, .minimum = 11, .repeatable = false });
}

TEST_F(EpdSuite, Extended_STS) {
    if (stsFiles().empty())
        GTEST_SKIP() << "No STS files in res/";
    runScoredSuite("Extended_STS", stsFiles(), { .nodes = 1'000'000 }, { .baseline = 8475, .minimum = 8250 });
}

// --- The EPD parser ---

TEST(EpdParser, BestMovesAvoidMovesAndId) {
    auto tc = parseEpdLine(R"(8/1p6/p5R1/k7/Prpp4/K7/1NP5/8 w - - am Rd6; bm Rb6 Rg5+; id "WAC.274";)");
    ASSERT_TRUE(tc.has_value());
    EXPECT_EQ("8/1p6/p5R1/k7/Prpp4/K7/1NP5/8 w - - 0 1", tc->fen);
    EXPECT_EQ((std::vector<std::string>{ "Rb6", "Rg5+" }), tc->bestMoves);
    EXPECT_EQ((std::vector<std::string>{ "Rd6" }), tc->avoidMoves);
    EXPECT_EQ("WAC.274", tc->id);
}

TEST(EpdParser, AvoidMoveOnly_CommentsIgnored) {
    auto tc = parseEpdLine(R"(k1b4r/1p3p2/pq2pNp1/5n1p/P3QP2/1P1R1BPP/2P5/1K6 b - - am Nxg3; id "arasan2024.50"; c0 "Deep Fritz 13 Q6600-Arasan 17.2 Q6600, SSDF 2014";)");
    ASSERT_TRUE(tc.has_value());
    EXPECT_TRUE(tc->bestMoves.empty());
    EXPECT_EQ((std::vector<std::string>{ "Nxg3" }), tc->avoidMoves);
    EXPECT_EQ("arasan2024.50", tc->id);
}

TEST(EpdParser, StsMoveScores) {
    auto tc = parseEpdLine(R"(1kr5/3n4/q3p2p/p2n2p1/PppB1P2/5BP1/1P2Q2P/3R2K1 w - - bm f5; id "Undermine.001"; c0 "f5=10, Be5+=2, Bf2=3, Bg4=2";)");
    ASSERT_TRUE(tc.has_value());
    EXPECT_EQ((std::vector<std::pair<std::string, int>>{ { "f5", 10 }, { "Be5+", 2 }, { "Bf2", 3 }, { "Bg4", 2 } }),
              tc->moveScores);
    EXPECT_EQ(10, tc->maxScore());

    // an ordinary comment isn't a score list.
    auto arasan = parseEpdLine(R"(r1bq1r1k/p1pnbpp1/1p2p3/6p1/3PB3/5N2/PPPQ1PPP/2KR3R w - - bm g4; id "arasan2024.1"; c0 "J. Polgar-Berkes, Budapest Hunguest Hotels 2003";)");
    ASSERT_TRUE(arasan.has_value());
    EXPECT_TRUE(arasan->moveScores.empty());

    // nor is one with an '=' in a URL, arasan2024.84.
    auto url = parseEpdLine(R"(rnb1kb1r/pp1p1ppp/1q2p3/8/3NP1n1/2N1B3/PPP2PPP/R2QKB1R w KQkq - bm Qxg4; id "arasan2024.84"; c0 "Gaw Paw Sicilian: http://rybkaforum.net/cgi-bin/rybkaforum/topic_show.pl?tid=18253";)");
    ASSERT_TRUE(url.has_value());
    EXPECT_TRUE(url->moveScores.empty());
}

TEST(EpdParser, StsMoveScores_CoordinateAndPromotionMoves) {
    // STS lists a few alternatives in coordinate notation, STS(v9.0) Advancement of a/b/c pawns.005.
    auto tc = parseEpdLine(R"(1q2r1k1/1b2bpp1/p2ppn1p/2p5/P3PP1B/2PB1RP1/2P1Q2P/2KR4 b - - bm c4; id "STS(v9.0) Advancement of a/b/c pawns.005"; c0 "c4=10, Bc6=5, Qa7=4, Qa8=5, Qc8=5, b8a8=5";)");
    ASSERT_TRUE(tc.has_value());
    ASSERT_EQ(6u, tc->moveScores.size());
    EXPECT_EQ((std::pair<std::string, int>{ "b8a8", 5 }), tc->moveScores.back());
    EXPECT_TRUE(isCoordinateMove("b8a8"));
    EXPECT_TRUE(isCoordinateMove("e7e8q"));
    EXPECT_FALSE(isCoordinateMove("Qa8"));
    EXPECT_FALSE(isCoordinateMove("exd5"));

    // a promotion has an '=' of its own.
    auto promotion = parseEpdLine(R"(8/4P1k1/8/8/8/8/8/4K3 w - - bm e8=Q; id "promotion"; c0 "e8=Q=10, e8=N=1, Kd2=2";)");
    ASSERT_TRUE(promotion.has_value());
    EXPECT_EQ((std::vector<std::pair<std::string, int>>{ { "e8=Q", 10 }, { "e8=N", 1 }, { "Kd2", 2 } }),
              promotion->moveScores);

    // a few STS lines miss a comma, STS(v12.0) Center Control.026.
    auto missingComma = parseEpdLine(R"(2r3k1/p2npp2/p2p1bp1/3P4/1qN1PQ1P/1P2B3/r5P1/1R3R1K b - - bm Ne5; c0 "Ne5=10, a5=7, Bd4=10Qb8=6, Rc7=6"; id "STS(v12.0) Center Control.026";)");
    ASSERT_TRUE(missingComma.has_value());
    EXPECT_EQ((std::vector<std::pair<std::string, int>>{ { "Ne5", 10 }, { "a5", 7 }, { "Bd4", 10 }, { "Qb8", 6 }, { "Rc7", 6 } }),
              missingComma->moveScores);
}

TEST(EpdParser, NoMoves_Skipped) {
    EXPECT_FALSE(parseEpdLine("").has_value());
    EXPECT_FALSE(parseEpdLine(R"(8/8/8/8/8/8/8/K6k w - - id "nothing";)").has_value());
}
