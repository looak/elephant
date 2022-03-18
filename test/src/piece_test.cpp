#include <gtest/gtest.h>
#include "chess_piece.h"

namespace ElephantTest
{
////////////////////////////////////////////////////////////////
class ChessPieceFixture : public ::testing::Test
{
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
    EXPECT_EQ(PieceType::NON, p.getType()) << "By default we have no type set.";
    EXPECT_EQ(PieceSet::WHITE, p.getSet()) << "Default set is white";
}

TEST_F(ChessPieceFixture, AllPieces)
{
    ChessPiece whitePawn(PieceSet::WHITE, PieceType::PAWN);
    EXPECT_EQ(PieceType::PAWN, whitePawn.getType());
    EXPECT_EQ(PieceSet::WHITE, whitePawn.getSet());

    ChessPiece blackPawn(PieceSet::BLACK, PieceType::PAWN);
    EXPECT_EQ(PieceType::PAWN, blackPawn.getType());
    EXPECT_EQ(PieceSet::BLACK, blackPawn.getSet());

    ChessPiece whiteKnight(PieceSet::WHITE, PieceType::KNIGHT);
    EXPECT_EQ(PieceType::KNIGHT, whiteKnight.getType());
    EXPECT_EQ(PieceSet::WHITE, whiteKnight.getSet());

    ChessPiece blackKnight(PieceSet::BLACK, PieceType::KNIGHT);
    EXPECT_EQ(PieceType::KNIGHT, blackKnight.getType());
    EXPECT_EQ(PieceSet::BLACK, blackKnight.getSet());

    ChessPiece whiteBishop(PieceSet::WHITE, PieceType::BISHOP);
    EXPECT_EQ(PieceType::BISHOP, whiteBishop.getType());
    EXPECT_EQ(PieceSet::WHITE, whiteBishop.getSet());

    ChessPiece blackBishop(PieceSet::BLACK, PieceType::BISHOP);
    EXPECT_EQ(PieceType::BISHOP, blackBishop.getType());
    EXPECT_EQ(PieceSet::BLACK, blackBishop.getSet());

    ChessPiece whiteRook(PieceSet::WHITE, PieceType::ROOK);
    EXPECT_EQ(PieceType::ROOK, whiteRook.getType());
    EXPECT_EQ(PieceSet::WHITE, whiteRook.getSet());

    ChessPiece blackRook(PieceSet::BLACK, PieceType::ROOK);
    EXPECT_EQ(PieceType::ROOK, blackRook.getType());
    EXPECT_EQ(PieceSet::BLACK, blackRook.getSet());

    ChessPiece whiteQueen(PieceSet::WHITE, PieceType::QUEEN);
    EXPECT_EQ(PieceType::QUEEN, whiteQueen.getType());
    EXPECT_EQ(PieceSet::WHITE, whiteQueen.getSet());

    ChessPiece blackQueen(PieceSet::BLACK, PieceType::QUEEN);
    EXPECT_EQ(PieceType::QUEEN, blackQueen.getType());
    EXPECT_EQ(PieceSet::BLACK, blackQueen.getSet());

    ChessPiece whiteKing(PieceSet::WHITE, PieceType::KING);
    EXPECT_EQ(PieceType::KING, whiteKing.getType());
    EXPECT_EQ(PieceSet::WHITE, whiteKing.getSet());

    ChessPiece blackKing(PieceSet::BLACK, PieceType::KING);
    EXPECT_EQ(PieceType::KING, blackKing.getType());
    EXPECT_EQ(PieceSet::BLACK, blackKing.getSet());
}

TEST_F(ChessPieceFixture, AssignmentOperations)
{
    ChessPiece blackPawn(PieceSet::BLACK, PieceType::PAWN);
    EXPECT_EQ(PieceType::PAWN, blackPawn.getType());
    EXPECT_EQ(PieceSet::BLACK, blackPawn.getSet()); 

    blackPawn = ChessPiece();
    EXPECT_EQ(PieceType::NON, blackPawn.getType());
    EXPECT_EQ(PieceSet::WHITE, blackPawn.getSet());

    blackPawn = ChessPiece(PieceSet::BLACK, PieceType::QUEEN);
    EXPECT_EQ(PieceType::QUEEN, blackPawn.getType());
    EXPECT_EQ(PieceSet::BLACK, blackPawn.getSet());

    blackPawn = ChessPiece();
    EXPECT_EQ(PieceType::NON, blackPawn.getType());
    EXPECT_EQ(PieceSet::WHITE, blackPawn.getSet());

    blackPawn = ChessPiece(PieceSet::BLACK, PieceType::KING);
    EXPECT_EQ(PieceType::KING, blackPawn.getType());
    EXPECT_EQ(PieceSet::BLACK, blackPawn.getSet());

    ChessPiece whiteRook(PieceSet::WHITE, PieceType::ROOK);
    EXPECT_EQ(PieceType::ROOK, whiteRook.getType());
    EXPECT_EQ(PieceSet::WHITE, whiteRook.getSet());

    blackPawn = whiteRook;
    EXPECT_EQ(PieceType::ROOK, blackPawn.getType());
    EXPECT_EQ(PieceSet::WHITE, blackPawn.getSet());
}

TEST_F(ChessPieceFixture, EqualityOperations)
{   
    ChessPiece blackPawn(PieceSet::BLACK, PieceType::PAWN);
    EXPECT_EQ(blackPawn, blackPawn);
    
    ChessPiece anotherBlackPawn(PieceSet::BLACK, PieceType::PAWN);
    EXPECT_EQ(blackPawn, anotherBlackPawn);   

    ChessPiece whitePawn(PieceSet::WHITE, PieceType::PAWN);
    EXPECT_NE(blackPawn, whitePawn);
    
    ChessPiece whiteRook(PieceSet::WHITE, PieceType::ROOK);
    EXPECT_EQ(whiteRook, whiteRook);

    ChessPiece anotherWhiteRook(PieceSet::WHITE, PieceType::ROOK);
    EXPECT_EQ(whiteRook, anotherWhiteRook);
}

TEST_F(ChessPieceFixture, ToStringOperations)
{
    ChessPiece blackPawn(PieceSet::BLACK, PieceType::PAWN);
    EXPECT_EQ('p', blackPawn.toString());

    ChessPiece blackKnight(PieceSet::BLACK, PieceType::KNIGHT);
    EXPECT_EQ('n', blackKnight.toString());

    ChessPiece blackBishop(PieceSet::BLACK, PieceType::BISHOP);
    EXPECT_EQ('b', blackBishop.toString());

    ChessPiece blackRook(PieceSet::BLACK, PieceType::ROOK);
    EXPECT_EQ('r', blackRook.toString());

    ChessPiece blackQueen(PieceSet::BLACK, PieceType::QUEEN);
    EXPECT_EQ('q', blackQueen.toString());

    ChessPiece blackKing(PieceSet::BLACK, PieceType::KING);
    EXPECT_EQ('k', blackKing.toString());

    ChessPiece whitePawn(PieceSet::WHITE, PieceType::PAWN);
    EXPECT_EQ('P', whitePawn.toString());
    EXPECT_NE(blackPawn.toString(), whitePawn.toString());

    ChessPiece whiteKnight(PieceSet::WHITE, PieceType::KNIGHT);
    EXPECT_EQ('N', whiteKnight.toString());

    ChessPiece whiteBishop(PieceSet::WHITE, PieceType::BISHOP);
    EXPECT_EQ('B', whiteBishop.toString());

    ChessPiece whiteRook(PieceSet::WHITE, PieceType::ROOK);
    EXPECT_EQ('R', whiteRook.toString());

    ChessPiece whiteQueen(PieceSet::WHITE, PieceType::QUEEN);
    EXPECT_EQ('Q', whiteQueen.toString());

    ChessPiece whiteKing(PieceSet::WHITE, PieceType::KING);
    EXPECT_EQ('K', whiteKing.toString());
}

TEST_F(ChessPieceFixture, FromStringOperations)
{
    ChessPiece testPiece;
    EXPECT_FALSE(testPiece.fromString('a'));
    EXPECT_FALSE(testPiece.fromString('z'));

    ChessPiece blackPawn(PieceSet::BLACK, PieceType::PAWN);
    EXPECT_TRUE(testPiece.fromString('p'));
    EXPECT_EQ(blackPawn, testPiece);

    ChessPiece blackKnight(PieceSet::BLACK, PieceType::KNIGHT);
    EXPECT_TRUE(testPiece.fromString('n'));
    EXPECT_EQ(blackKnight, testPiece);

    ChessPiece blackBishop(PieceSet::BLACK, PieceType::BISHOP);
    EXPECT_TRUE(testPiece.fromString('b'));
    EXPECT_EQ(blackBishop, testPiece);

    ChessPiece blackRook(PieceSet::BLACK, PieceType::ROOK);
    EXPECT_TRUE(testPiece.fromString('r'));
    EXPECT_EQ(blackRook, testPiece);

    ChessPiece blackQueen(PieceSet::BLACK, PieceType::QUEEN);
    EXPECT_TRUE(testPiece.fromString('q'));
    EXPECT_EQ(blackQueen, testPiece);

    ChessPiece blackKing(PieceSet::BLACK, PieceType::KING);
    EXPECT_TRUE(testPiece.fromString('k'));
    EXPECT_EQ(blackKing, testPiece);

    ChessPiece whitePawn(PieceSet::WHITE, PieceType::PAWN);
    EXPECT_TRUE(testPiece.fromString('P'));
    EXPECT_EQ(whitePawn, testPiece);

    ChessPiece whiteKnight(PieceSet::WHITE, PieceType::KNIGHT);
    EXPECT_TRUE(testPiece.fromString('N'));
    EXPECT_EQ(whiteKnight, testPiece);

    ChessPiece whiteBishop(PieceSet::WHITE, PieceType::BISHOP);
    EXPECT_TRUE(testPiece.fromString('B'));
    EXPECT_EQ(whiteBishop, testPiece);

    ChessPiece whiteRook(PieceSet::WHITE, PieceType::ROOK);
    EXPECT_TRUE(testPiece.fromString('R'));
    EXPECT_EQ(whiteRook, testPiece);

    ChessPiece whiteQueen(PieceSet::WHITE, PieceType::QUEEN);
    EXPECT_TRUE(testPiece.fromString('Q'));
    EXPECT_EQ(whiteQueen, testPiece);

    ChessPiece whiteKing(PieceSet::WHITE, PieceType::KING);
    EXPECT_TRUE(testPiece.fromString('K'));
    EXPECT_EQ(whiteKing, testPiece);
}

TEST_F(ChessPieceFixture, FlipSet)
{
    ChessPiece whiteBishop(PieceSet::WHITE, PieceType::BISHOP);
    PieceSet expected = PieceSet::BLACK;
    PieceSet set = ChessPiece::FlipSet(whiteBishop.getSet());
    EXPECT_EQ(expected, set);
    
    expected = PieceSet::WHITE;
    set = ChessPiece::FlipSet(set);
    EXPECT_EQ(expected, set);

    byte exp = 0;
    byte res = (byte)PieceSet::BLACK;
    res = ChessPiece::FlipSet(res);
    EXPECT_EQ(exp, res);

    exp = 1;
    res = ChessPiece::FlipSet(res);
    EXPECT_EQ(exp, res);
}


////////////////////////////////////////////////////////////////

}