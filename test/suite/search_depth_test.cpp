#include <gtest/gtest.h>
#include "elephant_test_logger.hpp"

#include <io/fen_parser.hpp>
#include <core/game_context.hpp>
#include <search/search.hpp>
#include <search_cases.hpp>
#include <system/time_manager.hpp>

// Searches too deep for the unit tests: moves the engine only finds seconds into a search.

namespace ElephantTest {
////////////////////////////////////////////////////////////////
class SearchDepthFixture : public ::testing::Test {
public:
    SearchDepthFixture() :
        timeManager(testingParams, Set::WHITE)
    {}
    void SetUp() override {
        testingParams.ThreadCount = 1;
    }

    SearchParameters testingParams;
    TimeManager timeManager;
};
////////////////////////////////////////////////////////////////

TEST_F(SearchDepthFixture, NullMovePruning_DeepLine) {
    for (const auto& searchCase : s_nullMovePruningDeep) {
        GameContext context;
        io::fen_parser::deserialize(searchCase.fen.c_str(), context.editChessboard());

        Search searcher(context);

        // d4e3 needs a full depth 9 search without pruning, null move pruning and LMR take plies off the lines that
        // matter. With both it was first found at depth 13, futility & reverse futility pruning pushed that to 17
        // (64 MB hash). 0.15.1's IIR, applied at every node without a TT or PV move now that PVs are cleared, finds it
        // at 19. With pawn structure in the evaluation it's found at 22, 23 leaves a ply of margin. Raise this when new
        // pruning or evaluation pushes it out of reach.
        testingParams.SearchDepth = 23;
        SearchResult result;
        if (context.readToPlay() == Set::BLACK) {
            timeManager.applyTimeSettings(testingParams, Set::BLACK);
            result = searcher.go<Set::BLACK>(testingParams, timeManager);
        }
        else {
            timeManager.applyTimeSettings(testingParams, Set::WHITE);
            result = searcher.go<Set::WHITE>(testingParams, timeManager);
        }

        EXPECT_EQ(searchCase.expectedMove, result.move().toString());
        OUT() << "Tested position: " << searchCase.fen;
        OUT() << "Expected move:   " << searchCase.expectedMove;
        OUT() << "Found move:      " << result.move().toString();
        OUT() << "-----------------------------";
    }
}

////////////////////////////////////////////////////////////////

}  // namespace ElephantTest
