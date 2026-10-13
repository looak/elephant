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
//   Smoke*     about a minute, before every merge:  ElephantSuite --gtest_filter=PerftFixture.*:EpdSuite.Smoke*
//   Extended*  about ten minutes, before a release or merging a search change:  --gtest_filter=EpdSuite.Extended*
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

// "f5=10, Be5+=2, Bf2=3" as move & points, empty when the comment isn't a list of scored moves.
std::vector<std::pair<std::string, int>> parseMoveScores(std::string text) {
    text.erase(std::remove(text.begin(), text.end(), '"'), text.end());
    std::vector<std::pair<std::string, int>> scores;
    std::istringstream entries(text);
    for (std::string entry; std::getline(entries, entry, ',');) {
        entry = trim(entry);
        const size_t equals = entry.find('=');
        if (equals == std::string::npos || equals == 0)
            return {};
        try {
            scores.emplace_back(entry.substr(0, equals), std::stoi(entry.substr(equals + 1)));
        }
        catch (const std::exception&) {
            return {};
        }
    }
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
        for (const auto& [san, value] : tc.moveScores) {
            if (contains(parseSanMoves(context, { san }, tc)))
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

    // Scored suites like STS: every file is a theme, a position earns the points of the move played. Expects a total of
    // at least minimumScore points, reports the score per theme.
    void runScoredSuite(const std::string& suiteName, const std::vector<std::string>& epdFiles, EpdSettings tierSettings,
                        long long minimumScore) {
        const EpdSettings settings = applyEnvironment(tierSettings);
        if (const char* minimumEnv = std::getenv("EG_EPD_MIN_PASSED"))
            minimumScore = std::atoll(minimumEnv);

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

        const std::string summary = std::format("{}: {}/{} points ({:.1f}%), best move {}/{}, minimum {}, {}, {:.0f} s",
            suiteName, score, maxScore, maxScore ? 100.0 * score / maxScore : 0.0, solved, total, minimumScore,
            settings.describe(), seconds);
        spdlog::info(summary);
        std::cout << summary << "\n";
        appendResult(suiteName, settings, solved, total, seconds, score, maxScore);
        EXPECT_GE(score, minimumScore) << summary;
    }

    // Runs every position in res/epdFile and expects at least minimumSolved of them to be solved.
    void runSuite(const std::string& suiteName, const std::string& epdFile, EpdSettings tierSettings, size_t minimumSolved) {
        const std::vector<EpdTestCase> cases = loadEpdFile(std::format("{}/res/{}", ROOT_PATH, epdFile));
        if (cases.empty()) {
            GTEST_SKIP() << "No positions found in res/" << epdFile;
        }
        const EpdSettings settings = applyEnvironment(tierSettings);
        if (const char* minimumEnv = std::getenv("EG_EPD_MIN_PASSED"))
            minimumSolved = static_cast<size_t>(std::atoi(minimumEnv));

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

        const std::string summary = std::format("{}: {}/{} solved ({:.1f}%), minimum {}, {}, {:.0f} s", suiteName,
            solved, cases.size(), 100.0 * solved / cases.size(), minimumSolved, settings.describe(), seconds);
        spdlog::info(summary);
        std::cout << summary << "\n";
        appendResult(suiteName, settings, solved, cases.size(), seconds);
        EXPECT_GE(solved, minimumSolved) << summary;
    }
};

// --- Smoke tier, about a minute ---

// 270 at 1.0.0-herd.7.
TEST_F(EpdSuite, Smoke_WinAtChess) {
    runSuite("Smoke_WinAtChess", "wac_new.epd", { .nodes = 500'000 }, 265);
}

// --- Extended tier, about ten minutes ---

// 282 at 1.0.0-herd.7.
TEST_F(EpdSuite, Extended_WinAtChess) {
    runSuite("Extended_WinAtChess", "wac_new.epd", { .nodes = 2'000'000 }, 277);
}

// Lazy SMP, the threads share the node limit. Not deterministic, the minimum leaves more room. 282 at 1.0.0-herd.7.
TEST_F(EpdSuite, Extended_WinAtChess_4Threads) {
    runSuite("Extended_WinAtChess_4Threads", "wac_new.epd", { .nodes = 8'000'000, .threads = 4 }, 272);
}

// Arasan is built for minutes per position, at 2M nodes only a few fall. 8 at 1.0.0-herd.7.
TEST_F(EpdSuite, Extended_Arasan2024) {
    runSuite("Extended_Arasan2024", "arasan2024.epd", { .nodes = 2'000'000 }, 6);
}

// 6 at 1.0.0-herd.7.
TEST_F(EpdSuite, Extended_Arasan21) {
    runSuite("Extended_Arasan21", "arasan21.epd", { .nodes = 2'000'000 }, 4);
}

// Strategic Test Suite, positional judgement rather than tactics. Each position scores the move played, 10 for the best.
TEST_F(EpdSuite, Extended_STS) {
    std::vector<std::string> files;
    for (int theme = 1; theme <= 15; ++theme) {
        if (std::filesystem::exists(std::format("{}/res/STS{}.epd", ROOT_PATH, theme)))
            files.push_back(std::format("STS{}.epd", theme));
    }
    if (files.empty())
        GTEST_SKIP() << "No STS files in res/";
    // 7837/13000 at 1.0.0-herd.7, STS 1-13.
    runScoredSuite("Extended_STS", files, { .nodes = 200'000 }, 7700);
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
}

TEST(EpdParser, NoMoves_Skipped) {
    EXPECT_FALSE(parseEpdLine("").has_value());
    EXPECT_FALSE(parseEpdLine(R"(8/8/8/8/8/8/8/K6k w - - id "nothing";)").has_value());
}
