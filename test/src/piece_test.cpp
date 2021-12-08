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
    ChessPiece whitePawn(WHITE, PAWN);
    EXPECT_EQ(PieceType::PAWN, whitePawn.getType());
    EXPECT_EQ(PieceSet::WHITE, whitePawn.getSet());

    ChessPiece blackPawn(BLACK, PAWN);
    EXPECT_EQ(PieceType::PAWN, blackPawn.getType());
    EXPECT_EQ(PieceSet::BLACK, blackPawn.getSet());

    ChessPiece whiteKnight(WHITE, KNIGHT);
    EXPECT_EQ(PieceType::KNIGHT, whiteKnight.getType());
    EXPECT_EQ(PieceSet::WHITE, whiteKnight.getSet());

    ChessPiece blackKnight(BLACK, KNIGHT);
    EXPECT_EQ(PieceType::KNIGHT, blackKnight.getType());
    EXPECT_EQ(PieceSet::BLACK, blackKnight.getSet());

    ChessPiece whiteBishop(WHITE, BISHOP);
    EXPECT_EQ(PieceType::BISHOP, whiteBishop.getType());
    EXPECT_EQ(PieceSet::WHITE, whiteBishop.getSet());

    ChessPiece blackBishop(BLACK, BISHOP);
    EXPECT_EQ(PieceType::BISHOP, blackBishop.getType());
    EXPECT_EQ(PieceSet::BLACK, blackBishop.getSet());

    ChessPiece whiteRook(WHITE, ROOK);
    EXPECT_EQ(PieceType::ROOK, whiteRook.getType());
    EXPECT_EQ(PieceSet::WHITE, whiteRook.getSet());

    ChessPiece blackRook(BLACK, ROOK);
    EXPECT_EQ(PieceType::ROOK, blackRook.getType());
    EXPECT_EQ(PieceSet::BLACK, blackRook.getSet());

    ChessPiece whiteQueen(WHITE, QUEEN);
    EXPECT_EQ(PieceType::QUEEN, whiteQueen.getType());
    EXPECT_EQ(PieceSet::WHITE, whiteQueen.getSet());

    ChessPiece blackQueen(BLACK, QUEEN);
    EXPECT_EQ(PieceType::QUEEN, blackQueen.getType());
    EXPECT_EQ(PieceSet::BLACK, blackQueen.getSet());

    ChessPiece whiteKing(WHITE, KING);
    EXPECT_EQ(PieceType::KING, whiteKing.getType());
    EXPECT_EQ(PieceSet::WHITE, whiteKing.getSet());

    ChessPiece blackKing(BLACK, KING);
    EXPECT_EQ(PieceType::KING, blackKing.getType());
    EXPECT_EQ(PieceSet::BLACK, blackKing.getSet());
}

////////////////////////////////////////////////////////////////

}