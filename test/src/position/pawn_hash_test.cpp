#include <gtest/gtest.h>

#include <core/game_context.hpp>
#include <eval/pawn_hash.hpp>
#include <io/fen_parser.hpp>

namespace ElephantTest {

namespace {

TaperedScore hashed(PawnHashTable& table, const char* fen) {
    GameContext context;
    io::fen_parser::deserialize(fen, context.editChessboard());
    return table.probe(context.readChessPosition().material()).score;
}

TaperedScore counted(const char* fen) {
    GameContext context;
    io::fen_parser::deserialize(fen, context.editChessboard());
    return pawnScore(context.readChessPosition().material());
}

} // namespace

TEST(PawnHash, MissThenHit_BothThePawnScore) {
    PawnHashTable table;
    const char* fen = "4k3/pp3p1p/2p3p1/3p4/3P4/2P1P1P1/PP3P1P/4K3 w - - 0 1";
    EXPECT_EQ(counted(fen), hashed(table, fen));
    EXPECT_EQ(0u, table.hits());
    EXPECT_EQ(1u, table.misses());

    EXPECT_EQ(counted(fen), hashed(table, fen));
    EXPECT_EQ(1u, table.hits());
}

TEST(PawnHash, PiecesDontMatter_PawnsDo) {
    PawnHashTable table;
    // the same pawns with other pieces is a hit, a pawn moved is not.
    (void)hashed(table, "r3k3/pp3p1p/2p3p1/3p4/3P4/2P1P1P1/PP3P1P/4K2R w - - 0 1");
    EXPECT_EQ(counted("4k3/pp3p1p/2p3p1/3p4/3P4/2P1P1P1/PP3P1P/2Q1K3 w - - 0 1"),
              hashed(table, "4k3/pp3p1p/2p3p1/3p4/3P4/2P1P1P1/PP3P1P/2Q1K3 w - - 0 1"));
    EXPECT_EQ(1u, table.hits());

    const char* moved = "4k3/pp3p1p/2p3p1/3p4/3P4/2P1P1P1/PP3P2/4K3 w - - 0 1";  // h2 gone
    EXPECT_EQ(counted(moved), hashed(table, moved));
    EXPECT_EQ(1u, table.hits());
    EXPECT_EQ(2u, table.misses());
}

TEST(PawnHash, ColoursSwapped_AreDifferentStructures) {
    PawnHashTable table;
    // a lone white pawn on e5 and a lone black pawn on e5 are the same squares, not the same structure.
    EXPECT_EQ(counted("4k3/8/8/4P3/8/8/8/4K3 w - - 0 1"), hashed(table, "4k3/8/8/4P3/8/8/8/4K3 w - - 0 1"));
    EXPECT_EQ(counted("4k3/8/8/4p3/8/8/8/4K3 w - - 0 1"), hashed(table, "4k3/8/8/4p3/8/8/8/4K3 w - - 0 1"));
    EXPECT_EQ(0u, table.hits());
}

TEST(PawnHash, NoPawns_ScoresNothing) {
    PawnHashTable table;
    EXPECT_EQ(S(0, 0), hashed(table, "4k3/8/8/8/8/8/8/R3K3 w - - 0 1"));
}

// without pawns nothing can drive a piece off, every 4th to 6th rank square is an outpost; the entry a position without
// pawns finds before anything is written there says so too, as counted.
TEST(PawnHash, NoPawns_UnwrittenEntryHasTheCountedOutposts) {
    PawnHashTable table;
    GameContext context;
    io::fen_parser::deserialize("4k3/8/8/8/8/8/8/R3K3 w - - 0 1", context.editChessboard());
    const auto& material = context.readChessPosition().material();
    const PawnInfo& info = table.probe(material);
    EXPECT_EQ(1u, table.hits());
    EXPECT_EQ(pawnInfo(material).outpostsOf(Set::WHITE), info.outpostsOf(Set::WHITE));
    EXPECT_EQ(pawnInfo(material).outpostsOf(Set::BLACK), info.outpostsOf(Set::BLACK));
    EXPECT_EQ(UINT64_C(0x0000FFFFFF000000), info.outpostsOf(Set::WHITE));
    EXPECT_EQ(UINT64_C(0x000000FFFFFF0000), info.outpostsOf(Set::BLACK));
}

}  // namespace ElephantTest
