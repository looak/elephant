#include <gtest/gtest.h>

#include <vector>

#include <core/game_context.hpp>
#include <eval/mobility.hpp>
#include <io/fen_parser.hpp>

namespace ElephantTest {

namespace {

// records what mobilityScore reports for every piece, see NoMobilityTrace.
struct RecordingTrace {
    struct Piece {
        u32 pieceId;
        u32 safeSquares;
        i32 sign;
    };
    std::vector<Piece> pieces;

    void mobility(u32 pieceId, u32 safeSquares, i32 sign) { pieces.push_back({ pieceId, safeSquares, sign }); }
};

RecordingTrace traceOf(const char* fen) {
    GameContext context;
    io::fen_parser::deserialize(fen, context.editChessboard());
    RecordingTrace trace;
    (void)mobilityScore(context.readChessPosition().material(), trace);
    return trace;
}

} // namespace

TEST(Mobility, KnightInTheCentreAndInTheCorner) {
    // e4 reaches all 8 of its squares, a1 only b3 & c2.
    const RecordingTrace centre = traceOf("4k3/8/8/8/4N3/8/8/4K3 w - - 0 1");
    ASSERT_EQ(1u, centre.pieces.size());
    EXPECT_EQ(knightId, centre.pieces[0].pieceId);
    EXPECT_EQ(8u, centre.pieces[0].safeSquares);
    EXPECT_EQ(1, centre.pieces[0].sign);

    const RecordingTrace corner = traceOf("4k3/8/8/8/8/8/8/N3K3 w - - 0 1");
    ASSERT_EQ(1u, corner.pieces.size());
    EXPECT_EQ(2u, corner.pieces[0].safeSquares);
}

TEST(Mobility, OwnPiecesAndEnemyPawnAttacksAreNotSafe) {
    // knight e4: d2 holds its own pawn, d6 & f6 are covered by black's pawn on e7. 8 - 1 - 2 = 5.
    const RecordingTrace trace = traceOf("4k3/4p3/8/8/4N3/8/3P4/4K3 w - - 0 1");
    ASSERT_EQ(1u, trace.pieces.size());
    EXPECT_EQ(5u, trace.pieces[0].safeSquares);
}

TEST(Mobility, SlidersStopAtPieces_BlackCountsNegative) {
    // black rook a8: along the rank b8 to d8 (the king on e8 blocks), down the file to a2 (white's pawn on a2 can be
    // taken, it isn't black's own). b8 c8 d8 a7 a6 a5 a4 a3 a2 = 9.
    const RecordingTrace trace = traceOf("r3k3/8/8/8/8/8/P7/4K3 b - - 0 1");
    ASSERT_EQ(1u, trace.pieces.size());
    EXPECT_EQ(rookId, trace.pieces[0].pieceId);
    EXPECT_EQ(9u, trace.pieces[0].safeSquares);
    EXPECT_EQ(-1, trace.pieces[0].sign);
}

TEST(Mobility, UntunedTablesScoreNothing) {
    // until tuned every mobility value is S(0, 0), mobility changes no evaluation.
    GameContext context;
    io::fen_parser::deserialize("r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3",
                                context.editChessboard());
    EXPECT_EQ(S(0, 0), mobilityScore(context.readChessPosition().material()));
}

}  // namespace ElephantTest
