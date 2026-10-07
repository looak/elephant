#include <gtest/gtest.h>
#include <core/game_context.hpp>
#include <eval/evaluator.hpp>
#include <io/fen_parser.hpp>

struct EvaluationTestCase {
    std::string fen;
};

Position flip(PositionReader origin) {
    Position flipped;
    auto flippedEditor = flipped.edit();
    Square orgPos = Square::A1;
    Square flipItr = Square::H8;

    while (orgPos != Square::NullSQ) {
        ChessPiece piece = origin.pieceAt(orgPos);
        if (piece.isValid()) {
            Set flippedSet = (piece.getSet() == Set::WHITE) ? Set::BLACK : Set::WHITE;
            flippedEditor.placePiece<false>(ChessPiece(flippedSet, piece.getType()), flipItr);
        }
        orgPos = static_cast<Square>(static_cast<uint8_t>(orgPos) + 1);
        flipItr = static_cast<Square>(static_cast<uint8_t>(flipItr) - 1);
    }
    return flipped;
}


TEST(EvaluationSymmetry, StartBoard)
{
    std::string startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    Position originalPosition;
    io::fen_parser::deserialize(startFen.c_str(), originalPosition.edit());

    Position flippedPosition = flip(originalPosition.read());    
    Evaluator evaluatorBlack(flippedPosition.read());    
    Evaluator evaluatorWhite(originalPosition.read());
    i32 evalWhite = evaluatorWhite.Evaluate();
    i32 evalBlack = evaluatorBlack.Evaluate();

    EXPECT_EQ(evalWhite, -evalBlack) << "Evaluation symmetry failed for start position.";
}

TEST(EvaluationSymmetry, HighlevelPositionFromLichess)
{
    std::string fen = "r1bq1r2/p3ppkp/1pn3p1/2pn4/1P1P4/1P3NP1/P3PPBP/RNQ2RK1 b - - 0 11";

    Position position;
    io::fen_parser::deserialize(fen.c_str(), position.edit());

    Evaluator evaluator(position.read());
    i32 eval = evaluator.Evaluate();

    Position flippedPosition = flip(position.read());    
    Evaluator evaluatorFlipped(flippedPosition.read());

    i32 evalFlipped = evaluatorFlipped.Evaluate();

    // Just check that evaluation runs and returns a value within a reasonable range
    EXPECT_EQ(eval, -evalFlipped);
}