#include <gtest/gtest.h>
#include <array>
#include "elephant_test_utils.h"
#include "fen_parser.h"
#include "game_context.h"

namespace ElephantTest
{
////////////////////////////////////////////////////////////////
class FenParserFixture : public ::testing::Test
{
public:
    virtual void SetUp()
    {

    };
    virtual void TearDown() {};

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
    EXPECT_EQ(Notation(), testContext.readChessboard().readEnPassant());
    EXPECT_EQ(0x0f, testContext.readChessboard().readCastlingState());

    ChessPiece p(Set::BLACK, PieceType::ROOK);
    Notation n(56);
    const auto& piece = testContext.readChessboard().readTile(n).readPiece();
    PrintBoard(testContext.readChessboard());
    EXPECT_EQ(p, piece);
    
    n = Notation(63);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    p = ChessPiece(Set::BLACK, PieceType::KNIGHT);
    n = Notation(57);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());
    
    n = Notation(62);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    p = ChessPiece(Set::BLACK, PieceType::BISHOP);
    n = Notation(58);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    n = Notation(61);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    p = ChessPiece(Set::BLACK, PieceType::QUEEN);
    n = Notation(59);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    p = ChessPiece(Set::BLACK, PieceType::KING);
    n = Notation(60);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    p = ChessPiece(Set::BLACK, PieceType::PAWN);
    for (byte ind = 48; ind < 56; ++ind)
    {
        n = Notation(ind);
        EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());
    }

    p = ChessPiece(Set::WHITE, PieceType::ROOK);
    n = Notation(0);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    n = Notation(7);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    p = ChessPiece(Set::WHITE, PieceType::KNIGHT);
    n = Notation(1);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    n = Notation(6);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    p = ChessPiece(Set::WHITE, PieceType::BISHOP);
    n = Notation(2);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    n = Notation(5);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    p = ChessPiece(Set::WHITE, PieceType::QUEEN);
    n = Notation(3);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    p = ChessPiece(Set::WHITE, PieceType::KING);
    n = Notation(4);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    p = ChessPiece(Set::WHITE, PieceType::PAWN);
    for (byte ind = 8; ind < 16; ++ind)
    {
        n = Notation(ind);
        EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());
    }

    p = ChessPiece();
    for (byte ind = 16; ind < 48; ++ind)
    {
        n = Notation(ind);
        EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());
    }
}

// The so far longest game in the history of world championship chess.
// 2021 World Championship between Magnus Carlsen and Ian Nepomniachtchi.
// At this position, in game 6 Nepomniachtchi resigned.
TEST_F(FenParserFixture, NepomniachtchiResignsGameSix)
{
    std::string startingPositionFen("3k4/5RN1/4P3/5P2/7K/8/8/6q1 b - - 2 136");
    bool result = FENParser::deserialize(startingPositionFen.c_str(), testContext);
    EXPECT_TRUE(result);
    PrintBoard(testContext.readChessboard());
    EXPECT_EQ(2, testContext.readPly());
    EXPECT_EQ(136, testContext.readMoveCount());
    EXPECT_EQ(Set::BLACK, testContext.readToPlay());
    EXPECT_EQ(Notation(), testContext.readChessboard().readEnPassant());
    EXPECT_EQ(0x00, testContext.readChessboard().readCastlingState());

    Chessboard expected;
    expected.editTile(g1).editPiece() = BLACKQUEEN;
    expected.editTile(d8).editPiece() = BLACKKING;
    expected.editTile(e6).editPiece() = WHITEPAWN;
    expected.editTile(f5).editPiece() = WHITEPAWN;
    expected.editTile(g7).editPiece() = WHITEKNIGHT;
    expected.editTile(f7).editPiece() = WHITEROOK;
    expected.editTile(h4).editPiece() = WHITEKING;

    auto&& expitr = expected.begin();
    auto&& resitr = testContext.readChessboard().begin();
    while (expitr != expected.end())
    {
        auto expectedPiece = (*expitr).readPiece();
        auto actualPiece = (*resitr).readPiece();
        EXPECT_EQ(expectedPiece, actualPiece);
        ++expitr;
        ++resitr;
    }
}

TEST_F(FenParserFixture, PerftPositionThree)
{
	std::string fen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    bool result = FENParser::deserialize(fen.c_str(), testContext);

    EXPECT_TRUE(result);
    PrintBoard(testContext.readChessboard());
    EXPECT_EQ(0, testContext.readPly());
    EXPECT_EQ(1, testContext.readMoveCount());
    EXPECT_EQ(Set::WHITE, testContext.readToPlay());
    EXPECT_EQ(Notation(), testContext.readChessboard().readEnPassant());
    EXPECT_EQ(0x00, testContext.readChessboard().readCastlingState());

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

////////////////////////////////////////////////////////////////

} // namespace ElephantTest