#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <vector>
#include "elephant_gambit_config.h"
#include "core/game_context.hpp"
#include "io/fen_parser.hpp"
#include "io/san_parser.hpp"
#include "search/search.hpp"

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
    void SetUp() override {
        // Set a reasonable, fixed depth for all tests
        // You can't use your full time-managed search here.
        // params.SearchDepth = 8; // Or 8, 12... pick one and be consistent
        params.MoveTime = 2500; // 2.5 seconds per move
    }

    SearchParameters params;
    TimeManager timeManager{params, Set::WHITE}; // Dummy TimeManager
};

// The test itself
TEST_P(EpdCorrectness, FindBestMove) {
    EpdTestCase tc = GetParam();
    GameContext context;
    
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
    
    PackedMove expectedMove;
    ASSERT_NO_THROW({
        expectedMove = io::san_parser::deserialize(
            context.readChessPosition(),
            context.readToPlay() == Set::WHITE,
            tc.bestMoveSan
        );
    }) << "SAN parser FAILED to parse the expected move: " << tc.bestMoveSan << " for test: " << tc.id;
    
    EXPECT_EQ(expectedMove.toString(), result.move().toString())
        << "Test ID: " << tc.id;    
}

// 4. Instantiate the test suite
INSTANTIATE_TEST_SUITE_P(
    WinAtChess,
    EpdCorrectness,    
    ::testing::ValuesIn(loadEpdFile(std::format("{}/res/wac_new.epd", ROOT_PATH))),
    [](const ::testing::TestParamInfo<EpdTestCase>& info) {
        // Clean up the ID for use as a test name
        std::string name = info.param.id;
        name.erase(std::remove(name.begin(), name.end(), '"'), name.end());
        std::replace(name.begin(), name.end(), '.', '_');
        return name;
    }
);

INSTANTIATE_TEST_SUITE_P(
    Arasan21,
    EpdCorrectness,
    ::testing::ValuesIn(loadEpdFile(std::format("{}/res/arasan21.epd", ROOT_PATH))),    
    [](const ::testing::TestParamInfo<EpdTestCase>& info) {
        // Clean up the ID for use as a test name
        std::string name = info.param.id;
        name.erase(std::remove(name.begin(), name.end(), '"'), name.end());
        std::replace(name.begin(), name.end(), '.', '_');
        return name;
    }
);