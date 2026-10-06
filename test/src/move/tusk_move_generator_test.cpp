#include <gtest/gtest.h>
#include "elephant_test_logger.hpp"

#include <core/game_context.hpp>
#include <io/fen_parser.hpp>
#include <move/generation/king_pin_threats.hpp>
#include <move/generation/tusk/move_generator.hpp>
#include <system/clock.hpp>

#include <algorithm>
#include <string>
#include <vector>

namespace ElephantTest {
namespace {

template<Set us>
KingPinThreats<us> computePins(PositionReader position) {
    return KingPinThreats<us>(to_square(position.material().king<us>().lsbIndex()), position);
}

// Perft with bulk counting at depth 1, nothing but generation and make/unmake.
template<Set us>
u64 tuskPerft(GameContext& context, int depth) {
    PositionReader position = context.readChessPosition();
    KingPinThreats<us> pins = computePins<us>(position);
    tusk::MoveGenerator<us> generator(position, pins);
    tusk::MoveGenResult<us> moves = generator.generateAll();

    if (depth == 1)
        return moves.generatedCount();

    u64 nodes = 0;
    while (PrioritizedMove move = moves.next()) {
        context.MakeMove(move.move);
        nodes += tuskPerft<opposing_set<us>()>(context, depth - 1);
        context.UnmakeMove();
    }
    return nodes;
}

// Same as tuskPerft but walks the staged, lazy generate() path with ordering moves taken from the position itself.
template<Set us>
u64 tuskStagedPerft(GameContext& context, int depth) {
    PositionReader position = context.readChessPosition();
    KingPinThreats<us> pins = computePins<us>(position);

    // pick ordering moves out of the legal moves so pv/tt/killer dedupe is exercised.
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

template<typename TPerft>
u64 runPerft(const std::string& fen, int depth, TPerft whitePerft, TPerft blackPerft) {
    GameContext context;
    io::fen_parser::deserialize(fen.c_str(), context.editChessboard());
    return context.readToPlay() == Set::WHITE ? whitePerft(context, depth) : blackPerft(context, depth);
}

struct TuskPerftCase {
    std::string name;
    std::string fen;
    int depth;
    u64 expected;
};

const std::vector<TuskPerftCase> referencePositions = {
    { "start position", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609 },
    { "kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603 },
    { "position 3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5, 674624 },
    { "position 4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4, 422333 },
    { "position 5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4, 2103487 },
    { "position 6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 4, 3894594 },
    { "illegal enpassant 1", "3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1", 6, 1134888 },
    { "illegal enpassant 2", "8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1", 6, 1015133 },
    { "en passant capture, checks opponent", "8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1", 6, 1440467 },
    { "short castling", "5k2/8/8/8/8/8/8/4K2R w K - 0 1", 6, 661072 },
    { "long castling", "3k4/8/8/8/8/8/8/R3K3 w Q - 0 1", 6, 803711 },
    { "castling rights", "r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1", 4, 1274206 },
    { "castling prevented", "r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1", 4, 1720476 },
    { "promotion out of check", "2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1", 6, 3821001 },
    { "discovered check", "8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1", 5, 1004658 },
    { "promote to give check", "4k3/1P6/8/8/8/8/K7/8 w - - 0 1", 6, 217342 },
    { "under promote to give check", "8/P1k5/K7/8/8/8/8/8 w - - 0 1", 6, 92683 },
    { "self stalemate", "K1k5/8/P7/8/8/8/8/8 w - - 0 1", 6, 2217 },
    { "stalemate and checkmate 1", "8/k1P5/8/1K6/8/8/8/8 w - - 0 1", 7, 567584 },
    { "stalemate and checkmate 2", "8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1", 4, 23527 },
    { "bishop vs rook endgame", "1k6/1b6/8/8/7R/8/8/4K2R b K - 0 1", 5, 1063513 },
    { "two hundred million nodes", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 193690690, 5 },
    { "two hundred million nodes", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 178633661, 7 },
};

} // namespace

TEST(TuskMoveGenerator, PerftReferencePositions) {
    u64 totalNodes = 0;
    Clock clock;
    clock.Start();
    for (const auto& perftCase : referencePositions) {
        const u64 nodes = runPerft(perftCase.fen, perftCase.depth, &tuskPerft<Set::WHITE>, &tuskPerft<Set::BLACK>);
        EXPECT_EQ(perftCase.expected, nodes) << perftCase.name << " depth " << perftCase.depth;
        totalNodes += nodes;
    }
    clock.Stop();
    OUT() << " tusk perft nodes: " << totalNodes << " in " << clock.getElapsedTime() << " ms, "
          << clock.calcNodesPerSecond(totalNodes) << " nps";
}

// The lazy staged path, with pv/tt/killers set, must hand out exactly the same moves as generateAll().
TEST(TuskMoveGenerator, StagedMatchesUnordered) {
    for (const auto& perftCase : referencePositions) {
        const int depth = std::min(perftCase.depth, 3);
        const u64 unordered = runPerft(perftCase.fen, depth, &tuskPerft<Set::WHITE>, &tuskPerft<Set::BLACK>);
        const u64 staged = runPerft(perftCase.fen, depth, &tuskStagedPerft<Set::WHITE>, &tuskStagedPerft<Set::BLACK>);
        EXPECT_EQ(unordered, staged) << perftCase.name << " depth " << depth;
    }
}

TEST(TuskMoveGenerator, StagedHandsOutEachMoveOnce) {
    for (const auto& perftCase : referencePositions) {
        GameContext context;
        io::fen_parser::deserialize(perftCase.fen.c_str(), context.editChessboard());
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
        EXPECT_EQ(seen.end(), std::adjacent_find(seen.begin(), seen.end())) << perftCase.name;
        EXPECT_EQ(seen.size(), moves.searched().size()) << perftCase.name;
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
