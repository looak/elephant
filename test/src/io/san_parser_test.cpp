#include <gtest/gtest.h>

#include <move/move.hpp>
#include <position/position.hpp>
#include <io/fen_parser.hpp>
#include <io/san_parser.hpp>
#include <debug/elephant_exceptions.hpp>



namespace ElephantTest {
////////////////////////////////////////////////////////////////

/**
 * @file san_parser_test.cpp
 * @brief Testing all things SAN parsing
 * Naming convention as of October 2023: <TestedFunctionality>_<ExpectedResult>
 * @author Alexander Loodin Ek  */
class SanParserFixture : public ::testing::Test {
public:
    Position testingPosition;
};

////////////////////////////////////////////////////////////////

TEST_F(SanParserFixture, ParseValidSan) {
    // setup
    PositionEditor editor(testingPosition);
    editor.placePiece(piece_constants::white_pawn, Square::E2);
    std::string san = "e4";
    bool whiteToMove = true;

    // do
    PackedMove move = io::san_parser::deserialize(testingPosition, whiteToMove, san);

    // verify
    EXPECT_FALSE(move.isNull());
    EXPECT_TRUE(move.isQuiet());
    EXPECT_EQ(move.targetSqr(), Square::E4);
    EXPECT_EQ(move.sourceSqr(), Square::E2);
}

TEST_F(SanParserFixture, ParseInvalidSan) {
    // setup
    PositionEditor editor(testingPosition);
    editor.placePiece(piece_constants::white_pawn, Square::E2);
    std::string san = "e9";  // invalid square
    bool whiteToMove = true;

    // do & verify
    EXPECT_THROW({
        io::san_parser::deserialize(testingPosition, whiteToMove, san);
    }, ephant::io_error);
}

TEST_F(SanParserFixture, BulkTest_GenericParsingTest) {
    // setup
    std::string gameSixFen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R";
    io::fen_parser::deserialize(gameSixFen, testingPosition.edit());
    struct TestCase {
        std::string san;
        Square expectedSource;
        Square expectedTarget;
    };

    using enum Square;

    std::vector<TestCase> testCases = {
        { "d6", D5, D6 },
        { "Nd3", E5, D3 },
        { "Ba6", E2, A6 },
        { "Kf1", E1, F1 }
    };

    bool whiteToMove = true;
    for (const auto& testCase : testCases) {
        // do
        PackedMove move = io::san_parser::deserialize(testingPosition, whiteToMove, testCase.san);

        // verify
        EXPECT_FALSE(move.isNull());        
        EXPECT_EQ(move.targetSqr(), testCase.expectedTarget);
        EXPECT_EQ(move.sourceSqr(), testCase.expectedSource);
    }
}

TEST_F(SanParserFixture, AmbiguousMoveParsing_UnableToResolveAmbiguity_ShouldThrow) {
    // setup
    std::string fen = "3k4/8/8/8/8/2N3N1/8/3K4";
    io::fen_parser::deserialize(fen, testingPosition.edit());
    std::string san = "Ne4";  // Both knights can move to e4, this should be ambiguous
    bool whiteToMove = true;

    // do
    EXPECT_THROW({
        io::san_parser::deserialize(testingPosition, whiteToMove, san);
    }, ephant::io_error);

    // setup
    san = "Nce4";  // specifying the c-file knight should resolve ambiguity

    // do
    PackedMove move = io::san_parser::deserialize(testingPosition, whiteToMove, san);
    
    // verify
    EXPECT_FALSE(move.isNull());
    EXPECT_EQ(move.sourceSqr(), Square::C3);
    EXPECT_EQ(move.targetSqr(), Square::E4);
}

TEST_F(SanParserFixture, CaptureMoveParsing_ShouldResolveEitherWithOrWithoutSpecifiedInSAN) {
    // setup
    std::string fen = "r1b2rk1/1p4p1/p1n1p3/3p1pB1/NqP3n1/b2BP3/1PQN1P1P/1K4RR";
    io::fen_parser::deserialize(fen, testingPosition.edit());
    std::string san = "Rxg4";  // Rook captures on g4
    bool whiteToMove = true;

    // do
    PackedMove move = io::san_parser::deserialize(testingPosition, whiteToMove, san);

    // verify
    EXPECT_FALSE(move.isNull());
    EXPECT_TRUE(move.isCapture());
    EXPECT_EQ(move.sourceSqr(), Square::G1);
    EXPECT_EQ(move.targetSqr(), Square::G4);
}

TEST_F(SanParserFixture, PawnPromotion) {
    // setup
    std::string fen = "2k1n3/4PP2/8/8/8/8/8/3K4 w - - 0 1";
    io::fen_parser::deserialize(fen, testingPosition.edit());
    std::string san = "f8=Q";  // Pawn promotes to Queen on
    bool whiteToMove = true;

    // do
    PackedMove move = io::san_parser::deserialize(testingPosition, whiteToMove, san);
    EXPECT_FALSE(move.isNull());
    EXPECT_TRUE(move.isPromotion());
    EXPECT_EQ(move.sourceSqr(), Square::F7);
    EXPECT_EQ(move.targetSqr(), Square::F8);
    EXPECT_EQ(move.readPromoteToPieceType(), static_cast<u16>(PieceType::QUEEN));

    san = "xe8=R+";  // Pawn captures and promotes to Rook
    move = io::san_parser::deserialize(testingPosition, whiteToMove, san);
    EXPECT_FALSE(move.isNull());
    EXPECT_TRUE(move.isPromotion());
    EXPECT_TRUE(move.isCapture());
    EXPECT_EQ(move.sourceSqr(), Square::F7);
    EXPECT_EQ(move.targetSqr(), Square::E8);
    EXPECT_EQ(move.readPromoteToPieceType(), static_cast<u16>(PieceType::ROOK));
}

TEST_F(SanParserFixture, CastlingMoveParsing_KingSide) {
    // setup 
    std::string fen = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1";
    io::fen_parser::deserialize(fen, testingPosition.edit());
    testingPosition.edit().castling().grantAll();
    std::string san = "O-O";  // King-side castling
    bool whiteToMove = true;

    // do
    PackedMove move = io::san_parser::deserialize(testingPosition, whiteToMove, san);
    EXPECT_FALSE(move.isNull());
    EXPECT_TRUE(move.isCastling());
    EXPECT_EQ(move.sourceSqr(), Square::E1);
    EXPECT_EQ(move.targetSqr(), Square::G1);

    san = "O-O-O";  // Queen-side castling
    move = io::san_parser::deserialize(testingPosition, whiteToMove, san);
    EXPECT_FALSE(move.isNull());
    EXPECT_TRUE(move.isCastling());
    EXPECT_EQ(move.sourceSqr(), Square::E1);
    EXPECT_EQ(move.targetSqr(), Square::C1);

    whiteToMove = false;
    san = "O-O#";  // Black King-side castling
    move = io::san_parser::deserialize(testingPosition, whiteToMove, san);
    EXPECT_FALSE(move.isNull());
    EXPECT_TRUE(move.isCastling());
    EXPECT_EQ(move.sourceSqr(), Square::E8);
    EXPECT_EQ(move.targetSqr(), Square::G8);
}

TEST_F(SanParserFixture, CheckingMoveParsing_ShouldIgnoreCheckIndicators) {
      // setup
    std::string fen = "rnbq3r/ppp2kpp/4pp2/3n4/2BP4/BQ3N2/P4PPP/4RRK1 w - - 0 1";
    io::fen_parser::deserialize(fen, testingPosition.edit());
    std::string san = "Ng5+";  // Knight move with check indicator
    bool whiteToMove = true;

    // do
    PackedMove move = io::san_parser::deserialize(testingPosition, whiteToMove, san);
    
    // verify
    EXPECT_FALSE(move.isNull());
    EXPECT_EQ(move.sourceSqr(), Square::F3);
    EXPECT_EQ(move.targetSqr(), Square::G5);
}

} // namespace ElephantTest