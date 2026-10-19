#include <gtest/gtest.h>

#include <map>
#include <tuple>
#include <vector>

#include <core/game_context.hpp>
#include <eval/pawn_structure.hpp>
#include <io/fen_parser.hpp>

namespace ElephantTest {

namespace {

enum class Term { Passed, Isolated, Doubled, Backward, Connected };

// records what pawnScore reports for every pawn term, see NoPawnTrace.
struct RecordingTrace {
    struct Entry {
        Term term;
        u32 at;  // relative rank for passed & connected, file for the others
        i32 sign;
    };
    std::vector<Entry> entries;

    void passed(u32 relRank, i32 sign) { entries.push_back({ Term::Passed, relRank, sign }); }
    void isolated(u32 file, i32 sign) { entries.push_back({ Term::Isolated, file, sign }); }
    void doubled(u32 file, i32 sign) { entries.push_back({ Term::Doubled, file, sign }); }
    void backward(u32 file, i32 sign) { entries.push_back({ Term::Backward, file, sign }); }
    void connected(u32 relRank, i32 sign) { entries.push_back({ Term::Connected, relRank, sign }); }

    // how many times term was reported at at with sign.
    [[nodiscard]] std::size_t count(Term term, u32 at, i32 sign) const {
        std::size_t n = 0;
        for (const Entry& entry : entries)
            n += entry.term == term && entry.at == at && entry.sign == sign;
        return n;
    }

    // how many times term was reported at all, for white (sign 1) or black (sign -1).
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
    (void)pawnScore(context.readChessPosition().material(), trace);
    return trace;
}

} // namespace

TEST(PawnStructure, LonePawnIsPassedAndIsolated) {
    // e5 is rank 5, relative rank 4 counting rank 2 as 1.
    const RecordingTrace trace = traceOf("4k3/8/8/4P3/8/8/8/4K3 w - - 0 1");
    ASSERT_EQ(2u, trace.entries.size());
    EXPECT_EQ(1u, trace.count(Term::Passed, 4, 1));
    EXPECT_EQ(1u, trace.count(Term::Isolated, 4, 1));
}

TEST(PawnStructure, EnemyPawnOnAnAdjacentFileAheadStopsPassed) {
    // black's d6 can take on e5's path, e5 isn't passed. d6 isn't either, e5 stands in front of it on an adjacent file.
    const RecordingTrace trace = traceOf("4k3/8/3p4/4P3/8/8/8/4K3 w - - 0 1");
    EXPECT_EQ(0u, trace.count(Term::Passed, 1));
    EXPECT_EQ(0u, trace.count(Term::Passed, -1));

    // an enemy pawn behind it doesn't: d4 has passed e5 already.
    const RecordingTrace behind = traceOf("4k3/8/8/4P3/3p4/8/8/4K3 w - - 0 1");
    EXPECT_EQ(1u, behind.count(Term::Passed, 4, 1));
}

TEST(PawnStructure, DoubledCountsEachPawnWithOneInFront) {
    // e2 has e3 in front, it's doubled and not passed; e3 is passed. Both isolated.
    const RecordingTrace pair = traceOf("4k3/8/8/8/8/4P3/4P3/4K3 w - - 0 1");
    EXPECT_EQ(1u, pair.count(Term::Doubled, 4, 1));
    EXPECT_EQ(1u, pair.count(Term::Passed, 1));
    EXPECT_EQ(1u, pair.count(Term::Passed, 2, 1));
    EXPECT_EQ(2u, pair.count(Term::Isolated, 4, 1));

    // three on a file count two.
    const RecordingTrace triple = traceOf("4k3/8/8/8/4P3/4P3/4P3/4K3 w - - 0 1");
    EXPECT_EQ(2u, triple.count(Term::Doubled, 4, 1));
}

TEST(PawnStructure, NeighbourOnAnyRankOfAnAdjacentFileIsNotIsolated) {
    // a2 has nothing on the b file; c2 & d5 are on adjacent files, however far apart.
    const RecordingTrace trace = traceOf("4k3/8/8/3P4/8/8/P1P5/4K3 w - - 0 1");
    EXPECT_EQ(1u, trace.count(Term::Isolated, 1));
    EXPECT_EQ(1u, trace.count(Term::Isolated, 0, 1));
}

TEST(PawnStructure, BackwardPawn) {
    // d3's neighbour c4 is ahead of it, nothing supports d4, and black's e5 attacks d4. c4 is supported by d3.
    const RecordingTrace trace = traceOf("4k3/8/8/4p3/2P5/3P4/8/4K3 w - - 0 1");
    EXPECT_EQ(1u, trace.count(Term::Backward, 1));
    EXPECT_EQ(1u, trace.count(Term::Backward, 3, 1));

    // without e5 nothing attacks d4, d3 can catch up.
    const RecordingTrace free = traceOf("4k3/8/8/8/2P5/3P4/8/4K3 w - - 0 1");
    EXPECT_EQ(0u, free.count(Term::Backward, 1));

    // an isolated pawn isn't backward as well, even with its stop square attacked.
    const RecordingTrace isolated = traceOf("4k3/8/8/4p3/8/3P4/8/4K3 w - - 0 1");
    EXPECT_EQ(0u, isolated.count(Term::Backward, 1));
    EXPECT_EQ(1u, isolated.count(Term::Isolated, 3, 1));
}

TEST(PawnStructure, ConnectedBesideOrDefended) {
    // a phalanx: d4 & e4 side by side, both connected, relative rank 3.
    const RecordingTrace phalanx = traceOf("4k3/8/8/8/3PP3/8/8/4K3 w - - 0 1");
    EXPECT_EQ(2u, phalanx.count(Term::Connected, 3, 1));

    // a chain: d3 defends e4, e4 is connected, d3 isn't.
    const RecordingTrace chain = traceOf("4k3/8/8/8/4P3/3P4/8/4K3 w - - 0 1");
    EXPECT_EQ(1u, chain.count(Term::Connected, 1));
    EXPECT_EQ(1u, chain.count(Term::Connected, 3, 1));
}

TEST(PawnStructure, BlackCountsNegativeFromItsOwnSide) {
    // black's e4 is its own 5th rank, relative rank 4 counting rank 2 as 1, the same as white's e5.
    const RecordingTrace trace = traceOf("4k3/8/8/8/4p3/8/8/4K3 b - - 0 1");
    EXPECT_EQ(1u, trace.count(Term::Passed, 4, -1));
    EXPECT_EQ(1u, trace.count(Term::Isolated, 4, -1));
    EXPECT_EQ(0u, trace.count(Term::Passed, 1));
}

TEST(PawnStructure, MirroredPositionsCancel) {
    // white's pawns are black's mirrored, every term white reports black reports at the same rank or file.
    for (const char* fen : { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                             "4k3/pp3p1p/2p3p1/3p4/3P4/2P3P1/PP3P1P/4K3 w - - 0 1" }) {
        const RecordingTrace trace = traceOf(fen);
        std::map<std::tuple<Term, u32>, i32> sum;
        for (const auto& entry : trace.entries)
            sum[{ entry.term, entry.at }] += entry.sign;
        for (const auto& [key, total] : sum)
            EXPECT_EQ(0, total) << fen;

        GameContext context;
        io::fen_parser::deserialize(fen, context.editChessboard());
        EXPECT_EQ(S(0, 0), pawnScore(context.readChessPosition().material())) << fen;
    }
}

}  // namespace ElephantTest
