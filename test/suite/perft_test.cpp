#include <gtest/gtest.h>
#include "elephant_test_logger.hpp"

#include <io/fen_parser.hpp>
#include <core/game_context.hpp>
#include <move/generation/move_gen_policy.hpp>
#include <search/perft_search.hpp>
#include <system/clock.hpp>
#include <perft_cases.hpp>

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <memory>
#include <thread>
#include <vector>

// Perft on every core: each position is split two plies deep and the subtrees are shared out between the threads,
// each thread with a GameContext of its own. EG_PERFT_THREADS=1 runs it on one thread. The positions & their node
// counts are in perft_cases.hpp, the unit tests run them shallow.

namespace ElephantTest {
////////////////////////////////////////////////////////////////
class PerftFixture : public ::testing::Test {
};
//////////////////////////////////////////////////////////////

namespace {

struct PerftCase {
    const PerftPosition* position;
    int depth;
};

// a subtree: the moves from its case's position down to it & the depth left below it.
struct PerftJob {
    size_t caseIndex;
    std::vector<PackedMove> path;
    int depth;
};

unsigned perftThreads()
{
    if (const char* threads = std::getenv("EG_PERFT_THREADS"))
        return static_cast<unsigned>(std::max(1, std::atoi(threads)));
    return std::max(1u, std::thread::hardware_concurrency());
}

// perft doesn't use the transposition table, a context per thread at the default 64 MB adds up.
std::unique_ptr<GameContext> perftContext()
{
    auto context = std::make_unique<GameContext>();
    context->editTranspositionTable().resize(1);
    return context;
}

template<typename TMoveGen>
std::vector<PackedMove> legalMoves(GameContext& context)
{
    std::vector<PackedMove> moves;
    auto collect = [&moves](PackedMove move) { moves.push_back(move); };
    const PositionReader position = context.readChessboard().readPosition();
    if (context.readToPlay() == Set::WHITE)
        TMoveGen::template forEachMove<Set::WHITE>(position, collect);
    else
        TMoveGen::template forEachMove<Set::BLACK>(position, collect);
    return moves;
}

// a node with no moves adds no jobs, it has no nodes below it.
template<typename TMoveGen>
void splitJobs(GameContext& context, size_t caseIndex, std::vector<PackedMove>& path, int depth, int plies,
    std::vector<PerftJob>& jobs)
{
    if (plies == 0 || depth <= 1) {
        jobs.push_back({ caseIndex, path, depth });
        return;
    }
    for (PackedMove move : legalMoves<TMoveGen>(context)) {
        context.MakeMove<true>(move);
        path.push_back(move);
        splitJobs<TMoveGen>(context, caseIndex, path, depth - 1, plies - 1, jobs);
        path.pop_back();
        context.UnmakeMove();
    }
}

// the leaf node count of each case.
template<typename TMoveGen>
std::vector<u64> runPerft(const std::vector<PerftCase>& cases, unsigned threads)
{
    // the biggest cases first, so no thread is left with a deep subtree at the end.
    std::vector<size_t> order(cases.size());
    for (size_t i = 0; i < order.size(); ++i)
        order[i] = i;
    std::ranges::stable_sort(order, std::greater{}, [&cases](size_t i) {
        return cases[i].position->nodesAt(cases[i].depth);
    });

    std::vector<PerftJob> jobs;
    {
        auto context = perftContext();
        for (size_t caseIndex : order) {
            context->Reset();
            io::fen_parser::deserialize(cases[caseIndex].position->fen.c_str(), context->editChessboard());
            std::vector<PackedMove> path;
            splitJobs<TMoveGen>(*context, caseIndex, path, cases[caseIndex].depth, 2, jobs);
        }
    }

    std::vector<std::atomic<u64>> nodes(cases.size());
    std::atomic<size_t> next = 0;
    auto work = [&](GameContext& context) {
        for (size_t i = next++; i < jobs.size(); i = next++) {
            const PerftJob& job = jobs[i];
            context.Reset();
            io::fen_parser::deserialize(cases[job.caseIndex].position->fen.c_str(), context.editChessboard());
            for (PackedMove move : job.path)
                context.MakeMove<true>(move);
            PerftSearch perft(context);
            nodes[job.caseIndex] += perft.Run<TMoveGen>(job.depth).Nodes;
        }
    };

    std::vector<std::unique_ptr<GameContext>> contexts;
    for (unsigned t = 0; t < threads; ++t)
        contexts.push_back(perftContext());
    {
        std::vector<std::jthread> workers;
        for (auto& context : contexts)
            workers.emplace_back(work, std::ref(*context));
    }

    return std::vector<u64>(nodes.begin(), nodes.end());
}

template<typename TMoveGen>
void RunPositions(const std::vector<PerftCase>& cases)
{
    const unsigned threads = perftThreads();
    Clock clock;
    clock.Start();
    const std::vector<u64> nodes = runPerft<TMoveGen>(cases, threads);
    clock.Stop();

    u64 totalNodes = 0;
    for (size_t i = 0; i < cases.size(); ++i) {
        const PerftPosition& position = *cases[i].position;
        const u64 expected = position.nodesAt(cases[i].depth);
        totalNodes += nodes[i];
        if (nodes[i] == expected)
            OUT_PASSED() << position.name << ", depth " << cases[i].depth << ": " << nodes[i] << " nodes";
        else
            OUT_FAILED() << position.name << ", depth " << cases[i].depth << ": " << nodes[i] << " nodes, expected "
                         << expected << ", difference " << static_cast<i64>(nodes[i] - expected);
        EXPECT_EQ(expected, nodes[i]) << position.name << " at depth " << cases[i].depth << "\n  " << position.fen;
    }

    OUT() << "---------------------------------";
    OUT() << " ### AGGREGATE RESULTS (" << TMoveGen::name << ", " << threads << " threads) ###";
    OUT() << "  Total nodes:  - - - - - - - " << totalNodes << " nodes";
    OUT() << "  Total elapsed time: - - - - " << clock.getElapsedTime() << " ms";
    OUT() << "  Total nodes per second: - - " << clock.calcNodesPerSecond(totalNodes) << " nps";
    OUT() << "---------------------------------";
}

// every position at its deepest depth within c_perftReferenceBudget.
std::vector<PerftCase> referenceCases()
{
    std::vector<PerftCase> cases;
    for (const PerftPosition& position : perftPositions())
        cases.push_back({ &position, position.deepestWithin(c_perftReferenceBudget) });
    return cases;
}

// the positions listed deeper than the reference budget, at their deepest depth.
std::vector<PerftCase> deepCases()
{
    std::vector<PerftCase> cases;
    for (const PerftPosition& position : perftPositions()) {
        if (position.maxDepth() > position.deepestWithin(c_perftReferenceBudget))
            cases.push_back({ &position, position.maxDepth() });
    }
    return cases;
}

} // namespace

// templated on the generator, a new one is A/B tested by running the positions with its policy too.
TEST_F(PerftFixture, ReferencePositions)
{
    RunPositions<move_gen_policy::Tusk>(referenceCases());
}

// billions of nodes, about a minute on eight cores.
TEST_F(PerftFixture, DeepPositions)
{
    RunPositions<move_gen_policy::Tusk>(deepCases());
}

////////////////////////////////////////////////////////////////

}  // namespace ElephantTest
