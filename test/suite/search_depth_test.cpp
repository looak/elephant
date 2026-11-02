#include <gtest/gtest.h>
#include "elephant_test_logger.hpp"

#include <io/fen_parser.hpp>
#include <core/game.hpp>
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
    SearchLimits testingParams;
    TimeManager timeManager;
};
////////////////////////////////////////////////////////////////

// Disabled, #50: since continuation history the engine plays g7g3, which wins as well as d4e3 (both about +6.7 at
// depth 30), so the position no longer tests whether pruning hides the d4e3 line.
TEST_F(SearchDepthFixture, DISABLED_NullMovePruning_DeepLine) {
    for (const auto& searchCase : s_nullMovePruningDeep) {
        Game context;
        context.loadFen(searchCase.fen.c_str());

        SearchContext searchContext;
        TranspositionTable tt;
        Search searcher(searchContext, tt);

        // d4e3 needs a full depth 9 search without pruning, null move pruning and LMR take plies off the lines that
        // matter. With both it was first found at depth 13, futility & reverse futility pruning pushed that to 17
        // (64 MB hash). 0.15.1's IIR, applied at every node without a TT or PV move now that PVs are cleared, finds it
        // at 19. With pawn structure in the evaluation it's found at 22, 23 leaves a ply of margin. Raise this when new
        // pruning or evaluation pushes it out of reach.
        testingParams.SearchDepth = 23;
        timeManager.applyTimeSettings(testingParams, context.readToPlay());
        SearchResult result = searcher.go(makeSearchRequest(context, testingParams), timeManager);

        EXPECT_EQ(searchCase.expectedMove, result.move().toString());
        OUT() << "Tested position: " << searchCase.fen;
        OUT() << "Expected move:   " << searchCase.expectedMove;
        OUT() << "Found move:      " << result.move().toString();
        OUT() << "-----------------------------";
    }
}

////////////////////////////////////////////////////////////////

}  // namespace ElephantTest
