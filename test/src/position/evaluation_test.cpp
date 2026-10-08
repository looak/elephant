#include <gtest/gtest.h>
#include <core/game_context.hpp>
#include <eval/evaluator.hpp>
#include <io/fen_parser.hpp>

struct EvaluationTestCase {
    std::string fen;
};

// Mirrors the ranks and swaps the colors, the same position seen from the other side. A 180 degree rotation isn't the
// same thing, the piece/sq tables aren't symmetric between the a and h files.
Position flip(PositionReader origin) {
    Position flipped;
    auto flippedEditor = flipped.edit();

    for (u8 sqr = 0; sqr < 64; ++sqr) {
        ChessPiece piece = origin.pieceAt(static_cast<Square>(sqr));
        if (piece.isValid()) {
            Set flippedSet = (piece.getSet() == Set::WHITE) ? Set::BLACK : Set::WHITE;
            flippedEditor.placePiece<false>(ChessPiece(flippedSet, piece.getType()), static_cast<Square>(sqr ^ 56));
        }
    }
    return flipped;
}

namespace {
i32 evaluate(const std::string& fen) {
    Position position;
    io::fen_parser::deserialize(fen.c_str(), position.edit());
    return Evaluator(position.read()).Evaluate();
}

i32 gamePhase(const std::string& fen) {
    Position position;
    io::fen_parser::deserialize(fen.c_str(), position.edit());
    return Evaluator(position.read()).gamePhase();
}

void expectSymmetric(const std::string& fen) {
    Position position;
    io::fen_parser::deserialize(fen.c_str(), position.edit());
    Position flippedPosition = flip(position.read());

    i32 eval = Evaluator(position.read()).Evaluate();
    i32 evalFlipped = Evaluator(flippedPosition.read()).Evaluate();
    EXPECT_EQ(eval, -evalFlipped) << fen;
}
} // namespace

TEST(EvaluationSymmetry, StartBoard)
{
    expectSymmetric("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

TEST(EvaluationSymmetry, HighlevelPositionFromLichess)
{
    expectSymmetric("r1bq1r2/p3ppkp/1pn3p1/2pn4/1P1P4/1P3NP1/P3PPBP/RNQ2RK1 b - - 0 11");
}

TEST(EvaluationSymmetry, AsymmetricMiddlegameAndEndgame)
{
    expectSymmetric("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    expectSymmetric("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    expectSymmetric("6k1/5ppp/8/8/8/8/1Q6/6K1 w - - 0 1");
}

// Catches the tables being read upside down, the eval would still be symmetric but reward pawns for staying home.
TEST(EvaluationPesto, PawnAdvancementIsRewarded)
{
    // only kings & pawns, phase 0 so the endgame table alone counts: a7 is 178, a2 is 13.
    const i32 whitePawnA7 = evaluate("4k3/P7/8/8/8/8/8/4K3 w - - 0 1");
    const i32 whitePawnA2 = evaluate("4k3/8/8/8/8/8/P7/4K3 w - - 0 1");
    EXPECT_EQ(whitePawnA7 - whitePawnA2, 178 - 13);

    // the same pawns for black, a2 is black's 7th rank.
    const i32 blackPawnA2 = evaluate("4k3/8/8/8/8/8/p7/4K3 w - - 0 1");
    const i32 blackPawnA7 = evaluate("4k3/p7/8/8/8/8/8/4K3 w - - 0 1");
    EXPECT_EQ(blackPawnA2 - blackPawnA7, -(178 - 13));
}

// At full phase only the midgame tables count, knight g1 is -19 and f3 is 17.
TEST(EvaluationPesto, MidgameTablesAtFullPhase)
{
    const i32 start = evaluate("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    const i32 knightF3 = evaluate("rnbqkbnr/pppppppp/8/8/8/5N2/PPPPPPPP/RNBQKB1R b KQkq - 1 1");
    EXPECT_EQ(start, 0);
    EXPECT_EQ(knightF3 - start, 17 - (-19));
}

TEST(EvaluationPesto, GamePhase)
{
    EXPECT_EQ(gamePhase("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"), 24);
    EXPECT_EQ(gamePhase("4k3/pppppppp/8/8/8/8/PPPPPPPP/4K3 w - - 0 1"), 0);
    // knight 1, bishop 1, rook 2, queen 4.
    EXPECT_EQ(gamePhase("4k3/8/8/8/8/8/8/NBRQK3 w - - 0 1"), 8);
    // promotions don't push the phase past 24.
    EXPECT_EQ(gamePhase("QQQQQQQk/8/8/8/8/8/8/K7 w - - 0 1"), 24);
}
