#include <gtest/gtest.h>

#include <core/game_context.hpp>
#include <eval/pawn_hash.hpp>
#include <io/fen_parser.hpp>

namespace ElephantTest {

namespace {

TaperedScore hashed(PawnHashTable& table, const char* fen) {
    GameContext context;
    io::fen_parser::deserialize(fen, context.editChessboard());
    return table.score(context.readChessPosition().material());
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

}  // namespace ElephantTest
