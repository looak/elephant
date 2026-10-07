#include <gtest/gtest.h>
#include <array>

#include <io/fen_parser.hpp>
#include <core/chessboard.hpp>
#include <io/printer.hpp>

#include "chess_positions.hpp"


namespace ElephantTest {
////////////////////////////////////////////////////////////////

/**
 * @file fen_parser_test.cpp
 * @brief Testing serializing and deserializing functionality of fen parser.
 * @author Alexander Loodin Ek    */
class FenParserFixture : public ::testing::Test {
public:
    virtual void SetUp(){

    };
    virtual void TearDown(){};

};
////////////////////////////////////////////////////////////////
TEST_F(FenParserFixture, Initialize)
{
    Chessboard board;
    std::string empty = "";
    bool result = io::fen_parser::deserialize(empty.c_str(), board);
    EXPECT_FALSE(result);
}

TEST_F(FenParserFixture, StartingPosition)
{
    std::string startingPositionFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Chessboard newGameBoard;
    PositionReader posReader = newGameBoard.readPosition();
    bool result = io::fen_parser::deserialize(startingPositionFen, newGameBoard);
    EXPECT_TRUE(result);

    EXPECT_EQ(0, newGameBoard.readPlyCount());
    EXPECT_EQ(1, newGameBoard.readMoveCount());
    EXPECT_EQ(Set::WHITE, newGameBoard.readToPlay());
    EXPECT_FALSE(posReader.enPassant());
    EXPECT_TRUE(posReader.castling().hasAll());

    io::printer::position(std::cout, newGameBoard.readPosition());

    Chessboard expected;
    chess_positions::defaultStartingPosition(expected.editPosition());
    EXPECT_TRUE(expected.compare(newGameBoard));
}

TEST_F(FenParserFixture, StartingPosition_OnlyPositionNoState)
{
    // setup
    std::string startingPositionFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";    
    Position position;

    // do
    bool result = io::fen_parser::deserialize(startingPositionFen, position);

    // verify
    EXPECT_TRUE(result);

    Chessboard expected;
    chess_positions::defaultStartingPosition(expected.editPosition());
    position.edit().castling().grantAll();
    position.edit().enPassant().clear();

    EXPECT_TRUE(expected.compare(position));

    // without state
    std::string noStateFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR";
    Position positionNoState;
    result = io::fen_parser::deserialize(noStateFen, positionNoState);

    EXPECT_TRUE(result);
    position.edit().castling().clear();

    EXPECT_EQ(position, positionNoState);
}

// The so far longest game in the history of world championship chess.
// 2021 World Championship between Magnus Carlsen and Ian Nepomniachtchi.
// At this position, in game 6 Nepomniachtchi resigned.
TEST_F(FenParserFixture, NepomniachtchiResignsGameSix)
{
    std::string gameSixFen("3k4/5RN1/4P3/5P2/7K/8/8/6q1 b - - 2 136");
    Chessboard resultBoard;
    bool result = io::fen_parser::deserialize(gameSixFen, resultBoard);
    EXPECT_TRUE(result);
    // PrintBoard(testContext.readChessboard());
    EXPECT_EQ(2, resultBoard.readPlyCount());
    EXPECT_EQ(136, resultBoard.readMoveCount());
    EXPECT_EQ(Set::BLACK, resultBoard.readToPlay());
    EXPECT_FALSE(resultBoard.readPosition().enPassant());
    EXPECT_TRUE(resultBoard.readPosition().castling().hasNone());

    Chessboard expected;
    chess_positions::nepomniachtchiResignsGameSix(expected.editPosition());
    expected.editState().moveCount = 136;
    expected.editState().plyCount = 2;
    expected.editState().whiteToMove = false;

    auto&& expitr = expected.readPosition().begin();
    auto&& resitr = resultBoard.readPosition().begin();
    while (expitr != expected.readPosition().end()) {
        auto expectedPiece = expitr.get();
        auto actualPiece = resitr.get();
        EXPECT_EQ(expectedPiece, actualPiece);
        ++expitr;
        ++resitr;
    }

    // adding this for sanity sake, making sure all our compares are actually agreeing on the result.
    EXPECT_TRUE(expected.compare(resultBoard)) << "This should essentially do the same thing as the while loop above.";

    // round trip
    std::string output;
    io::fen_parser::serialize(resultBoard, output);
    EXPECT_TRUE(result);
    EXPECT_EQ(gameSixFen, output);
}

TEST_F(FenParserFixture, PerftPositionThree)
{
    Chessboard testBoard;
    std::string fen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    bool result = io::fen_parser::deserialize(fen, testBoard);

    EXPECT_TRUE(result);
    
    EXPECT_EQ(0, testBoard.readPlyCount());
    EXPECT_EQ(1, testBoard.readMoveCount());
    EXPECT_EQ(Set::WHITE, testBoard.readToPlay());
    EXPECT_FALSE(testBoard.readPosition().enPassant());
    EXPECT_TRUE(testBoard.readPosition().castling().hasNone());
}

TEST_F(FenParserFixture, SerializeDefaultPosition)
{
    Chessboard testingBoard;
    chess_positions::defaultStartingPosition(testingBoard.editPosition());
    std::string output;
    bool result = io::fen_parser::serialize(testingBoard, output);

    const std::string expected = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    EXPECT_TRUE(result);
    EXPECT_EQ(expected, output);
}

TEST_F(FenParserFixture, EnPassantPlyMovePlay_RoundTripSerialize)
{
    Chessboard testingBoard;
    std::string fen("8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 5 19");
    io::fen_parser::deserialize(fen, testingBoard);

    EXPECT_EQ(5, testingBoard.readPlyCount());
    EXPECT_EQ(19, testingBoard.readMoveCount());
    EXPECT_EQ(Set::BLACK, testingBoard.readToPlay());
    auto sqr = testingBoard.readPosition().enPassant().readSquare();
    EXPECT_EQ(Square::D3, sqr);

    std::string output;
    bool result = io::fen_parser::serialize(testingBoard, output);
    EXPECT_EQ(fen, output);
    EXPECT_TRUE(result);
}

////////////////////////////////////////////////////////////////

}  // namespace ElephantTest