#include <gtest/gtest.h>
#include "chessboard.h"

namespace ElephantTest
{
////////////////////////////////////////////////////////////////
class ChessboardFixture : public ::testing::Test
{
public:
    virtual void SetUp()
    {

    };
    virtual void TearDown() {};

    Chessboard m_emptyChessboard; // by default a board should start empty.

};
////////////////////////////////////////////////////////////////
TEST_F(ChessboardFixture, Empty)
{
    ChessPiece expectedPiece; // empty, default, 0;

    EXPECT_EQ(expectedPiece, m_emptyChessboard.readTile({ 0, 0 }).getPiece());
    EXPECT_EQ(expectedPiece, m_emptyChessboard.readTile({ 7, 7 }).getPiece());
    EXPECT_EQ(expectedPiece, m_emptyChessboard.readTile({ 3, 3 }).getPiece());
    EXPECT_EQ(expectedPiece, m_emptyChessboard.readTile({ 4, 4 }).getPiece());
 
    EXPECT_EQ(expectedPiece, m_emptyChessboard.readTile(Notation::BuildPosition('a', 1)).getPiece());
    EXPECT_EQ(expectedPiece, m_emptyChessboard.readTile(Notation::BuildPosition('b', 2)).getPiece());
    EXPECT_EQ(expectedPiece, m_emptyChessboard.readTile(Notation::BuildPosition('e', 4)).getPiece());
    EXPECT_EQ(expectedPiece, m_emptyChessboard.readTile(Notation::BuildPosition('f', 4)).getPiece());
}

TEST_F(ChessboardFixture, Notation_BuildPosition)
{
    Notation expected(0, 0);
    auto pos = Notation::BuildPosition('a', 1);
    EXPECT_EQ(expected, pos);

    expected = Notation(0xf, 0xf);
    pos = Notation::BuildPosition('z', 1); // invalid position
    EXPECT_EQ(expected, pos);

    Notation defaultValue;
    EXPECT_EQ(expected, defaultValue);

    expected = Notation(4, 3);
    pos = Notation::BuildPosition('e', 4);
    EXPECT_EQ(expected, pos);
}

TEST_F(ChessboardFixture, Notation_GetIndex)
{
    byte expected = 0;
    auto pos = Notation::BuildPosition('a', 1);
    EXPECT_EQ(expected, pos.getIndex());

    expected = 135;
    pos = Notation::BuildPosition('z', 1); // invalid position
    EXPECT_EQ(expected, pos.getIndex());

    Notation defaultValue;
    EXPECT_EQ(expected, defaultValue.getIndex());

    expected = 28;
    pos = Notation::BuildPosition('e', 4);
    EXPECT_EQ(expected, pos.getIndex());

    expected = 11;
    pos = Notation::BuildPosition('d', 2);
    EXPECT_EQ(expected, pos.getIndex());

    expected = 55;
    pos = Notation::BuildPosition('h', 7);
    EXPECT_EQ(expected, pos.getIndex());

    expected = 56;
    pos = Notation::BuildPosition('a', 8);
    EXPECT_EQ(expected, pos.getIndex());

    expected = 1;
    pos = Notation::BuildPosition('b', 1);
    EXPECT_EQ(expected, pos.getIndex());
}

TEST_F(ChessboardFixture, ChessboardIterator_Iterrating)
{
    auto itr = m_emptyChessboard.begin();
    //const Chessboard constBoard;

    byte expected = 0;
    EXPECT_FALSE(itr.end());
    EXPECT_EQ(expected, itr.index());
    EXPECT_EQ(expected, itr.rank());
    EXPECT_EQ(expected, itr.file());

    itr++;
    byte otherExpected = 1;
    EXPECT_EQ(otherExpected, itr.index());
    EXPECT_EQ(expected, itr.rank());
    EXPECT_EQ(otherExpected, itr.file());
    
    itr++;
    otherExpected = 2;
    EXPECT_EQ(otherExpected, itr.index());
    EXPECT_EQ(expected, itr.rank());
    EXPECT_EQ(otherExpected, itr.file());

    auto otherItr = m_emptyChessboard.begin();
    for (int r = 0; r < 8; r++)
    {
        EXPECT_EQ(r, otherItr.rank());
        for (int f = 0; f < 8; f++)
        {
            auto notation = Notation(f, r);
            byte expectedIndex = notation.getIndex();
            EXPECT_EQ(expectedIndex, otherItr.index());
            EXPECT_EQ(f, otherItr.file());
            EXPECT_EQ(r, otherItr.rank());
            otherItr++;
        }
    }
}

TEST_F(ChessboardFixture, ChessboardIterator_Equality)
{
    // implement const vs non-const equality operator

    Chessboard b;
    const Chessboard cb;
    auto itrA = b.begin();
    auto itrD = b.begin();
    auto itrB = m_emptyChessboard.begin();
    auto itrC = cb.begin();

    EXPECT_NE(b.begin(), b.end());
    EXPECT_EQ(b.begin(), b.begin());
    EXPECT_EQ(b.end(), b.end());

    EXPECT_EQ(itrA, itrD);
    EXPECT_EQ(itrC, itrC);
    //EXPECT_EQ(itrC, itrA);
    EXPECT_EQ(itrA, itrA);
    EXPECT_NE(itrA, itrB);
    // EXPECT_NE(itrC, itrB);
    // arbitrery adds
    itrA++; itrA++; itrA++; itrA++;
    itrB++; itrB++; itrB++; itrB++;
    itrC++; itrC++; itrC++; itrC++;

    //EXPECT_EQ(itrC, itrA);
    EXPECT_EQ(itrA, itrA);
    EXPECT_NE(itrA, itrB);
    EXPECT_EQ(itrC, itrC);
    itrA++; itrA++; 
    itrB++; 
    itrC++; itrC++;

    //EXPECT_EQ(itrC, itrA);
    EXPECT_EQ(itrA, itrA);
    EXPECT_NE(itrA, itrB);
    EXPECT_EQ(itrC, itrC);
}

////////////////////////////////////////////////////////////////

} // namespace ElephantTest