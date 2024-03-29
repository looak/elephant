#include "fen_parser.h"
#include <gtest/gtest.h>
#include <array>
#include "elephant_test_utils.h"
#include "game_context.h"

namespace ElephantTest {
////////////////////////////////////////////////////////////////
class FenParserFixture : public ::testing::Test {
public:
    virtual void SetUp(){

    };
    virtual void TearDown(){};

    GameContext testContext;
};
////////////////////////////////////////////////////////////////
TEST_F(FenParserFixture, Initialize)
{
    std::string empty = "";
    bool result = FENParser::deserialize(empty.c_str(), testContext);
    EXPECT_FALSE(result);
}

TEST_F(FenParserFixture, StartingPosition)
{
    std::string startingPositionFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    bool result = FENParser::deserialize(startingPositionFen.c_str(), testContext);
    EXPECT_TRUE(result);

    EXPECT_EQ(0, testContext.readPly());
    EXPECT_EQ(1, testContext.readMoveCount());
    EXPECT_EQ(Set::WHITE, testContext.readToPlay());
    EXPECT_FALSE(testContext.readChessboard().readPosition().readEnPassant());
    EXPECT_TRUE(testContext.readChessboard().readCastlingState().hasAll());

    ChessPiece p(Set::BLACK, PieceType::ROOK);
    Notation n(56);
    const auto& piece = testContext.readChessboard().readPieceAt(n.toSquare());
    PrintBoard(testContext.readChessboard());
    EXPECT_EQ(p, piece);

    n = Notation(63);
    EXPECT_EQ(p, testContext.readChessboard().readPieceAt(n.toSquare()));

    p = ChessPiece(Set::BLACK, PieceType::KNIGHT);
    n = Notation(57);
    EXPECT_EQ(p, testContext.readChessboard().readPieceAt(n.toSquare()));

    n = Notation(62);
    EXPECT_EQ(p, testContext.readChessboard().readPieceAt(n.toSquare()));

    p = ChessPiece(Set::BLACK, PieceType::BISHOP);
    n = Notation(58);
    EXPECT_EQ(p, testContext.readChessboard().readPieceAt(n.toSquare()));

    n = Notation(61);
    EXPECT_EQ(p, testContext.readChessboard().readPieceAt(n.toSquare()));

    p = ChessPiece(Set::BLACK, PieceType::QUEEN);
    n = Notation(59);
    EXPECT_EQ(p, testContext.readChessboard().readPieceAt(n.toSquare()));

    p = ChessPiece(Set::BLACK, PieceType::KING);
    n = Notation(60);
    EXPECT_EQ(p, testContext.readChessboard().readPieceAt(n.toSquare()));

    p = ChessPiece(Set::BLACK, PieceType::PAWN);
    for (byte ind = 48; ind < 56; ++ind) {
        n = Notation(ind);
        EXPECT_EQ(p, testContext.readChessboard().readPieceAt(n.toSquare()));
    }

    p = ChessPiece(Set::WHITE, PieceType::ROOK);
    n = Notation(0);
    EXPECT_EQ(p, testContext.readChessboard().readPieceAt(n.toSquare()));

    n = Notation(7);
    EXPECT_EQ(p, testContext.readChessboard().readPieceAt(n.toSquare()));

    p = ChessPiece(Set::WHITE, PieceType::KNIGHT);
    n = Notation(1);
    EXPECT_EQ(p, testContext.readChessboard().readPieceAt(n.toSquare()));

    n = Notation(6);
    EXPECT_EQ(p, testContext.readChessboard().readPieceAt(n.toSquare()));

    p = ChessPiece(Set::WHITE, PieceType::BISHOP);
    n = Notation(2);
    EXPECT_EQ(p, testContext.readChessboard().readPieceAt(n.toSquare()));

    n = Notation(5);
    EXPECT_EQ(p, testContext.readChessboard().readPieceAt(n.toSquare()));

    p = ChessPiece(Set::WHITE, PieceType::QUEEN);
    n = Notation(3);
    EXPECT_EQ(p, testContext.readChessboard().readPieceAt(n.toSquare()));

    p = ChessPiece(Set::WHITE, PieceType::KING);
    n = Notation(4);
    EXPECT_EQ(p, testContext.readChessboard().readPieceAt(n.toSquare()));

    p = ChessPiece(Set::WHITE, PieceType::PAWN);
    for (byte ind = 8; ind < 16; ++ind) {
        n = Notation(ind);
        EXPECT_EQ(p, testContext.readChessboard().readPieceAt(n.toSquare()));
    }

    p = ChessPiece();
    for (byte ind = 16; ind < 48; ++ind) {
        n = Notation(ind);
        EXPECT_EQ(p, testContext.readChessboard().readPieceAt(n.toSquare()));
    }
}

// The so far longest game in the history of world championship chess.
// 2021 World Championship between Magnus Carlsen and Ian Nepomniachtchi.
// At this position, in game 6 Nepomniachtchi resigned.
TEST_F(FenParserFixture, NepomniachtchiResignsGameSix)
{
    std::string gameSixFen("3k4/5RN1/4P3/5P2/7K/8/8/6q1 b - - 2 136");
    bool result = FENParser::deserialize(gameSixFen.c_str(), testContext);
    EXPECT_TRUE(result);
    // PrintBoard(testContext.readChessboard());
    EXPECT_EQ(2, testContext.readPly());
    EXPECT_EQ(136, testContext.readMoveCount());
    EXPECT_EQ(Set::BLACK, testContext.readToPlay());
    EXPECT_FALSE(testContext.readChessboard().readPosition().readEnPassant());
    EXPECT_TRUE(testContext.readChessboard().readCastlingState().hasNone());

    Chessboard expected;
    expected.PlacePiece(BLACKQUEEN, g1);
    expected.PlacePiece(BLACKKING, d8);
    expected.PlacePiece(WHITEPAWN, e6);
    expected.PlacePiece(WHITEPAWN, f5);
    expected.PlacePiece(WHITEKNIGHT, g7);
    expected.PlacePiece(WHITEROOK, f7);
    expected.PlacePiece(WHITEKING, h4);

    auto&& expitr = expected.begin();
    auto&& resitr = testContext.readChessboard().begin();
    while (expitr != expected.end()) {
        auto expectedPiece = expitr.get();
        auto actualPiece = resitr.get();
        EXPECT_EQ(expectedPiece, actualPiece);
        ++expitr;
        ++resitr;
    }

    // round trip
    std::string output;
    FENParser::serialize(testContext, output);
    EXPECT_TRUE(result);
    EXPECT_EQ(gameSixFen, output);
}

TEST_F(FenParserFixture, PerftPositionThree)
{
    std::string fen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    bool result = FENParser::deserialize(fen.c_str(), testContext);

    EXPECT_TRUE(result);
    // PrintBoard(testContext.readChessboard());
    EXPECT_EQ(0, testContext.readPly());
    EXPECT_EQ(1, testContext.readMoveCount());
    EXPECT_EQ(Set::WHITE, testContext.readToPlay());
    EXPECT_FALSE(testContext.readChessboard().readPosition().readEnPassant());
    EXPECT_TRUE(testContext.readChessboard().readCastlingState().hasNone());
}

TEST_F(FenParserFixture, SerializeDefaultPosition)
{
    GameContext context;
    SetupDefaultStartingPosition(context.editChessboard());
    std::string output;
    bool result = FENParser::serialize(context, output);

    const std::string expected = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    EXPECT_TRUE(result);
    EXPECT_EQ(expected, output);
}

TEST_F(FenParserFixture, EnPassantPlyMovePlay_RoundTripSerialize)
{
    GameContext context;
    std::string fen("8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 5 19");
    FENParser::deserialize(fen.c_str(), context);

    EXPECT_EQ(5, context.readPly());
    EXPECT_EQ(19, context.readMoveCount());
    EXPECT_EQ(Set::BLACK, context.readToPlay());
    auto sqr = context.readChessboard().readPosition().readEnPassant().readSquare();
    EXPECT_EQ(Square::D3, sqr);

    std::string output;
    bool result = FENParser::serialize(context, output);
    EXPECT_EQ(fen, output);
    EXPECT_TRUE(result);
}

////////////////////////////////////////////////////////////////

}  // namespace ElephantTest