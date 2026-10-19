#include <gtest/gtest.h>

#include <map>
#include <tuple>
#include <vector>

#include <core/game_context.hpp>
#include <eval/passed_pawns.hpp>
#include <io/fen_parser.hpp>

namespace ElephantTest {

namespace {

enum class Extra { OwnKing, EnemyKing, Blocked, FreePath, RookBehind };

// records what passerScore reports for every passed pawn, see NoPasserTrace.
struct RecordingTrace {
    struct Entry {
        Extra extra;
        u32 at;  // distance, relative rank or which rook
        i32 sign;
    };
    std::vector<Entry> entries;

    void passedOwnKing(u32 d, i32 sign) { entries.push_back({ Extra::OwnKing, d, sign }); }
    void passedEnemyKing(u32 d, i32 sign) { entries.push_back({ Extra::EnemyKing, d, sign }); }
    void passedBlocked(u32 relRank, i32 sign) { entries.push_back({ Extra::Blocked, relRank, sign }); }
    void passedFreePath(u32 relRank, i32 sign) { entries.push_back({ Extra::FreePath, relRank, sign }); }
    void passedRookBehind(u32 whose, i32 sign) { entries.push_back({ Extra::RookBehind, whose, sign }); }

    [[nodiscard]] std::size_t count(Extra extra, u32 at, i32 sign) const {
        std::size_t n = 0;
        for (const Entry& entry : entries)
            n += entry.extra == extra && entry.at == at && entry.sign == sign;
        return n;
    }

    [[nodiscard]] std::size_t count(Extra extra) const {
        std::size_t n = 0;
        for (const Entry& entry : entries)
            n += entry.extra == extra;
        return n;
    }
};

RecordingTrace traceOf(const char* fen) {
    GameContext context;
    io::fen_parser::deserialize(fen, context.editChessboard());
    const auto& material = context.readChessPosition().material();
    RecordingTrace trace;
    (void)passerScore(material, pawnInfo(material), trace);
    return trace;
}

} // namespace

TEST(PassedPawns, KingDistances) {
    // e5 passed: white's king on e3 is 2 king moves away, black's on e8 is 3. e8 holds the king, the path isn't free,
    // e6 is empty, it isn't blocked either.
    const RecordingTrace trace = traceOf("4k3/8/8/4P3/8/4K3/8/8 w - - 0 1");
    EXPECT_EQ(1u, trace.count(Extra::OwnKing, 2, 1));
    EXPECT_EQ(1u, trace.count(Extra::EnemyKing, 3, 1));
    EXPECT_EQ(0u, trace.count(Extra::Blocked));
    EXPECT_EQ(0u, trace.count(Extra::FreePath));
}

TEST(PassedPawns, BlockedOrFreePath) {
    // black's king on e6 stands in front of e5.
    const RecordingTrace blocked = traceOf("8/8/4k3/4P3/8/4K3/8/8 w - - 0 1");
    EXPECT_EQ(1u, blocked.count(Extra::Blocked, 4, 1));
    EXPECT_EQ(0u, blocked.count(Extra::FreePath));

    // the kings off the file, e6 to e8 are empty.
    const RecordingTrace free = traceOf("k7/8/8/4P3/8/8/8/K7 w - - 0 1");
    EXPECT_EQ(1u, free.count(Extra::FreePath, 4, 1));
    EXPECT_EQ(0u, free.count(Extra::Blocked));
    EXPECT_EQ(1u, free.count(Extra::OwnKing, 4, 1));
}

TEST(PassedPawns, RookBehind) {
    const RecordingTrace own = traceOf("k7/8/8/4P3/8/8/8/K3R3 w - - 0 1");
    EXPECT_EQ(1u, own.count(Extra::RookBehind, 0, 1));

    const RecordingTrace enemy = traceOf("k7/8/8/4P3/8/8/K7/4r3 w - - 0 1");
    EXPECT_EQ(1u, enemy.count(Extra::RookBehind, 1, 1));

    // a knight on e3 stands between, the rook isn't behind it any more. A rook in front doesn't count.
    const RecordingTrace between = traceOf("k7/8/8/4P3/8/4N3/8/K3R3 w - - 0 1");
    EXPECT_EQ(0u, between.count(Extra::RookBehind));
    const RecordingTrace ahead = traceOf("k3R3/8/8/4P3/8/8/8/K7 w - - 0 1");
    EXPECT_EQ(0u, ahead.count(Extra::RookBehind));
}

TEST(PassedPawns, BlackFromItsOwnSide) {
    // d4 is black's 5th rank, relative rank 4; its king on d5 is behind it, d3 to d1 are empty.
    const RecordingTrace trace = traceOf("8/8/8/3k4/3p4/8/8/7K b - - 0 1");
    EXPECT_EQ(1u, trace.count(Extra::FreePath, 4, -1));
    EXPECT_EQ(1u, trace.count(Extra::OwnKing, 1, -1));
    EXPECT_EQ(1u, trace.count(Extra::EnemyKing, 4, -1));
}

TEST(PassedPawns, OnlyPassedPawnsCount) {
    // e4 & e5 block each other, neither is passed.
    const RecordingTrace trace = traceOf("4k3/8/8/4p3/4P3/8/8/4K3 w - - 0 1");
    EXPECT_TRUE(trace.entries.empty());
}

TEST(PassedPawns, MirroredPositionsCancel) {
    for (const char* fen : { "6k1/1r4p1/8/P7/p7/8/1R4P1/6K1 w - - 0 1",
                             "4k3/8/3N4/3P4/3p4/3n4/8/4K3 w - - 0 1" }) {
        const RecordingTrace trace = traceOf(fen);
        EXPECT_FALSE(trace.entries.empty()) << fen;
        std::map<std::tuple<Extra, u32>, i32> sum;
        for (const auto& entry : trace.entries)
            sum[{ entry.extra, entry.at }] += entry.sign;
        for (const auto& [key, total] : sum)
            EXPECT_EQ(0, total) << fen;
    }
}

}  // namespace ElephantTest
