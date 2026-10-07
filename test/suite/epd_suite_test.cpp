#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <format>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "elephant_gambit_config.h"
#include "core/game_context.hpp"
#include "io/fen_parser.hpp"
#include "io/san_parser.hpp"
#include "search/search.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <system/time_manager.hpp>

// EPD suites measure engine strength rather than correctness, so a few misses are expected.
// Each suite runs every position as one test and passes as long as enough positions are solved:
//   EG_EPD_MIN_PASSED=280  overrides the suite's minimum number of solved positions
//   EG_EPD_DEPTH=7         fixed depth, deterministic, no time limit
//   EG_EPD_MOVETIME=1000   milliseconds per position (default when neither is set)

struct EpdTestCase {
    std::string id;
    std::string fen;
    std::string bestMoveSan;
};

std::ostream& operator<<(std::ostream& os, const EpdTestCase& tc) {
return os << "\n  ID:       " << tc.id
            << "\n  FEN:      " << tc.fen
            << "\n  Expected: " << tc.bestMoveSan;
}

// Simple helper to parse the file
std::vector<EpdTestCase> loadEpdFile(const std::string& filePath) {
    std::vector<EpdTestCase> cases;
    std::ifstream file(filePath);
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        EpdTestCase tc;
        size_t bmPos = line.find(" bm ");
        size_t idPos = line.find(" id ");

        if (bmPos == std::string::npos || idPos == std::string::npos) {
            continue; // Not a valid test line
        }

        tc.fen = line.substr(0, bmPos);

        size_t bmEndPos = line.find(';', bmPos);
        tc.bestMoveSan = line.substr(bmPos + 4, bmEndPos - (bmPos + 4));

        size_t idEndPos = line.find(';', idPos);
        tc.id = line.substr(idPos + 4, idEndPos - (idPos + 4));
        tc.id.erase(std::remove(tc.id.begin(), tc.id.end(), '"'), tc.id.end());

        cases.push_back(tc);
    }
    return cases;
}

class EpdSuite : public ::testing::Test {
protected:
    static std::string makeRunLogFilename(const std::string& suiteName) {
        // timestamp or random-based name
        auto now = std::chrono::system_clock::now();
        auto t   = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        std::ostringstream oss;
        oss << "logs/" << suiteName << "_"
            << std::put_time(&tm, "%Y%m%d_%H%M") << ".log";
        return oss.str();
    }

    static void initializeLogger(const std::string& suiteName) {
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(makeRunLogFilename(suiteName), true);
        auto logger    = std::make_shared<spdlog::logger>("engine", file_sink);

        spdlog::set_default_logger(logger);
        spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v"); // high precision for search
        spdlog::set_level(spdlog::level::debug); // turn on the fire hose

        // Trace critical constants once
        spdlog::info("RUNTIME CONSTANTS Check:");
        spdlog::info("  c_checkmateConstant: {}", c_checkmateConstant);
        spdlog::info("  c_infinity: {}", c_infinity);
        spdlog::info("  MAX_i16: {}", std::numeric_limits<i16>::max());
    }

    void SetUp() override {
        const char* depthEnv = std::getenv("EG_EPD_DEPTH");
        const char* moveTimeEnv = std::getenv("EG_EPD_MOVETIME");
        if (depthEnv != nullptr) {
            params.SearchDepth = static_cast<u8>(std::atoi(depthEnv));
            params.MoveTime = 0;
        }
        else {
            params.MoveTime = moveTimeEnv != nullptr ? static_cast<u32>(std::atoi(moveTimeEnv)) : 1000;
        }
    }

