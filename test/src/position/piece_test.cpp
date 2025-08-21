#include <gtest/gtest.h>
#include <material/chess_piece.hpp>

namespace ElephantTest {
////////////////////////////////////////////////////////////////
class ChessPieceFixture : public ::testing::Test {
public:
    /*   virtual void SetUp()
    {

    };
    virtual void TearDown() {};
*/
};
////////////////////////////////////////////////////////////////
TEST_F(ChessPieceFixture, Empty)
{
    ChessPiece p;
    EXPECT_EQ(PieceType::NONE, p.getType()) << "By default we have no type set.";
    EXPECT_EQ(Set::WHITE, p.getSet()) << "Default set is white";
}

TEST_F(ChessPieceFixture, AllPieces)
{
    ChessPiece whitePawn(Set::WHITE, PieceType::PAWN);
    EXPECT_EQ(PieceType::PAWN, whitePawn.getType());
    EXPECT_EQ(Set::WHITE, whitePawn.getSet());

    ChessPiece blackPawn(Set::BLACK, PieceType::PAWN);
    EXPECT_EQ(PieceType::PAWN, blackPawn.getType());
    EXPECT_EQ(Set::BLACK, blackPawn.getSet());

    ChessPiece whiteKnight(Set::WHITE, PieceType::KNIGHT);
    EXPECT_EQ(PieceType::KNIGHT, whiteKnight.getType());
    EXPECT_EQ(Set::WHITE, whiteKnight.getSet());

    ChessPiece blackKnight(Set::BLACK, PieceType::KNIGHT);
    EXPECT_EQ(PieceType::KNIGHT, blackKnight.getType());
    EXPECT_EQ(Set::BLACK, blackKnight.getSet());

    ChessPiece whiteBishop(Set::WHITE, PieceType::BISHOP);
    EXPECT_EQ(PieceType::BISHOP, whiteBishop.getType());
    EXPECT_EQ(Set::WHITE, whiteBishop.getSet());

    ChessPiece blackBishop(Set::BLACK, PieceType::BISHOP);
    EXPECT_EQ(PieceType::BISHOP, blackBishop.getType());
    EXPECT_EQ(Set::BLACK, blackBishop.getSet());

    ChessPiece whiteRook(Set::WHITE, PieceType::ROOK);
    EXPECT_EQ(PieceType::ROOK, whiteRook.getType());
    EXPECT_EQ(Set::WHITE, whiteRook.getSet());

    ChessPiece blackRook(Set::BLACK, PieceType::ROOK);
    EXPECT_EQ(PieceType::ROOK, blackRook.getType());
    EXPECT_EQ(Set::BLACK, blackRook.getSet());

    ChessPiece whiteQueen(Set::WHITE, PieceType::QUEEN);
    EXPECT_EQ(PieceType::QUEEN, whiteQueen.getType());
    EXPECT_EQ(Set::WHITE, whiteQueen.getSet());

    ChessPiece blackQueen(Set::BLACK, PieceType::QUEEN);
    EXPECT_EQ(PieceType::QUEEN, blackQueen.getType());
    EXPECT_EQ(Set::BLACK, blackQueen.getSet());

    ChessPiece whiteKing(Set::WHITE, PieceType::KING);
    EXPECT_EQ(PieceType::KING, whiteKing.getType());
    EXPECT_EQ(Set::WHITE, whiteKing.getSet());

    ChessPiece blackKing(Set::BLACK, PieceType::KING);
    EXPECT_EQ(PieceType::KING, blackKing.getType());
    EXPECT_EQ(Set::BLACK, blackKing.getSet());
}

TEST_F(ChessPieceFixture, AssignmentOperations)
{
    ChessPiece blackPawn(Set::BLACK, PieceType::PAWN);
    EXPECT_EQ(PieceType::PAWN, blackPawn.getType());
    EXPECT_EQ(Set::BLACK, blackPawn.getSet());

    blackPawn = ChessPiece();
    EXPECT_EQ(PieceType::NONE, blackPawn.getType());
    EXPECT_EQ(Set::WHITE, blackPawn.getSet());

    blackPawn = ChessPiece(Set::BLACK, PieceType::QUEEN);
    EXPECT_EQ(PieceType::QUEEN, blackPawn.getType());
    EXPECT_EQ(Set::BLACK, blackPawn.getSet());

    blackPawn = ChessPiece();
    EXPECT_EQ(PieceType::NONE, blackPawn.getType());
    EXPECT_EQ(Set::WHITE, blackPawn.getSet());

    blackPawn = ChessPiece(Set::BLACK, PieceType::KING);
    EXPECT_EQ(PieceType::KING, blackPawn.getType());
    EXPECT_EQ(Set::BLACK, blackPawn.getSet());

    ChessPiece whiteRook(Set::WHITE, PieceType::ROOK);
    EXPECT_EQ(PieceType::ROOK, whiteRook.getType());
    EXPECT_EQ(Set::WHITE, whiteRook.getSet());

    blackPawn = whiteRook;
    EXPECT_EQ(PieceType::ROOK, blackPawn.getType());
    EXPECT_EQ(Set::WHITE, blackPawn.getSet());
}

TEST_F(ChessPieceFixture, EqualityOperations)
{
    ChessPiece blackPawn(Set::BLACK, PieceType::PAWN);
    EXPECT_EQ(blackPawn, blackPawn);

    ChessPiece anotherBlackPawn(Set::BLACK, PieceType::PAWN);
    EXPECT_EQ(blackPawn, anotherBlackPawn);

    ChessPiece whitePawn(Set::WHITE, PieceType::PAWN);
    EXPECT_NE(blackPawn, whitePawn);

    ChessPiece whiteRook(Set::WHITE, PieceType::ROOK);
    EXPECT_EQ(whiteRook, whiteRook);

    ChessPiece anotherWhiteRook(Set::WHITE, PieceType::ROOK);
    EXPECT_EQ(whiteRook, anotherWhiteRook);
}

TEST_F(ChessPieceFixture, ToStringOperations)
{
    ChessPiece blackPawn(Set::BLACK, PieceType::PAWN);
    EXPECT_EQ('p', blackPawn.toString());

    ChessPiece blackKnight(Set::BLACK, PieceType::KNIGHT);
    EXPECT_EQ('n', blackKnight.toString());

    ChessPiece blackBishop(Set::BLACK, PieceType::BISHOP);
    EXPECT_EQ('b', blackBishop.toString());

    ChessPiece blackRook(Set::BLACK, PieceType::ROOK);
    EXPECT_EQ('r', blackRook.toString());

    ChessPiece blackQueen(Set::BLACK, PieceType::QUEEN);
    EXPECT_EQ('q', blackQueen.toString());

    ChessPiece blackKing(Set::BLACK, PieceType::KING);
    EXPECT_EQ('k', blackKing.toString());

    ChessPiece whitePawn(Set::WHITE, PieceType::PAWN);
    EXPECT_EQ('P', whitePawn.toString());
    EXPECT_NE(blackPawn.toString(), whitePawn.toString());

    ChessPiece whiteKnight(Set::WHITE, PieceType::KNIGHT);
    EXPECT_EQ('N', whiteKnight.toString());

    ChessPiece whiteBishop(Set::WHITE, PieceType::BISHOP);
    EXPECT_EQ('B', whiteBishop.toString());

    ChessPiece whiteRook(Set::WHITE, PieceType::ROOK);
    EXPECT_EQ('R', whiteRook.toString());

    ChessPiece whiteQueen(Set::WHITE, PieceType::QUEEN);
    EXPECT_EQ('Q', whiteQueen.toString());

    ChessPiece whiteKing(Set::WHITE, PieceType::KING);
    EXPECT_EQ('K', whiteKing.toString());
}

TEST_F(ChessPieceFixture, FromStringOperations)
{
    ChessPiece testPiece;
    EXPECT_FALSE(testPiece.fromString('a'));
    EXPECT_FALSE(testPiece.fromString('z'));

    ChessPiece blackPawn(Set::BLACK, PieceType::PAWN);
    EXPECT_TRUE(testPiece.fromString('p'));
    EXPECT_EQ(blackPawn, testPiece);

    ChessPiece blackKnight(Set::BLACK, PieceType::KNIGHT);
    EXPECT_TRUE(testPiece.fromString('n'));
    EXPECT_EQ(blackKnight, testPiece);

    ChessPiece blackBishop(Set::BLACK, PieceType::BISHOP);
    EXPECT_TRUE(testPiece.fromString('b'));
    EXPECT_EQ(blackBishop, testPiece);

    ChessPiece blackRook(Set::BLACK, PieceType::ROOK);
    EXPECT_TRUE(testPiece.fromString('r'));
    EXPECT_EQ(blackRook, testPiece);

    ChessPiece blackQueen(Set::BLACK, PieceType::QUEEN);
    EXPECT_TRUE(testPiece.fromString('q'));
    EXPECT_EQ(blackQueen, testPiece);

    ChessPiece blackKing(Set::BLACK, PieceType::KING);
    EXPECT_TRUE(testPiece.fromString('k'));
    EXPECT_EQ(blackKing, testPiece);

    ChessPiece whitePawn(Set::WHITE, PieceType::PAWN);
    EXPECT_TRUE(testPiece.fromString('P'));
    EXPECT_EQ(whitePawn, testPiece);

    ChessPiece whiteKnight(Set::WHITE, PieceType::KNIGHT);
    EXPECT_TRUE(testPiece.fromString('N'));
    EXPECT_EQ(whiteKnight, testPiece);

    ChessPiece whiteBishop(Set::WHITE, PieceType::BISHOP);
    EXPECT_TRUE(testPiece.fromString('B'));
    EXPECT_EQ(whiteBishop, testPiece);

    ChessPiece whiteRook(Set::WHITE, PieceType::ROOK);
    EXPECT_TRUE(testPiece.fromString('R'));
    EXPECT_EQ(whiteRook, testPiece);

    ChessPiece whiteQueen(Set::WHITE, PieceType::QUEEN);
    EXPECT_TRUE(testPiece.fromString('Q'));
    EXPECT_EQ(whiteQueen, testPiece);

    ChessPiece whiteKing(Set::WHITE, PieceType::KING);
    EXPECT_TRUE(testPiece.fromString('K'));
    EXPECT_EQ(whiteKing, testPiece);
}

TEST_F(ChessPieceFixture, FlipSet)
{
    ChessPiece whiteBishop(Set::WHITE, PieceType::BISHOP);
    Set expected = Set::BLACK;
    Set set = ChessPiece::FlipSet(whiteBishop.getSet());
    EXPECT_EQ(expected, set);

    expected = Set::WHITE;
    set = ChessPiece::FlipSet(set);
    EXPECT_EQ(expected, set);

    byte exp = 0;
    byte res = (byte)Set::BLACK;
    res = ChessPiece::FlipSet(res);
    EXPECT_EQ(exp, res);

    exp = 1;
    res = ChessPiece::FlipSet(res);
    EXPECT_EQ(exp, res);
}

////////////////////////////////////////////////////////////////

}  // namespace ElephantTest