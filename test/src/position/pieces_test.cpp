#include <gtest/gtest.h>

#include <vector>

#include <core/game_context.hpp>
#include <eval/pieces.hpp>
#include <io/fen_parser.hpp>

namespace ElephantTest {

namespace {

// records what pieceScore reports, see NoPieceTrace: pairs in entries, outposts in outposts.
struct RecordingTrace {
    struct Entry {
        u32 pieceId;
        i32 sign;
    };
    struct OutpostEntry {
        u32 pieceId;
        u32 defended;
        i32 sign;
    };
    std::vector<Entry> entries;
    std::vector<OutpostEntry> outposts;

    void piecePair(u32 pieceId, i32 sign) { entries.push_back({ pieceId, sign }); }
    void outpost(u32 pieceId, u32 defended, i32 sign) { outposts.push_back({ pieceId, defended, sign }); }

    [[nodiscard]] std::size_t count(u32 pieceId, i32 sign) const {
        std::size_t n = 0;
        for (const Entry& entry : entries)
            n += entry.pieceId == pieceId && entry.sign == sign;
        return n;
    }

    [[nodiscard]] std::size_t countOutposts(u32 pieceId, u32 defended, i32 sign) const {
        std::size_t n = 0;
        for (const OutpostEntry& entry : outposts)
            n += entry.pieceId == pieceId && entry.defended == defended && entry.sign == sign;
        return n;
    }
};

RecordingTrace traceOf(const char* fen) {
    GameContext context;
    io::fen_parser::deserialize(fen, context.editChessboard());
    const auto& material = context.readChessPosition().material();
    RecordingTrace trace;
    (void)pieceScore(material, pawnInfo(material), trace);
    return trace;
}

TaperedScore scoreOf(const char* fen) {
    GameContext context;
    io::fen_parser::deserialize(fen, context.editChessboard());
    const auto& material = context.readChessPosition().material();
    return pieceScore(material, pawnInfo(material));
}

} // namespace

TEST(Pieces, StartingPosition_EveryPairBothSides_ScoresZero) {
    const char* fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    const RecordingTrace trace = traceOf(fen);
    for (u32 pieceId : { knightId, bishopId, rookId }) {
        EXPECT_EQ(1u, trace.count(pieceId, 1));
        EXPECT_EQ(1u, trace.count(pieceId, -1));
    }
    EXPECT_TRUE(trace.outposts.empty());
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

TEST(Pieces, Outposts_DefendedUndefendedAndAttackable) {
    // black's pawns on a7 & h7 can only ever attack the b & g files. White: knight e5 defended by d4, a defended
    // outpost; bishop c6 undefended, an undefended outpost; knight g5, h7 can attack it from h6, none. Black: knight f4,
    // white's pawns on a2 & d4 can't reach it, an undefended outpost; bishop b4, a2 can attack it from a3, none.
    const char* fen = "6k1/p6p/2B5/4N1N1/1b1P1n2/8/P7/6K1 w - - 0 1";
    const RecordingTrace trace = traceOf(fen);
    EXPECT_EQ(1u, trace.countOutposts(knightId, 1, 1));
    EXPECT_EQ(1u, trace.countOutposts(bishopId, 0, 1));
    EXPECT_EQ(1u, trace.countOutposts(knightId, 0, -1));
    EXPECT_EQ(3u, trace.outposts.size());

    // white's two knights are a pair too.
    const TaperedScore expected = evaluator_data::piecePair[0] + evaluator_data::knightOutpost[1]
                                + evaluator_data::bishopOutpost[0] - evaluator_data::knightOutpost[0];
    EXPECT_EQ(expected, scoreOf(fen));
}

TEST(Pieces, Outposts_OnlyOnTheFourthToSixthRank) {
    // white knights on e7 and e3, no black pawn to attack either, its own pawn on d2 defending e3: neither is an
    // outpost.
    EXPECT_TRUE(traceOf("k7/4N3/8/8/8/4N3/3P4/K7 w - - 0 1").outposts.empty());
    // on e6 it is, without pawns every 4th to 6th rank square is one.
    EXPECT_EQ(1u, traceOf("k7/8/4N3/8/8/8/8/K7 w - - 0 1").countOutposts(knightId, 0, 1));
}

TEST(Pieces, Outposts_MirroredForBlack) {
    // a defended knight seen from black: knight e4 defended by d5, white's pawns on a2 & h2 can't reach it.
    const RecordingTrace trace = traceOf("4k3/8/8/3p4/4n3/8/P6P/4K3 w - - 0 1");
    EXPECT_EQ(1u, trace.countOutposts(knightId, 1, -1));
    EXPECT_EQ(1u, trace.outposts.size());
    EXPECT_EQ(-evaluator_data::knightOutpost[1], scoreOf("4k3/8/8/3p4/4n3/8/P6P/4K3 w - - 0 1"));
}

} // namespace ElephantTest
