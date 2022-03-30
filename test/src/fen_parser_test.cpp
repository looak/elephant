#include <gtest/gtest.h>
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

bool PrintBoard(const Chessboard& board)
{
    auto boardItr = board.begin();
    std::array<std::stringstream, 8> ranks;
        
    byte prevRank = -1;
    do 
    {
        if (prevRank != boardItr.rank())
        {
            ranks[boardItr.rank()] << (int)(boardItr.rank() + 1) << "  ";
        }
        
        ranks[boardItr.rank()] << '[' << (*boardItr).readPiece().toString() << ']';
        prevRank = boardItr.rank();
        ++boardItr;

    } while (boardItr != board.end());

    auto rankItr = ranks.rbegin();
    while (rankItr != ranks.rend())
    {        
        LOG_INFO() << (*rankItr).str().c_str();
        rankItr++;
    }

    LOG_INFO() << "    A  B  C  D  E  F  G  H";

    return true;
}

TEST_F(FenParserFixture, StartingPosition)
{
    std::string startingPositionFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    bool result = FENParser::deserialize(startingPositionFen.c_str(), testContext);
    EXPECT_TRUE(result);
    
    EXPECT_EQ(0, testContext.readPly());
    EXPECT_EQ(1, testContext.readMoveCount());
    EXPECT_EQ(PieceSet::WHITE, testContext.readToPlay());
    EXPECT_EQ(Notation(), testContext.readChessboard().readEnPassant());
    EXPECT_EQ(0x0f, testContext.readChessboard().readCastlingState());

    ChessPiece p(PieceSet::BLACK, PieceType::ROOK);
    Notation n(56);
    EXPECT_NE(Notation(), n);
    const auto& piece = testContext.readChessboard().readTile(n).readPiece();
    PrintBoard(testContext.readChessboard());
    EXPECT_EQ(p, piece);
    ChessboardTile tile;
    EXPECT_NE(tile, testContext.readChessboard().readTile(n));
    
    n = Notation(63);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    p = ChessPiece(PieceSet::BLACK, PieceType::KNIGHT);
    n = Notation(57);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());
    
    n = Notation(62);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    p = ChessPiece(PieceSet::BLACK, PieceType::BISHOP);
    n = Notation(58);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    n = Notation(61);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    p = ChessPiece(PieceSet::BLACK, PieceType::QUEEN);
    n = Notation(59);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    p = ChessPiece(PieceSet::BLACK, PieceType::KING);
    n = Notation(60);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    p = ChessPiece(PieceSet::BLACK, PieceType::PAWN);
    for (byte ind = 48; ind < 56; ++ind)
    {
        n = Notation(ind);
        EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());
    }

    p = ChessPiece(PieceSet::WHITE, PieceType::ROOK);
    n = Notation(0);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    n = Notation(7);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    p = ChessPiece(PieceSet::WHITE, PieceType::KNIGHT);
    n = Notation(1);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    n = Notation(6);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    p = ChessPiece(PieceSet::WHITE, PieceType::BISHOP);
    n = Notation(2);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    n = Notation(5);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    p = ChessPiece(PieceSet::WHITE, PieceType::QUEEN);
    n = Notation(3);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    p = ChessPiece(PieceSet::WHITE, PieceType::KING);
    n = Notation(4);
    EXPECT_EQ(p, testContext.readChessboard().readTile(n).readPiece());

    p = ChessPiece(PieceSet::WHITE, PieceType::PAWN);
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
    EXPECT_EQ(PieceSet::BLACK, testContext.readToPlay());
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

////////////////////////////////////////////////////////////////

} // namespace ElephantTest