#include <gtest/gtest.h>
#include "elephant_test_logger.hpp"

#include <io/fen_parser.hpp>
#include <core/game_context.hpp>
#include <move/generation/move_generator.hpp>
#include <search/search.hpp>
#include <search/perft_search.hpp>
#include <system/clock.hpp>

#include <format>
#include <source_location>

namespace ElephantTest {
////////////////////////////////////////////////////////////////
class PerftFixture : public ::testing::Test {
};
//////////////////////////////////////////////////////////////

struct PerftCaseArgs {
    bool enabled;
    std::string testId;
    std::string fen;
    u32 expectedNodeCount;
    u8 searchDepth;

    // Auto-fill testId from the caller when not provided
    PerftCaseArgs(bool enabled_, std::string fen_, u32 expectedNodes_, u8 depth_, const std::source_location& loc = std::source_location::current()) 
        : enabled(enabled_)
        , testId(currentCallerName(loc))
        , fen(std::move(fen_))
        , expectedNodeCount(expectedNodes_)
        , searchDepth(depth_)
        {}

    // Explicit testId provided
    PerftCaseArgs(bool enabled_, std::string testId_, std::string fen_, u32 expectedNodes_, u8 depth_)
        : enabled(enabled_)
        , testId(std::move(testId_))
        , fen(std::move(fen_))
        , expectedNodeCount(expectedNodes_)
        , searchDepth(depth_)
        {}
private:
    static std::string currentCallerName(const std::source_location& loc) {
        if (auto* info = ::testing::UnitTest::GetInstance()->current_test_info()) {
            return std::string(info->test_suite_name()) + "." + info->name();
        }
        return std::string(loc.function_name());
    }
};

template<typename TMoveGen>
PerftResult ExecutePerftCase(const std::string& fen, int atDepth)
{
    // setup
    GameContext context;
    io::fen_parser::deserialize(fen.c_str(), context.editChessboard());

    // do
    PerftSearch perft(context);
    return perft.Run<TMoveGen>(atDepth);
}

template<typename TMoveGen>
PerftResult ExecutePerftTestCase(PerftCaseArgs perftCase, int number, int total)
{
    std::string testNumber = std::format("{}/{}", number, total);
    if (!perftCase.enabled) {
        OUT_ID(testNumber) << "Test Disabled: " << perftCase.testId;
        OUT_ID(testNumber) << "---------------------------------";
        return {};
    }

    Clock caseClock;
    caseClock.Start();

    OUT_ID(testNumber) << "Running test:     " << perftCase.testId << " (" << TMoveGen::name << ")";
    caseClock.Start();
    auto result = ExecutePerftCase<TMoveGen>(perftCase.fen, perftCase.searchDepth);
    caseClock.Stop();    
    result.NPS = caseClock.calcNodesPerSecond(result.AccNodes);
    OUT() << " Nodes: - - - - - - - " << result.Nodes << " nodes";
    OUT() << " Total nodes: - - - - " << result.AccNodes << " nodes";
    OUT() << " Nodes per second: - - " << result.NPS << " nps";
    OUT() << " Elapsed time: - - - - " << caseClock.getElapsedTime() << " ms";
    //EXPECT_EQ(perftCase.expectedNodeCount, result.Nodes);
    result.Passed = (perftCase.expectedNodeCount == result.Nodes);
    if (result.Passed) {
        OUT_PASSED() << "Expected nodes matched: " << perftCase.expectedNodeCount;
    }
    else {
        OUT_FAILED() << "Expected nodes: " << perftCase.expectedNodeCount << ", but got: " << result.Nodes;
        OUT_FAILED() << "Difference : " << static_cast<i64>(result.Nodes) - static_cast<i64>(perftCase.expectedNodeCount);
    }
    OUT_ID(testNumber) << "---------------------------------";
    return result;
}

std::vector<PerftCaseArgs> ReferencePositionCases()
{
    return {
        { true, "illegal enpassant", "3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1", 1134888, 6 },
        { true, "illegal enpassant", "8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1", 1015133, 6 },
        { true, "en passant capture, checks opponent", "8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1", 1440467, 6 },
        { true, "short castling", "5k2/8/8/8/8/8/8/4K2R w K - 0 1", 661072, 6 },
        { true, "long castling", "3k4/8/8/8/8/8/8/R3K3 w Q - 0 1", 803711, 6 },
        { true, "castling rights", "r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1", 1274206, 4 },
        { true, "castling prevented", "r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1", 1720476, 4 },
        { true, "promotion out of check", "2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1", 3821001, 6 },
        { true, "discovered check", "8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1", 1004658, 5 },
        { true, "promote to give check", "4k3/1P6/8/8/8/8/K7/8 w - - 0 1", 217342, 6 },
        { true, "under promote to give check", "8/P1k5/K7/8/8/8/8/8 w - - 0 1", 92683, 6 },
        { true, "self stalemate", "K1k5/8/P7/8/8/8/8/8 w - - 0 1", 2217, 6 },
        { true, "stalemate and checkmate", "8/k1P5/8/1K6/8/8/8/8 w - - 0 1", 567584, 7 },
        { true, "stalemate and checkmate", "8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1", 23527, 4 },
        { true, "two hundred million nodes kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 193690690, 5 },
        { true, "two hundred million nodes", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 178633661, 7 },
        { true, "fifteen million nodes", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 15833292, 5 },
        { true, "bishop vs rook endgame", "1k6/1b6/8/8/7R/8/8/4K2R b K - 0 1", 1063513, 5 },
    };
}

template<typename TMoveGen>
void RunReferencePositions(const std::vector<PerftCaseArgs>& perftTestCases)
{
    Clock clock;
    clock.Start();
    u64 totalNodes = 0;
    u64 totalNps = 0;
    bool testsPassed = true;
    int testCount = 1;
    std::vector<std::tuple<PerftResult, PerftCaseArgs>> results;
    for (auto& perftCase : perftTestCases) {
        auto result = ExecutePerftTestCase<TMoveGen>(perftCase, testCount, static_cast<int>(perftTestCases.size()));
        totalNodes += result.Nodes;
        totalNps += result.NPS;
        results.push_back({ result, perftCase });
        if (perftCase.enabled) testsPassed &= result.Passed;
        testCount++;
    }
    clock.Stop();

    for (const auto& [result, perftCase] : results) {
        if (perftCase.enabled == false) continue;
        EXPECT_EQ(perftCase.expectedNodeCount, result.Nodes) << " TestId: " << perftCase.testId;
    }

    u64 nps = clock.calcNodesPerSecond(totalNodes);
    i64 elapsedTime = clock.getElapsedTime();
    OUT() << "---------------------------------";
    OUT() << " ### AGGREGATE RESULTS (" << TMoveGen::name << ") ###";
    OUT() << "  Total nodes:  - - - - - - - " << totalNodes << " nodes";
    OUT() << "  Total elapsed time: - - - - " << elapsedTime << " ms";
    OUT() << "  Total nodes per second: - - " << nps << " nps";
    OUT() << "  Average nodes per second: - " << totalNps / perftTestCases.size() << " nps";
    OUT() << "---------------------------------";

    EXPECT_TRUE(testsPassed);
}

TEST_F(PerftFixture, EstablishedReferencePositions)
{
    RunReferencePositions<move_gen_policy::Legacy>(ReferencePositionCases());
}

TEST_F(PerftFixture, EstablishedReferencePositions_Tusk)
{
    RunReferencePositions<move_gen_policy::Tusk>(ReferencePositionCases());
}

// Runs every enabled reference position on both generators, verifies they agree and prints a speed comparison.
// Cases above abMaxNodes are skipped to keep the comparison quick.
TEST_F(PerftFixture, MoveGeneratorAB)
{
    constexpr u64 abMaxNodes = 500'000'000;
    u64 totalNodes = 0;
    i64 legacyTotalMs = 0;
    i64 tuskTotalMs = 0;

    OUT() << std::format(" {:<40} {:>10} {:>12} {:>12} {:>8}", "case", "nodes", "legacy nps", "tusk nps", "speedup");
    for (const auto& perftCase : ReferencePositionCases()) {
        if (!perftCase.enabled || perftCase.expectedNodeCount > abMaxNodes)
            continue;

        Clock legacyClock;
        legacyClock.Start();
        PerftResult legacy = ExecutePerftCase<move_gen_policy::Legacy>(perftCase.fen, perftCase.searchDepth);
        legacyClock.Stop();

        Clock tuskClock;
        tuskClock.Start();
        PerftResult tusk = ExecutePerftCase<move_gen_policy::Tusk>(perftCase.fen, perftCase.searchDepth);
        tuskClock.Stop();

        EXPECT_EQ(legacy.Nodes, tusk.Nodes) << " TestId: " << perftCase.testId;
        EXPECT_EQ(legacy.Captures, tusk.Captures) << " TestId: " << perftCase.testId;
        EXPECT_EQ(legacy.Castles, tusk.Castles) << " TestId: " << perftCase.testId;
        EXPECT_EQ(legacy.Promotions, tusk.Promotions) << " TestId: " << perftCase.testId;

        const u64 legacyNps = legacyClock.calcNodesPerSecond(legacy.AccNodes);
        const u64 tuskNps = tuskClock.calcNodesPerSecond(tusk.AccNodes);
        const double speedup = legacyNps > 0 ? static_cast<double>(tuskNps) / static_cast<double>(legacyNps) : 0.0;
        OUT() << std::format(" {:<40} {:>10} {:>12} {:>12} {:>7.2f}x", perftCase.testId, tusk.Nodes, legacyNps, tuskNps, speedup);

        totalNodes += tusk.AccNodes;
        legacyTotalMs += legacyClock.getElapsedTime();
        tuskTotalMs += tuskClock.getElapsedTime();
    }

    OUT() << "---------------------------------";
    OUT() << " legacy: " << legacyTotalMs << " ms, tusk: " << tuskTotalMs << " ms, total nodes: " << totalNodes;
    if (tuskTotalMs > 0)
        OUT() << std::format(" overall speedup: {:.2f}x", static_cast<double>(legacyTotalMs) / static_cast<double>(tuskTotalMs));
}

////////////////////////////////////////////////////////////////

}  // namespace ElephantTest