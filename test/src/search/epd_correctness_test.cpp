#include <gtest/gtest.h>
#include <fstream>
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

        cases.push_back(tc);
    }
    return cases;
}
class EpdCorrectness : public ::testing::TestWithParam<EpdTestCase> {
protected:
    static std::string s_runLogFilename;
    static bool s_loggerInitialized;

    static std::string makeRunLogFilename(const char* suiteName) {
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

    static void ensureLoggerInitialized() {
        if (s_loggerInitialized)
            return;

        const ::testing::TestInfo* const test_info =
            ::testing::UnitTest::GetInstance()->current_test_info();
        // Use the suite name to distinguish WinAtChess vs Arasan21 runs
        std::string suiteName = test_info ? test_info->test_suite_name() : "EpdCorrectness";

        s_runLogFilename = makeRunLogFilename(suiteName.c_str());

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(s_runLogFilename, true);
        auto logger    = std::make_shared<spdlog::logger>("engine", file_sink);

        spdlog::set_default_logger(logger);
        spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v"); // high precision for search
        spdlog::set_level(spdlog::level::debug); // turn on the fire hose

        s_loggerInitialized = true;

        // Trace critical constants once
        spdlog::info("RUNTIME CONSTANTS Check:");
        spdlog::info("  c_checkmateConstant: {}", c_checkmateConstant);
        spdlog::info("  c_infinity: {}", c_infinity);
        spdlog::info("  MAX_i16: {}", std::numeric_limits<i16>::max());
    }

    void SetUp() override {
        ensureLoggerInitialized();
        // params.SearchDepth = 10; // reasonable depth for tests
        params.MoveTime = 1000; // 0.25 second per move
    }
    void TearDown() override {
        // Flush and release the logger so the file is closed properly
        spdlog::debug("-------------------------------------------------");
     //   spdlog::shutdown(); 
    }

    SearchParameters params;
    TimeManager timeManager{params, Set::WHITE}; // Dummy TimeManager
};

// The test itself
TEST_P(EpdCorrectness, FindBestMove) {
    EpdTestCase tc = GetParam();
    GameContext context;
    
    spdlog::debug("Starting test case: {}", tc.id);
    spdlog::debug("Expecting {}, fen: {}", tc.bestMoveSan, tc.fen);

    // 1. Set up the position
    io::fen_parser::deserialize(tc.fen.c_str(), context.editChessboard());

    Search searcher(context);
    SearchResult result;
    
    // 2. Figure out who is to move and run the search
    if (context.readToPlay() == Set::WHITE) {
        timeManager.applyTimeSettings(params, Set::WHITE);
        result = searcher.go<Set::WHITE>(params, timeManager);
    } else {
        timeManager.applyTimeSettings(params, Set::BLACK);
        result = searcher.go<Set::BLACK>(params, timeManager);
    }

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
        PackedMove pmove;
        ASSERT_NO_THROW({
            pmove = io::san_parser::deserialize(
                context.readChessPosition(),
                context.readToPlay() == Set::WHITE,
                sanMove
            );
        }) << "SAN parser FAILED to parse the expected move: " << sanMove << " for test: " << tc.id;
        expectedMoves.push_back(pmove);
    }

    // scouting statistics logging
    // fetching atomics 
    u64 scoutCount = searcher.scout_search_count.load() == 0 ? 1 : searcher.scout_search_count.load(); // prevent div by zero
    u64 reSearchCount = searcher.scout_re_search_count.load();
    spdlog::debug("Scouting searches: {}, Re-searches: {} -- {}%", scoutCount, reSearchCount, (reSearchCount / scoutCount) * 100);

    // 4. Check if the engine's move is among the expected moves
    bool moveFound = false;
    for (const auto& expectedMove : expectedMoves) {
        if (expectedMove.toString() == result.move().toString()) {
            moveFound = true;
            spdlog::debug("Test ID: {} passed. Expected one of moves: {} | Engine move: {}", tc.id, tc.bestMoveSan, result.move().toString());
            break;
        }
    }

    if (moveFound == false) {        
        spdlog::error("Test ID: {} FAILED! Expected one of moves: {} | Engine move: {}", tc.id, tc.bestMoveSan, result.move().toString());
    }

    EXPECT_TRUE(moveFound) << "Test ID: " << tc.id;    
}

// static definitions
std::string EpdCorrectness::s_runLogFilename;
bool EpdCorrectness::s_loggerInitialized = false;

// 4. Instantiate the test suite
INSTANTIATE_TEST_SUITE_P(
    WinAtChess,
    EpdCorrectness,    
    ::testing::ValuesIn(loadEpdFile(std::format("{}/res/wac_new.epd", ROOT_PATH))),
    [](const ::testing::TestParamInfo<EpdTestCase>& wac_info) {
        // Clean up the ID for use as a test name
        std::string name = wac_info.param.id;
        name.erase(std::remove(name.begin(), name.end(), '"'), name.end());
        std::replace(name.begin(), name.end(), '.', '_');
        return name;
    }
);

INSTANTIATE_TEST_SUITE_P(
    Arasan21,
    EpdCorrectness,
    ::testing::ValuesIn(loadEpdFile(std::format("{}/res/arasan21.epd", ROOT_PATH))),    
    [](const ::testing::TestParamInfo<EpdTestCase>& arasan_info) {
        // Clean up the ID for use as a test name
        std::string name = arasan_info.param.id;
        name.erase(std::remove(name.begin(), name.end(), '"'), name.end());
        std::replace(name.begin(), name.end(), '.', '_');
        return name;
    }
);

