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
    PackedMove move = san_parser::deserialize(testingPosition, whiteToMove, san);

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
        san_parser::deserialize(testingPosition, whiteToMove, san);
    }, ephant::io_error);
}

TEST_F(SanParserFixture, BulkTest_GenericParsingTest) {
    // setup
    fen_parser::deserialize("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", testingPosition.edit());
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
        PackedMove move = san_parser::deserialize(testingPosition, whiteToMove, testCase.san);

        // verify
        EXPECT_FALSE(move.isNull());        
        EXPECT_EQ(move.targetSqr(), testCase.expectedTarget);
        EXPECT_EQ(move.sourceSqr(), testCase.expectedSource);
    }
}

TEST_F(SanParserFixture, AmbiguousMoveParsing_UnableToResolveAmbiguity_ShouldThrow) {
    // setup
    fen_parser::deserialize("3k4/8/8/8/8/2N3N1/8/3K4 w - - 0 1", testingPosition.edit());
    std::string san = "Nd5";  // Both knights can move to d5, this should be ambiguous
    bool whiteToMove = true;

    // do
    EXPECT_THROW({
        san_parser::deserialize(testingPosition, whiteToMove, san);
    }, ephant::io_error);

    // setup
    san = "Nce4";  // specifying the c-file knight should resolve ambiguity

    // do
    PackedMove move = san_parser::deserialize(testingPosition, whiteToMove, san);
    
    // verify
    EXPECT_FALSE(move.isNull());
    EXPECT_EQ(move.sourceSqr(), Square::C3);
    EXPECT_EQ(move.targetSqr(), Square::E4);
}

TEST_F(SanParserFixture, CaptureMoveParsing_ShouldResolveEitherWithOrWithoutSpecifiedInSAN) {
    

}

TEST_F(SanParserFixture, PawnPromotion) {

}

} // namespace ElephantTest