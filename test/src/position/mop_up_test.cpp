#include <gtest/gtest.h>

#include <core/game_context.hpp>
#include <eval/mop_up.hpp>
#include <io/fen_parser.hpp>

namespace ElephantTest {

namespace {

i32 mopUpOf(const char* fen) {
    GameContext context;
    io::fen_parser::deserialize(fen, context.editChessboard());
    const auto& material = context.readChessPosition().material();
    return mopUpScore(material, PestoAccumulator::computeFromScratch(material));
}

} // namespace

TEST(MopUp, EqualMaterial_None) {
    EXPECT_EQ(0, mopUpOf("4k3/8/8/8/8/8/8/4K3 w - - 0 1"));
    EXPECT_EQ(0, mopUpOf("3qk3/8/8/8/8/8/8/3QK3 w - - 0 1"));
}

TEST(MopUp, QueenUp_WinningKingCloses) {
    // kings on e1 & e8, 7 apart: (14 - 7) * 8, and 30 for black's king in the middle of the edge.
    EXPECT_EQ(86, mopUpOf("4k3/8/8/8/8/8/8/3QK3 w - - 0 1"));
    // the same for black, white's side.
    EXPECT_EQ(-86, mopUpOf("3qk3/8/8/8/8/8/8/4K3 w - - 0 1"));
    // white's king on e4, 4 from e8: (14 - 4) * 8 + 30.
    EXPECT_EQ(110, mopUpOf("4k3/8/8/8/4K3/8/8/3Q4 w - - 0 1"));
}

TEST(MopUp, LosingKingDrivenToTheCorner) {
    // the same king distance, 4: black's king in the centre, on the edge, in the corner.
    EXPECT_EQ(80, mopUpOf("8/8/8/4k3/8/8/8/3QK3 w - - 0 1"));
    EXPECT_EQ(110, mopUpOf("4k3/8/8/8/4K3/8/8/3Q4 w - - 0 1"));
    EXPECT_EQ(140, mopUpOf("7k/8/5K2/8/8/8/8/3Q4 w - - 0 1"));
}

TEST(MopUp, MoreThanTwoPawnsAhead) {
    EXPECT_EQ(0, mopUpOf("4k3/8/8/8/8/8/PP6/4K3 w - - 0 1"));
    EXPECT_LT(0, mopUpOf("4k3/8/8/8/8/8/PPP5/4K3 w - - 0 1"));
}

TEST(MopUp, OnlyInEndgames) {
    // a queen up with everything else on the board: 31 pieces and most of the material, not an endgame.
    EXPECT_EQ(0, mopUpOf("rnb1kbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
    // 19 pieces, but only the pawns & a rook: under half the starting material, an endgame.
    EXPECT_LT(0, mopUpOf("4k3/pppppppp/8/8/8/8/PPPPPPPP/R3K3 w - - 0 1"));
}

} // namespace ElephantTest
