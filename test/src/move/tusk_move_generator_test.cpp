#include <gtest/gtest.h>
#include "elephant_test_logger.hpp"

#include <core/game_context.hpp>
#include <io/fen_parser.hpp>
#include <move/generation/king_pin_threats.hpp>
#include <move/generation/move_gen_policy.hpp>
#include <move/generation/tusk/move_generator.hpp>
#include <search/perft_search.hpp>

#include <algorithm>
#include <string>
#include <vector>

// Perft correctness & speed of the tusk generator is covered by perft_test.cpp through move_gen_policy::Tusk.
// These tests cover what perft can't see: the staged & ordered generate() path, filters and peek.

namespace ElephantTest {
namespace {

template<Set us>
KingPinThreats<us> computePins(PositionReader position) {
    return KingPinThreats<us>(to_square(position.material().king<us>().lsbIndex()), position);
}

// Perft through the lazy staged generate() path, with ordering moves taken from the position itself so pv/tt/killer
// dedupe is exercised.
template<Set us>
u64 tuskStagedPerft(GameContext& context, int depth) {
    PositionReader position = context.readChessPosition();
    KingPinThreats<us> pins = computePins<us>(position);

    MoveOrderingView ordering;
    {
        tusk::MoveGenerator<us> allGenerator(position, pins);
        tusk::MoveGenResult<us> all = allGenerator.generateAll();
        std::vector<PackedMove> legal;
        while (PrioritizedMove move = all.next())
            legal.push_back(move.move);

        if (!legal.empty()) {
            ordering.pvMove = legal.front();
            ordering.ttMove = legal.back();
        }
        u32 killerIndx = 0;
        for (PackedMove move : legal) {
            if (killerIndx < 2 && !move.isCapture() && !move.isPromotion())
                ordering.killers[killerIndx++] = move;
        }
        for (; killerIndx < 2; ++killerIndx)
            ordering.killers[killerIndx] = PackedMove::NullMove();
    }

    tusk::MoveGenerator<us> generator(position, pins, { .ordering = &ordering });
    tusk::MoveGenResult<us> moves = generator.generate();

    u64 nodes = 0;
    while (PrioritizedMove move = moves.next()) {
        if (depth == 1) {
            ++nodes;
            continue;
        }
        context.MakeMove(move.move);
        nodes += tuskStagedPerft<opposing_set<us>()>(context, depth - 1);
        context.UnmakeMove();
    }
    return nodes;
}

const std::vector<std::string> testPositions = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
    "3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1",
    "8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1",
    "r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1",
    "2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1",
    "8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1",
};

} // namespace

// The lazy staged path, with pv/tt/killers set, must hand out exactly the same moves as generateAll().
TEST(TuskMoveGenerator, StagedMatchesUnordered) {
    constexpr int depth = 3;
    for (const std::string& fen : testPositions) {
        GameContext context;
        io::fen_parser::deserialize(fen.c_str(), context.editChessboard());

        PerftSearch perft(context);
        const u64 unordered = perft.Run<move_gen_policy::Tusk>(depth).Nodes;
        const u64 staged = context.readToPlay() == Set::WHITE
            ? tuskStagedPerft<Set::WHITE>(context, depth)
            : tuskStagedPerft<Set::BLACK>(context, depth);
        EXPECT_EQ(unordered, staged) << fen;
    }
}

TEST(TuskMoveGenerator, StagedHandsOutEachMoveOnce) {
    for (const std::string& fen : testPositions) {
        GameContext context;
        io::fen_parser::deserialize(fen.c_str(), context.editChessboard());
        if (context.readToPlay() != Set::WHITE)
            continue;

        PositionReader position = context.readChessPosition();
        KingPinThreats<Set::WHITE> pins = computePins<Set::WHITE>(position);
        tusk::MoveGenerator<Set::WHITE> generator(position, pins);
        tusk::MoveGenResult<Set::WHITE> moves = generator.generate();

        std::vector<u16> seen;
        while (PrioritizedMove move = moves.next())
            seen.push_back(move.move.read());

        std::sort(seen.begin(), seen.end());
        EXPECT_EQ(seen.end(), std::adjacent_find(seen.begin(), seen.end())) << fen;
        EXPECT_EQ(seen.size(), moves.searched().size()) << fen;
    }
}

TEST(TuskMoveGenerator, CapturesOnlyFilter) {
    GameContext context;
    io::fen_parser::deserialize("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", context.editChessboard());

    PositionReader position = context.readChessPosition();
    KingPinThreats<Set::WHITE> pins = computePins<Set::WHITE>(position);
    tusk::MoveGenerator<Set::WHITE> generator(position, pins, { .moveFilter = MoveTypes::CAPTURES_ONLY });
    tusk::MoveGenResult<Set::WHITE> moves = generator.generate();

    u32 count = 0;
    u16 lastPriority = 0x7FFF;
    while (PrioritizedMove move = moves.next()) {
        EXPECT_TRUE(move.move.isCapture() || move.move.isPromotion()) << move.move.toString();
        EXPECT_LE(move.priority, lastPriority) << "captures should come out best first";
        lastPriority = move.priority;
        ++count;
    }
    // kiwipete has 8 captures for white at the root.
    EXPECT_EQ(8u, count);
}

TEST(TuskMoveGenerator, PeekDoesNotConsume) {
    GameContext context;
    io::fen_parser::deserialize("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", context.editChessboard());

    PositionReader position = context.readChessPosition();
    KingPinThreats<Set::WHITE> pins = computePins<Set::WHITE>(position);
    tusk::MoveGenerator<Set::WHITE> generator(position, pins);
    tusk::MoveGenResult<Set::WHITE> moves = generator.generate();

    u32 count = 0;
    while (true) {
        PackedMove peeked = moves.peek();
        PrioritizedMove move = moves.next();
        EXPECT_EQ(peeked, move.move);
        if (!move)
            break;
        ++count;
    }
    EXPECT_EQ(48u, count);
}

} // namespace ElephantTest
