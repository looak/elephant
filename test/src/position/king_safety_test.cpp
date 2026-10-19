#include <gtest/gtest.h>

#include <vector>

#include <core/game_context.hpp>
#include <eval/king_safety.hpp>
#include <io/fen_parser.hpp>

namespace ElephantTest {

namespace {

enum class Term { Shelter, Storm, StormBlocked };

// records what kingSafetyScore reports for every file, see NoKingTrace.
struct RecordingTrace {
    struct Entry {
        Term term;
        u32 folded;  // 0 for a blocked storm, it's by rank alone
        u32 rank;
        i32 sign;
    };
    std::vector<Entry> entries;

    void kingShelter(u32 folded, u32 rank, i32 sign) { entries.push_back({ Term::Shelter, folded, rank, sign }); }
    void kingStorm(u32 folded, u32 rank, i32 sign) { entries.push_back({ Term::Storm, folded, rank, sign }); }
    void kingStormBlocked(u32 rank, i32 sign) { entries.push_back({ Term::StormBlocked, 0, rank, sign }); }

    [[nodiscard]] std::size_t count(Term term, u32 folded, u32 rank, i32 sign) const {
        std::size_t n = 0;
        for (const Entry& entry : entries)
            n += entry.term == term && entry.folded == folded && entry.rank == rank && entry.sign == sign;
        return n;
    }

    [[nodiscard]] std::size_t count(Term term, i32 sign) const {
        std::size_t n = 0;
        for (const Entry& entry : entries)
            n += entry.term == term && entry.sign == sign;
        return n;
    }
};

RecordingTrace traceOf(const char* fen) {
    GameContext context;
    io::fen_parser::deserialize(fen, context.editChessboard());
    RecordingTrace trace;
    (void)kingSafetyScore(context.readChessPosition().material(), trace);
    return trace;
}

} // namespace

TEST(KingSafety, CastledKings_ShelterAndDistantStorm) {
    // both castled short, white's h pawn on h3. White's king looks at f, g & h (folded 2, 1, 0), black's f7, g7 & h7
    // are on relative rank 7 seen from white, index 6.
    const RecordingTrace trace = traceOf("6k1/5ppp/8/8/8/7P/5PP1/6K1 w - - 0 1");
    EXPECT_EQ(1u, trace.count(Term::Shelter, 2, 1, 1));
    EXPECT_EQ(1u, trace.count(Term::Shelter, 1, 1, 1));
    EXPECT_EQ(1u, trace.count(Term::Shelter, 0, 2, 1));
    EXPECT_EQ(1u, trace.count(Term::Storm, 2, 6, 1));
    EXPECT_EQ(1u, trace.count(Term::Storm, 1, 6, 1));
    EXPECT_EQ(1u, trace.count(Term::Storm, 0, 6, 1));

    // black's side, mirrored: its own pawns on its second rank, white's h3 on relative rank 6 seen from black.
    EXPECT_EQ(1u, trace.count(Term::Shelter, 0, 1, -1));
    EXPECT_EQ(1u, trace.count(Term::Storm, 0, 5, -1));
    EXPECT_EQ(1u, trace.count(Term::Storm, 1, 6, -1));
    EXPECT_EQ(0u, trace.count(Term::StormBlocked, 1) + trace.count(Term::StormBlocked, -1));
}

TEST(KingSafety, MirroredPositionsCancel) {
    // the same structure for both sides, every white term has a black one with the opposite sign.
    const RecordingTrace trace = traceOf("6k1/5pp1/7p/8/8/7P/5PP1/6K1 w - - 0 1");
    for (const auto& entry : trace.entries) {
        if (entry.sign == 1)
            EXPECT_EQ(1u, trace.count(entry.term, entry.folded, entry.rank, -1));
    }
}

TEST(KingSafety, EdgeKing_LooksAtThreeFiles) {
    // white's king on a1 looks at a, b & c, folded 0, 1 & 2. No pawns on c: an open file and no storm.
    const RecordingTrace trace = traceOf("7k/8/8/8/8/8/PP6/K7 w - - 0 1");
    EXPECT_EQ(1u, trace.count(Term::Shelter, 0, 1, 1));
    EXPECT_EQ(1u, trace.count(Term::Shelter, 1, 1, 1));
    EXPECT_EQ(1u, trace.count(Term::Shelter, 2, 0, 1));
    EXPECT_EQ(1u, trace.count(Term::Storm, 2, 0, 1));
    EXPECT_EQ(3u, trace.count(Term::Shelter, 1));
}

TEST(KingSafety, BlockedStorm) {
    // black's g4 stands right in front of white's g3: blocked, by its rank, 4th seen from white, index 3. f & h have
    // no black pawns.
    const RecordingTrace trace = traceOf("6k1/8/8/8/6p1/6P1/5P1P/6K1 w - - 0 1");
    EXPECT_EQ(1u, trace.count(Term::Shelter, 1, 2, 1));
    EXPECT_EQ(1u, trace.count(Term::StormBlocked, 0, 3, 1));
    EXPECT_EQ(2u, trace.count(Term::Storm, 1));
}

TEST(KingSafety, UnblockedStorm) {
    // black's g4 two squares in front of white's g2, free to advance to g3.
    const RecordingTrace trace = traceOf("6k1/8/8/8/6p1/8/5PPP/6K1 w - - 0 1");
    EXPECT_EQ(1u, trace.count(Term::Shelter, 1, 1, 1));
    EXPECT_EQ(1u, trace.count(Term::Storm, 1, 3, 1));
    EXPECT_EQ(0u, trace.count(Term::StormBlocked, 1));
}

TEST(KingSafety, PawnsBehindTheKingDontCount) {
    // white's king on g3 has its pawns on f2, g2 & h2 behind it: three open files in front of it.
    const RecordingTrace trace = traceOf("6k1/8/8/8/8/6K1/5PPP/8 w - - 0 1");
    EXPECT_EQ(1u, trace.count(Term::Shelter, 2, 0, 1));
    EXPECT_EQ(1u, trace.count(Term::Shelter, 1, 0, 1));
    EXPECT_EQ(1u, trace.count(Term::Shelter, 0, 0, 1));

    // a pawn beside the king, on its rank, still covers it.
    const RecordingTrace beside = traceOf("6k1/8/8/8/8/5PKP/6P1/8 w - - 0 1");
    EXPECT_EQ(1u, beside.count(Term::Shelter, 2, 2, 1));
    EXPECT_EQ(1u, beside.count(Term::Shelter, 1, 0, 1));
    EXPECT_EQ(1u, beside.count(Term::Shelter, 0, 2, 1));
}

} // namespace ElephantTest
