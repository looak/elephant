#include <gtest/gtest.h>
#include "elephant_test_logger.hpp"

#include <core/game_context.hpp>
#include <io/fen_parser.hpp>
#include <move/generation/king_pin_threats.hpp>
#include <move/generation/move_gen_policy.hpp>
#include <move/generation/tusk/check_info.hpp>
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
tusk::CheckInfo<us> computePins(PositionReader position) {
    return tusk::CheckInfo<us>(position);
}

// Perft through the lazy staged generate() path, with ordering moves taken from the position itself so pv/tt/killer
// dedupe is exercised.
template<Set us>
u64 tuskStagedPerft(GameContext& context, int depth) {
    PositionReader position = context.readChessPosition();
    tusk::CheckInfo<us> pins = computePins<us>(position);

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
        tusk::CheckInfo<Set::WHITE> pins = computePins<Set::WHITE>(position);
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

// Qd2xd5 loses the queen to the c6 pawn, it's the only capture.
TEST(TuskMoveGenerator, DeferLosingCaptures_LosingCaptureComesAfterTheQuiets) {
    GameContext context;
    io::fen_parser::deserialize("4k3/8/2p5/3p4/8/8/3Q4/4K3 w - - 0 1", context.editChessboard());
    PositionReader position = context.readChessPosition();
    tusk::CheckInfo<Set::WHITE> pins = computePins<Set::WHITE>(position);
    const std::string losingCapture = "d2d5";

    auto handOutOrder = [&](bool defer) {
        tusk::MoveGenerator<Set::WHITE> generator(position, pins, { .deferLosingCaptures = defer });
        tusk::MoveGenResult<Set::WHITE> moves = generator.generate();
        std::vector<std::string> order;
        while (PrioritizedMove move = moves.next())
            order.push_back(move.move.toString());
        return order;
    };

    const std::vector<std::string> deferred = handOutOrder(true);
    const std::vector<std::string> inPlace = handOutOrder(false);

    ASSERT_FALSE(inPlace.empty());
    EXPECT_EQ(losingCapture, inPlace.front());
    EXPECT_EQ(losingCapture, deferred.back());

    // the same moves, each handed out once.
    std::vector<std::string> sortedDeferred = deferred, sortedInPlace = inPlace;
    std::sort(sortedDeferred.begin(), sortedDeferred.end());
    std::sort(sortedInPlace.begin(), sortedInPlace.end());
    EXPECT_EQ(sortedInPlace, sortedDeferred);
}

TEST(TuskMoveGenerator, DeferLosingCaptures_StagedHandsOutEachMoveOnce) {
    for (const std::string& fen : testPositions) {
        GameContext context;
        io::fen_parser::deserialize(fen.c_str(), context.editChessboard());
        if (context.readToPlay() != Set::WHITE)
            continue;

        PositionReader position = context.readChessPosition();
        tusk::CheckInfo<Set::WHITE> pins = computePins<Set::WHITE>(position);
        tusk::MoveGenerator<Set::WHITE> generator(position, pins, { .deferLosingCaptures = true });
        tusk::MoveGenResult<Set::WHITE> moves = generator.generate();
        tusk::MoveGenResult<Set::WHITE> all = generator.generateAll();

        std::vector<u16> seen;
        while (PrioritizedMove move = moves.next())
            seen.push_back(move.move.read());

        std::sort(seen.begin(), seen.end());
        EXPECT_EQ(seen.end(), std::adjacent_find(seen.begin(), seen.end())) << fen;
        EXPECT_EQ(all.generatedCount(), seen.size()) << fen;
        EXPECT_EQ(seen.size(), moves.searched().size()) << fen;
    }
}

TEST(TuskMoveGenerator, CapturesOnlyFilter) {
    GameContext context;
    io::fen_parser::deserialize("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", context.editChessboard());

    PositionReader position = context.readChessPosition();
    tusk::CheckInfo<Set::WHITE> pins = computePins<Set::WHITE>(position);
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

// pv, tt, captures, killers, then the remaining quiets.
TEST(TuskMoveGenerator, StagedOrdering) {
    GameContext context;
    io::fen_parser::deserialize("3k1r2/8/8/8/8/PPPP4/5Q2/1K6 w - - 0 1", context.editChessboard());

    PackedMove queenCapture(Square::F2, Square::F8);
    queenCapture.setCapture(true);
    const std::vector<PackedMove> expectedOrder = {
        PackedMove(Square::A3, Square::A4),    // pv
        PackedMove(Square::B3, Square::B4),    // tt
        queenCapture,                          // the only capture
        PackedMove(Square::C3, Square::C4),    // killer 1
        PackedMove(Square::D3, Square::D4),    // killer 2
    };

    MoveOrderingView view;
    view.pvMove = expectedOrder[0];
    view.ttMove = expectedOrder[1];
    view.killers[0] = expectedOrder[3];
    view.killers[1] = expectedOrder[4];

    PositionReader position = context.readChessPosition();
    tusk::CheckInfo<Set::WHITE> pins = computePins<Set::WHITE>(position);
    tusk::MoveGenerator<Set::WHITE> generator(position, pins, { .ordering = &view });
    tusk::MoveGenResult<Set::WHITE> moves = generator.generate();

    for (const PackedMove& expected : expectedOrder) {
        const PackedMove generated = moves.next().move;
        EXPECT_EQ(expected.read(), generated.read()) << "expected " << expected.toString()
            << " got " << (generated.isNull() ? std::string("null") : generated.toString());
    }

    // the remaining moves are quiets, none of the moves above again.
    while (PrioritizedMove move = moves.next()) {
        EXPECT_FALSE(move.move.isCapture()) << move.move.toString();
        for (const PackedMove& handedOut : expectedOrder)
            EXPECT_NE(handedOut.read(), move.move.read()) << move.move.toString() << " handed out twice";
    }
}

TEST(TuskMoveGenerator, PeekDoesNotConsume) {
    GameContext context;
    io::fen_parser::deserialize("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", context.editChessboard());

    PositionReader position = context.readChessPosition();
    tusk::CheckInfo<Set::WHITE> pins = computePins<Set::WHITE>(position);
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

namespace {
// Compares CheckInfo against the per direction KingPinThreats at every node of a tree.
template<Set us>
void verifyCheckInfoThroughTree(GameContext& context, int depth, std::vector<std::string>& line, std::string& firstFailure) {
    if (!firstFailure.empty())
        return;

    const PositionReader position = context.readChessPosition();
    const tusk::CheckInfo<us> checkInfo(position);
    const KingPinThreats<us> reference(to_square(position.material().king<us>().lsbIndex()), position);
    const u64 usMat = position.material().combine<us>().read();

    std::string mismatch;
    if (checkInfo.isChecked() != reference.isChecked())
        mismatch = "isChecked";
    else if (checkInfo.checkCount() != reference.isCheckedCount())
        mismatch = "checkCount";
    else if (checkInfo.checkCount() == 1 && checkInfo.checkMask() != reference.checks().read())
        mismatch = "checkMask";
    else if (checkInfo.pinned() != (reference.pins().read() & usMat))
        mismatch = "pinned";
    if (!mismatch.empty()) {
        for (const auto& m : line) firstFailure += m + " ";
        firstFailure += "(" + mismatch + ")";
        return;
    }

    if (depth == 0)
        return;

    move_gen_policy::Tusk::forEachMove<us>(position, [&](PackedMove move) {
        line.push_back(move.toString());
        context.MakeMove(move);
        verifyCheckInfoThroughTree<opposing_set<us>()>(context, depth - 1, line, firstFailure);
        context.UnmakeMove();
        line.pop_back();
    });
}
} // namespace

TEST(TuskCheckInfo, MatchesKingPinThreatsThroughTree) {
    // checks, double checks & pins along every line are all reached within these depths.
    const std::vector<std::pair<std::string, int>> positions = {
        { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3 },   // kiwipete
        { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5 },
        { "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 3 },
        { "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 3 },
        { "2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - - 0 1", 3 },             // WAC.001
    };

    for (const auto& [fen, depth] : positions) {
        GameContext context;
        io::fen_parser::deserialize(fen.c_str(), context.editChessboard());
        std::vector<std::string> line;
        std::string firstFailure;
        if (context.readToPlay() == Set::WHITE)
            verifyCheckInfoThroughTree<Set::WHITE>(context, depth, line, firstFailure);
        else
            verifyCheckInfoThroughTree<Set::BLACK>(context, depth, line, firstFailure);
        EXPECT_TRUE(firstFailure.empty()) << fen << "\n  CheckInfo differs after: " << firstFailure;
    }
}

} // namespace ElephantTest
