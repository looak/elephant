#include <gtest/gtest.h>

#include <vector>

#include <core/game_context.hpp>
#include <eval/pieces.hpp>
#include <io/fen_parser.hpp>

namespace ElephantTest {

namespace {

// records what pieceScore reports, see NoPieceTrace: pairs in entries, rooks & queens on files in files.
struct RecordingTrace {
    struct Entry {
        u32 pieceId;
        i32 sign;
    };
    struct FileEntry {
        u32 pieceId;
        u32 open;
        i32 sign;
    };
    std::vector<Entry> entries;
    std::vector<FileEntry> files;

    void piecePair(u32 pieceId, i32 sign) { entries.push_back({ pieceId, sign }); }
    void pieceOnFile(u32 pieceId, u32 open, i32 sign) { files.push_back({ pieceId, open, sign }); }

    [[nodiscard]] std::size_t count(u32 pieceId, i32 sign) const {
        std::size_t n = 0;
        for (const Entry& entry : entries)
            n += entry.pieceId == pieceId && entry.sign == sign;
        return n;
    }

    [[nodiscard]] std::size_t countOnFile(u32 pieceId, u32 open, i32 sign) const {
        std::size_t n = 0;
        for (const FileEntry& entry : files)
            n += entry.pieceId == pieceId && entry.open == open && entry.sign == sign;
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
    EXPECT_TRUE(trace.files.empty());
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
    // white has two knights, the bishop pair and two rooks, black a knight, a bishop & a rook. Every rook stands on a
    // file with its own pawn, no file terms.
    const TaperedScore expected =
        evaluator_data::piecePair[0] + evaluator_data::piecePair[1] + evaluator_data::piecePair[2];
    EXPECT_EQ(expected, scoreOf("rnb1k3/p7/8/8/8/8/P6P/RNB1KBNR w - - 0 1"));
    EXPECT_EQ(-expected, scoreOf("rnb1kbnr/p6p/8/8/8/8/P7/RNB1K3 w - - 0 1"));

    const RecordingTrace trace = traceOf("rnb1k3/p7/8/8/8/8/P6P/RNB1KBNR w - - 0 1");
    EXPECT_EQ(3u, trace.entries.size());
    EXPECT_EQ(0u, trace.count(knightId, -1) + trace.count(bishopId, -1) + trace.count(rookId, -1));
    EXPECT_TRUE(trace.files.empty());
}

TEST(Pieces, RooksAndQueens_OpenAndHalfOpenFiles) {
    // white's rook on the open d file, its queen on the e file where only black's pawn is (half-open); black's rook on
    // the a file behind its own pawn, nothing; black's queen on the h file where only white's pawn is (half-open).
    const RecordingTrace trace = traceOf("r3k2q/p3p3/8/8/8/8/7P/3RQ1K1 w - - 0 1");
    EXPECT_EQ(1u, trace.countOnFile(rookId, 1, 1));
    EXPECT_EQ(1u, trace.countOnFile(queenId, 0, 1));
    EXPECT_EQ(1u, trace.countOnFile(queenId, 0, -1));
    EXPECT_EQ(3u, trace.files.size());

    const TaperedScore expected =
        evaluator_data::rookOnFile[1] + evaluator_data::queenOnFile[0] - evaluator_data::queenOnFile[0];
    EXPECT_EQ(expected, scoreOf("r3k2q/p3p3/8/8/8/8/7P/3RQ1K1 w - - 0 1"));
}

TEST(Pieces, OccupiedFiles_EveryRankFoldedOntoTheFirst) {
    EXPECT_EQ(0u, pawn_internals::occupiedFiles(0));
    // a2, d4 & h7: files a, d & h.
    const u64 pawns = (UINT64_C(1) << 8) | (UINT64_C(1) << 27) | (UINT64_C(1) << 55);
    EXPECT_EQ(0b10001001u, pawn_internals::occupiedFiles(pawns));
}

} // namespace ElephantTest
