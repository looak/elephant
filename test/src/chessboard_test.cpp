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

    EXPECT_EQ(expectedPiece, m_emptyChessboard.readTile({ 0, 0 }).readPiece());
    EXPECT_EQ(expectedPiece, m_emptyChessboard.readTile({ 7, 7 }).readPiece());
    EXPECT_EQ(expectedPiece, m_emptyChessboard.readTile({ 3, 3 }).readPiece());
    EXPECT_EQ(expectedPiece, m_emptyChessboard.readTile({ 4, 4 }).readPiece());
 
    EXPECT_EQ(expectedPiece, m_emptyChessboard.readTile(Notation::BuildPosition('a', 1)).readPiece());
    EXPECT_EQ(expectedPiece, m_emptyChessboard.readTile(Notation::BuildPosition('b', 2)).readPiece());
    EXPECT_EQ(expectedPiece, m_emptyChessboard.readTile(Notation::BuildPosition('e', 4)).readPiece());
    EXPECT_EQ(expectedPiece, m_emptyChessboard.readTile(Notation::BuildPosition('f', 4)).readPiece());

    EXPECT_EQ(0, m_emptyChessboard.readCastlingState());
}

TEST_F(ChessboardFixture, Notation_Equality)
{
    Notation expected(0, 0);
    EXPECT_EQ(expected, expected);

    expected = Notation(0xf, 0xf);
    auto pos = Notation::BuildPosition('z', 1); // invalid position
    EXPECT_EQ(expected, pos);

    Notation defaultValue;
    expected = Notation(0xf, 0xf);
    pos = Notation(0);
    EXPECT_EQ(expected, defaultValue);
    EXPECT_NE(expected, pos);
    EXPECT_NE(pos, defaultValue);

    expected = Notation(28);
    pos = Notation::BuildPosition('e', 4);
    EXPECT_EQ(expected, pos);
    EXPECT_NE(defaultValue, expected);
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
    EXPECT_EQ(expected, pos.index());

    expected = 135;
    pos = Notation::BuildPosition('z', 1); // invalid position
    EXPECT_EQ(expected, pos.index());

    Notation defaultValue;
    EXPECT_EQ(expected, defaultValue.index());

    expected = 28;
    pos = Notation::BuildPosition('e', 4);
    EXPECT_EQ(expected, pos.index());

    expected = 11;
    pos = Notation::BuildPosition('d', 2);
    EXPECT_EQ(expected, pos.index());

    expected = 55;
    pos = Notation::BuildPosition('h', 7);
    EXPECT_EQ(expected, pos.index());

    expected = 56;
    pos = Notation::BuildPosition('a', 8);
    EXPECT_EQ(expected, pos.index());

    expected = 1;
    pos = Notation::BuildPosition('b', 1);
    EXPECT_EQ(expected, pos.index());
}

TEST_F(ChessboardFixture, Notation_IndexCtor)
{
    auto expected = Notation(0, 0);
    auto pos = Notation(0);
    EXPECT_EQ(expected, pos);

    expected = Notation(4, 3);
    pos = Notation(28);
    EXPECT_EQ(expected, pos);

    expected = Notation(3, 1);
    pos = Notation(11);
    EXPECT_EQ(expected, pos);

    expected = Notation(7, 6);
    pos = Notation(55);
    EXPECT_EQ(expected, pos);

    expected = Notation(0, 7);
    pos = Notation(56);
    EXPECT_EQ(expected, pos);

    for (int r = 0; r < 8; r++)
    {
        for (int f = 0; f < 8; f++)
        {
            expected = Notation(f, r);
            pos = Notation(expected.index());
            EXPECT_EQ(expected, pos);
        }
    }
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
            byte expectedIndex = notation.index();
            EXPECT_EQ(expectedIndex, otherItr.index());
            EXPECT_EQ(f, otherItr.file());
            EXPECT_EQ(r, otherItr.rank());
            otherItr++;
        }
    }
}

TEST_F(ChessboardFixture, ChessboardIterator_IterratingExt)
{
    auto itr = m_emptyChessboard.begin();
    //const Chessboard constBoard;

    byte expectedIndex = 0;
    byte expectedRank = 0;
    byte expectedFile = 0;
    EXPECT_FALSE(itr.end());
    EXPECT_EQ(expectedIndex, itr.index());
    EXPECT_EQ(expectedRank, itr.rank());
    EXPECT_EQ(expectedFile, itr.file());

    itr += 2;
    expectedIndex = 2;
    expectedRank = 0;
    expectedFile = 2;
    EXPECT_EQ(expectedIndex, itr.index());
    EXPECT_EQ(expectedRank, itr.rank());
    EXPECT_EQ(expectedFile, itr.file());

    itr += 8;
    expectedIndex = 10;
    expectedRank = 1;
    expectedFile = 2;
    EXPECT_EQ(expectedIndex, itr.index());
    EXPECT_EQ(expectedRank, itr.rank());
    EXPECT_EQ(expectedFile, itr.file());

    itr += 16;
    expectedIndex = 26;
    expectedRank = 3;
    expectedFile = 2;
    EXPECT_EQ(expectedIndex, itr.index());
    EXPECT_EQ(expectedRank, itr.rank());
    EXPECT_EQ(expectedFile, itr.file());

    itr += 7;
    expectedIndex = 33;
    expectedRank = 4;
    expectedFile = 1;
    EXPECT_EQ(expectedIndex, itr.index());
    EXPECT_EQ(expectedRank, itr.rank());
    EXPECT_EQ(expectedFile, itr.file());
    
    itr += 7;
    expectedIndex = 40;
    expectedRank = 5;
    expectedFile = 0;
    EXPECT_EQ(expectedIndex, itr.index());
    EXPECT_EQ(expectedRank, itr.rank());
    EXPECT_EQ(expectedFile, itr.file());

    itr += 7;
    expectedIndex = 47;
    expectedRank = 5;
    expectedFile = 7;
    EXPECT_EQ(expectedIndex, itr.index());
    EXPECT_EQ(expectedRank, itr.rank());
    EXPECT_EQ(expectedFile, itr.file());

    itr += 7;
    expectedIndex = 54;
    expectedRank = 6;
    expectedFile = 6;
    EXPECT_EQ(expectedIndex, itr.index());
    EXPECT_EQ(expectedRank, itr.rank());
    EXPECT_EQ(expectedFile, itr.file());

    itr += 7;
    expectedIndex = 61;
    expectedRank = 7;
    expectedFile = 5;
    EXPECT_EQ(expectedIndex, itr.index());
    EXPECT_EQ(expectedRank, itr.rank());
    EXPECT_EQ(expectedFile, itr.file());

    itr += 7;
    expectedIndex = 64;
    expectedRank = 8;
    expectedFile = 0;
    EXPECT_EQ(expectedIndex, itr.index());
    EXPECT_EQ(expectedRank, itr.rank());
    EXPECT_EQ(expectedFile, itr.file());

    auto scnditr = m_emptyChessboard.begin();    

    scnditr += 11;
    expectedIndex = 11;
    expectedRank = 1;
    expectedFile = 3;
    EXPECT_EQ(expectedIndex, scnditr.index());
    EXPECT_EQ(expectedRank, scnditr.rank());
    EXPECT_EQ(expectedFile, scnditr.file());

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