#include <gtest/gtest.h>

#include <vector>

#include <core/game_context.hpp>
#include <eval/pieces.hpp>
#include <io/fen_parser.hpp>

namespace ElephantTest {

namespace {

// records what pieceScore reports, see NoPieceTrace.
struct RecordingTrace {
    struct Entry {
        u32 pieceId;
        i32 sign;
    };
    std::vector<Entry> entries;

    void piecePair(u32 pieceId, i32 sign) { entries.push_back({ pieceId, sign }); }

    [[nodiscard]] std::size_t count(u32 pieceId, i32 sign) const {
        std::size_t n = 0;
        for (const Entry& entry : entries)
            n += entry.pieceId == pieceId && entry.sign == sign;
        return n;
    }
};

RecordingTrace traceOf(const char* fen) {
    GameContext context;
    io::fen_parser::deserialize(fen, context.editChessboard());
    RecordingTrace trace;
    (void)pieceScore(context.readChessPosition().material(), trace);
    return trace;
}

TaperedScore scoreOf(const char* fen) {
    GameContext context;
    io::fen_parser::deserialize(fen, context.editChessboard());
    return pieceScore(context.readChessPosition().material());
}

} // namespace

TEST(Pieces, StartingPosition_EveryPairBothSides_ScoresZero) {
    const char* fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    const RecordingTrace trace = traceOf(fen);
    for (u32 pieceId : { knightId, bishopId, rookId }) {
        EXPECT_EQ(1u, trace.count(pieceId, 1));
        EXPECT_EQ(1u, trace.count(pieceId, -1));
    }
    EXPECT_EQ(S(0, 0), scoreOf(fen));
}

TEST(Pieces, BishopPair_OnlyWithBothColours) {
    // white's bishops on c1 (dark) & f1 (light), black's on c8 (light) & e7 (dark): both pairs.
    EXPECT_EQ(1u, traceOf("2b1k3/4b3/8/8/8/8/8/2B1KB2 w - - 0 1").count(bishopId, 1));
    EXPECT_EQ(1u, traceOf("2b1k3/4b3/8/8/8/8/8/2B1KB2 w - - 0 1").count(bishopId, -1));

    // white's two bishops both on dark squares, c1 & e3, after a promotion: no pair. Black's lone bishop: none.
    const RecordingTrace trace = traceOf("2b1k3/8/8/8/8/4B3/8/2B1K3 w - - 0 1");
    EXPECT_EQ(0u, trace.count(bishopId, 1));
    EXPECT_EQ(0u, trace.count(bishopId, -1));
}

TEST(Pieces, OnlyOneSideWithPairs_ScoresTheirValues) {
    // white has two knights, the bishop pair and two rooks, black a knight, a bishop & a rook.
    const TaperedScore expected =
        evaluator_data::piecePair[0] + evaluator_data::piecePair[1] + evaluator_data::piecePair[2];
    EXPECT_EQ(expected, scoreOf("rnb1k3/8/8/8/8/8/8/RNB1KBNR w - - 0 1"));
    EXPECT_EQ(-expected, scoreOf("rnb1kbnr/8/8/8/8/8/8/RNB1K3 w - - 0 1"));

    const RecordingTrace trace = traceOf("rnb1k3/8/8/8/8/8/8/RNB1KBNR w - - 0 1");
    EXPECT_EQ(3u, trace.entries.size());
    EXPECT_EQ(0u, trace.count(knightId, -1) + trace.count(bishopId, -1) + trace.count(rookId, -1));
}

} // namespace ElephantTest
