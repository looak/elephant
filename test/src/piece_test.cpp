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

    blackPawn = 0x00;
    EXPECT_EQ(PieceType::NON, blackPawn.getType());
    EXPECT_EQ(PieceSet::WHITE, blackPawn.getSet());

    blackPawn = 0x86;
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
    EXPECT_EQ('p', ChessPiece::toString(blackPawn));

    ChessPiece blackKnight(PieceSet::BLACK, PieceType::KNIGHT);
    EXPECT_EQ('n', blackKnight.toString());
    EXPECT_EQ('n', ChessPiece::toString(blackKnight));

    ChessPiece blackBishop(PieceSet::BLACK, PieceType::BISHOP);
    EXPECT_EQ('b', blackBishop.toString());
    EXPECT_EQ('b', ChessPiece::toString(blackBishop));

    ChessPiece blackRook(PieceSet::BLACK, PieceType::ROOK);
    EXPECT_EQ('r', blackRook.toString());
    EXPECT_EQ('r', ChessPiece::toString(blackRook));

    ChessPiece blackQueen(PieceSet::BLACK, PieceType::QUEEN);
    EXPECT_EQ('q', blackQueen.toString());
    EXPECT_EQ('q', ChessPiece::toString(blackQueen));

    ChessPiece blackKing(PieceSet::BLACK, PieceType::KING);
    EXPECT_EQ('k', blackKing.toString());
    EXPECT_EQ('k', ChessPiece::toString(blackKing));

    ChessPiece whitePawn(PieceSet::WHITE, PieceType::PAWN);
    EXPECT_EQ('P', whitePawn.toString());
    EXPECT_EQ('P', ChessPiece::toString(whitePawn));
    EXPECT_NE(blackPawn.toString(), whitePawn.toString());

    ChessPiece whiteKnight(PieceSet::WHITE, PieceType::KNIGHT);
    EXPECT_EQ('N', whiteKnight.toString());
    EXPECT_EQ('N', ChessPiece::toString(whiteKnight));

    ChessPiece whiteBishop(PieceSet::WHITE, PieceType::BISHOP);
    EXPECT_EQ('B', whiteBishop.toString());
    EXPECT_EQ('B', ChessPiece::toString(whiteBishop));

    ChessPiece whiteRook(PieceSet::WHITE, PieceType::ROOK);
    EXPECT_EQ('R', whiteRook.toString());
    EXPECT_EQ('R', ChessPiece::toString(whiteRook));

    ChessPiece whiteQueen(PieceSet::WHITE, PieceType::QUEEN);
    EXPECT_EQ('Q', whiteQueen.toString());
    EXPECT_EQ('Q', ChessPiece::toString(whiteQueen));

    ChessPiece whiteKing(PieceSet::WHITE, PieceType::KING);
    EXPECT_EQ('K', whiteKing.toString());
    EXPECT_EQ('K', ChessPiece::toString(whiteKing));
}

////////////////////////////////////////////////////////////////

}