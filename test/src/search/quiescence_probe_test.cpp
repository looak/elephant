#include <gtest/gtest.h>

#include <core/chessboard.hpp>
#include <io/fen_parser.hpp>
#include <search/quiescence_probe.hpp>

namespace ElephantTest {
/**
 * @file quiescence_probe_test.cpp
 * @brief What the tuner and the indexer rely on from QuiescenceProbe: the line, whether it's decided and the position
 * it leaves behind.
 * Naming convention: <TestedFunction>_<Condition>_<ExpectedBehavior>    */
////////////////////////////////////////////////////////////////

TEST(QuiescenceProbeTest, Search_NothingToCapture_HasNoLine)
{
    QuiescenceProbe probe;
    const Chessboard board = *io::fen_parser::read("4k3/4p3/8/8/8/8/4P3/4K3 w - - 0 1");

    const QuiescenceResult result = probe.search(board);

    EXPECT_EQ(0u, result.line.length);
    EXPECT_FALSE(result.decided);
}

TEST(QuiescenceProbeTest, Search_HangingQueen_LineTakesItAndThePositionStays)
{
    QuiescenceProbe probe;
    const Chessboard board = *io::fen_parser::read("4k3/8/8/3q4/8/8/8/3RK3 w - - 0 1");

    const QuiescenceResult result = probe.search(board);

    ASSERT_GT(result.line.length, 0u);
    EXPECT_EQ("d1d5", result.line.moves[0].toString());
    EXPECT_FALSE(result.decided);
    EXPECT_EQ(board.readPosition().hash(), probe.position().hash());
}

TEST(QuiescenceProbeTest, Search_Mated_IsDecided)
{
    QuiescenceProbe probe;
    const Chessboard board = *io::fen_parser::read("R6k/6pp/8/8/8/8/8/4K3 b - - 0 1");

    const QuiescenceResult result = probe.search(board);

    EXPECT_TRUE(result.decided);
    EXPECT_EQ(0u, result.line.length);
    EXPECT_EQ(board.readPosition().hash(), probe.position().hash());
}

TEST(QuiescenceProbeTest, PlayLine_KeepsTheAccumulatorInStep)
{
    QuiescenceProbe probe;
    const Chessboard board = *io::fen_parser::read("4k3/8/8/3q4/8/8/8/3RK3 w - - 0 1");

    const QuiescenceResult result = probe.search(board);
    probe.playLine(result.line);

    EXPECT_NE(board.readPosition().hash(), probe.position().hash());
    EXPECT_EQ(PestoAccumulator::computeFromScratch(probe.position().material()), probe.accumulator());
    EXPECT_EQ(0u, probe.search(*io::fen_parser::read("4k3/8/8/3R4/8/8/8/4K3 b - - 0 1")).line.length);
}

TEST(QuiescenceProbeTest, Search_AfterPlayLine_StartsOverFromTheBoard)
{
    QuiescenceProbe probe;
    const Chessboard first = *io::fen_parser::read("4k3/8/8/3q4/8/8/8/3RK3 w - - 0 1");
    probe.playLine(probe.search(first).line);

    const Chessboard second = *io::fen_parser::read("4k3/4p3/8/8/8/8/4P3/4K3 b - - 0 1");
    (void)probe.search(second);

    EXPECT_EQ(second.readPosition().hash(), probe.position().hash());
    EXPECT_EQ(PestoAccumulator::computeFromScratch(second.readPosition().material()), probe.accumulator());
}

////////////////////////////////////////////////////////////////

}  // namespace ElephantTest
