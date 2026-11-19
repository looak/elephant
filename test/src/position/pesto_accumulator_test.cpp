#include <gtest/gtest.h>
#include <core/chessboard.hpp>
#include <eval/pesto_accumulator.hpp>
#include <io/fen_parser.hpp>
#include <move/generation/move_gen_policy.hpp>
#include <move/move_executor.hpp>

#include <string>
#include <vector>

namespace ElephantTest {

namespace {
bool matchesScratch(const Chessboard& board, const PestoAccumulator& accumulator) {
    return accumulator == PestoAccumulator::computeFromScratch(board.readPosition().material());
}

// Walks every legal line to the given depth making & unmaking through a MoveExecutor that carries the accumulator,
// checking it against a full recompute after every make and against its earlier value after every unmake. Records
// the first line that diverges.
template<Set us>
void verifyPestoThroughTree(Chessboard& board, PestoAccumulator& accumulator, int depth, std::vector<std::string>& line,
                            std::string& firstFailure) {
    if (depth == 0 || !firstFailure.empty())
        return;

    move_gen_policy::Tusk::forEachMove<us>(board.readPosition(), [&](PackedMove move) {
        if (!firstFailure.empty())
            return;

        const PestoAccumulator before = accumulator;
        line.push_back(move.toString());
        MoveExecutor executor(board.editPosition(), &accumulator);
        MoveUndoUnit undo;
        u16 halfmoveClock = 0;
        executor.makeMove(move, undo, halfmoveClock);

        if (!matchesScratch(board, accumulator)) {
            for (const auto& m : line) firstFailure += m + " ";
            firstFailure += "(after make)";
        }

        verifyPestoThroughTree<opposing_set<us>()>(board, accumulator, depth - 1, line, firstFailure);

        executor.unmakeMove(undo);
        if (firstFailure.empty() && !(accumulator == before)) {
            for (const auto& m : line) firstFailure += m + " ";
            firstFailure += "(after unmake)";
        }
        line.pop_back();
    });
}
} // namespace

TEST(PestoAccumulator, IncrementalMatchesComputedThroughTree)
{
    // castling, en passant, promotions & under promotions with captures are all reached within these depths.
    const std::vector<std::pair<std::string, int>> positions = {
        { "2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - - 0 1", 4 },             // WAC.001
        { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3 },   // kiwipete
        { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 4 },
        { "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 3 },
        { "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 3 },
    };

    for (const auto& [fen, depth] : positions) {
        Chessboard board = *io::fen_parser::read(fen);
        PestoAccumulator accumulator = PestoAccumulator::computeFromScratch(board.readPosition().material());

        std::vector<std::string> line;
        std::string firstFailure;
        if (board.readToPlay() == Set::WHITE)
            verifyPestoThroughTree<Set::WHITE>(board, accumulator, depth, line, firstFailure);
        else
            verifyPestoThroughTree<Set::BLACK>(board, accumulator, depth, line, firstFailure);

        EXPECT_TRUE(firstFailure.empty()) << fen << "\n  accumulator diverged after: " << firstFailure;
    }
}

TEST(PestoAccumulator, AddRemoveIsSymmetric)
{
    PestoAccumulator accumulator;
    const ChessPiece whiteQueen(Set::WHITE, PieceType::QUEEN);
    const ChessPiece blackKnight(Set::BLACK, PieceType::KNIGHT);

    accumulator.add(whiteQueen, Square::D1);
    accumulator.add(blackKnight, Square::F6);
    EXPECT_EQ(accumulator.phase, 5);
    EXPECT_GT(accumulator.material, 0);

    accumulator.remove(whiteQueen, Square::D1);
    accumulator.remove(blackKnight, Square::F6);
    EXPECT_EQ(accumulator, PestoAccumulator{});
}

} // namespace ElephantTest