    // Searches the position and returns true if the engine found one of the expected best moves.
    bool runCase(const EpdTestCase& tc) {
        GameContext context;

        spdlog::debug("-------------------------------------------------");
        spdlog::debug("Starting test case: {}", tc.id);
        spdlog::debug("Expecting {}, fen: {}", tc.bestMoveSan, tc.fen);

        // 1. Set up the position
        io::fen_parser::deserialize(tc.fen.c_str(), context.editChessboard());

        Search searcher(context);
        SearchResult result;

        std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
        // 2. Figure out who is to move and run the search
        if (context.readToPlay() == Set::WHITE) {
            timeManager.applyTimeSettings(params, Set::WHITE);
            result = searcher.go<Set::WHITE>(params, timeManager);
        } else {
            timeManager.applyTimeSettings(params, Set::BLACK);
            result = searcher.go<Set::BLACK>(params, timeManager);
        }
        std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsedSeconds = endTime - startTime;
        spdlog::debug("Search elapsed time: {} seconds", elapsedSeconds.count());

        // 3. Parse the expected move from SAN
        // there can be more than one "best move" in EPD.
        std::vector<std::string> sanMoves;
        size_t start = 0;
        size_t end = tc.bestMoveSan.find(' ');
        while (end != std::string::npos) {
            sanMoves.push_back(tc.bestMoveSan.substr(start, end - start));
            start = end + 1;
            end = tc.bestMoveSan.find(' ', start);
        }

        sanMoves.push_back(tc.bestMoveSan.substr(start, end - start)); // last move, or first if only one.

        std::vector<PackedMove> expectedMoves;
        for (const auto& sanMove : sanMoves) {
            try {
                expectedMoves.push_back(io::san_parser::deserialize(
                    context.readChessPosition(),
                    context.readToPlay() == Set::WHITE,
                    sanMove));
            }
            catch (...) {
                // a broken test case is a bug in the suite or the parser, never an expected miss
                ADD_FAILURE() << "SAN parser FAILED to parse the expected move: " << sanMove << " for test: " << tc.id;
                return false;
            }
        }

        // scouting statistics logging
        // fetching atomics
        u64 scoutCount = searcher.scout_search_count.load() == 0 ? 1 : searcher.scout_search_count.load(); // prevent div by zero
        u64 reSearchCount = searcher.scout_re_search_count.load();
        spdlog::debug("Scouting searches: {}, Re-searches: {} -- {}%", scoutCount, reSearchCount, 100.0 * reSearchCount / scoutCount);

        // 4. Check if the engine's move is among the expected moves
        for (const auto& expectedMove : expectedMoves) {
            if (expectedMove.toString() == result.move().toString()) {
                spdlog::debug("Test ID: {} passed. Expected one of moves: {} | Engine move: {}", tc.id, tc.bestMoveSan, result.move().toString());
                return true;
            }
        }

        spdlog::error("Test ID: {} FAILED! Expected one of moves: {} | Engine move: {}", tc.id, tc.bestMoveSan, result.move().toString());
        std::cout << std::format("  miss {:<12} expected {:<12} engine {:<6} score {:>6} nodes {:>10}  pv {}\n",
            tc.id, tc.bestMoveSan, result.move().toString(), result.score, result.count, result.pvLine.toString());
        return false;
    }

    // Runs every position in the file and expects at least minimumPassed of them to be solved.
    void runSuite(const std::string& suiteName, const std::string& epdFile, size_t minimumPassed) {
        const std::vector<EpdTestCase> cases = loadEpdFile(std::format("{}/res/{}", ROOT_PATH, epdFile));
        if (cases.empty()) {
            GTEST_SKIP() << "No positions found in res/" << epdFile;
        }

        if (const char* minimumEnv = std::getenv("EG_EPD_MIN_PASSED")) {
            minimumPassed = static_cast<size_t>(std::atoi(minimumEnv));
        }

        initializeLogger(suiteName);

        size_t passed = 0;
        for (const auto& tc : cases) {
            if (runCase(tc))
                ++passed;
        }

        const std::string summary = std::format("{}: {}/{} solved ({:.1f}%), minimum {}",
            suiteName, passed, cases.size(), 100.0 * passed / cases.size(), minimumPassed);
        spdlog::info(summary);
        std::cout << summary << "\n";

        EXPECT_GE(passed, minimumPassed) << summary;
    }

    SearchParameters params;
    TimeManager timeManager{params, Set::WHITE}; // Dummy TimeManager
};

// ~272/300 on windows-latest CI and 276/300 locally at 1000ms per move, the minimum leaves room for slower runners.
TEST_F(EpdSuite, WinAtChess) {
    runSuite("WinAtChess", "wac_new.epd", 260);
}

// No baseline yet, report only.
TEST_F(EpdSuite, Arasan21) {
    runSuite("Arasan21", "arasan21.epd", 0);
}
