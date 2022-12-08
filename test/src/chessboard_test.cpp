#include <gtest/gtest.h>
#include "chessboard.h"
#include "elephant_test_utils.h"
#include "hash_zorbist.h"

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

TEST_F(ChessboardFixture, KingCheckedByOpRook)
{
    Chessboard b;
    b.PlacePiece(WHITEKING, e1);
    b.PlacePiece(BLACKROOK, e8);

    bool result = b.Checked(Set::WHITE);
    EXPECT_TRUE(result);
}

TEST_F(ChessboardFixture, KingNotCheckedByOpRook)
{
    Chessboard b;
    b.PlacePiece(BLACKKING, e8);
    b.PlacePiece(WHITEROOK, f1);

    bool result = b.Checked(Set::BLACK);
    EXPECT_FALSE(result);
}

TEST_F(ChessboardFixture, KingsNotChecked)
{
    Chessboard b;
    b.PlacePiece(BLACKKING, e8);
    b.PlacePiece(WHITEKING, e1);

    bool result = b.Checked(Set::BLACK);
    EXPECT_FALSE(result);

    result = b.Checked(Set::WHITE);
    EXPECT_FALSE(result);
}

TEST_F(ChessboardFixture, Black_StartingPosition_Threatened)
{
    Chessboard board;
    auto k = BLACKKING;
    auto q = BLACKQUEEN;
    auto b = BLACKBISHOP;
    auto n = BLACKKNIGHT;
    auto r = BLACKROOK;
    auto p = BLACKPAWN;

    board.editCastlingState() = 15;
    // setup
    board.PlacePiece(r, a8);
    board.PlacePiece(n, b8);
    board.PlacePiece(b, c8);
    board.PlacePiece(q, d8);
    board.PlacePiece(k, e8);
    board.PlacePiece(b, f8);
    board.PlacePiece(n, g8);
    board.PlacePiece(r, h8);
    
    board.PlacePiece(p, a7);
    board.PlacePiece(p, b7);
    board.PlacePiece(p, c7);
    board.PlacePiece(p, d7);
    board.PlacePiece(p, e7);
    board.PlacePiece(p, f7);
    board.PlacePiece(p, g7);
    board.PlacePiece(p, h7);    

    u64 expected = ~universe;
    expected |= INT64_C(1) << b8.index();
    expected |= INT64_C(1) << c8.index();
    expected |= INT64_C(1) << d8.index();
    expected |= INT64_C(1) << e8.index();
    expected |= INT64_C(1) << f8.index();
    expected |= INT64_C(1) << g8.index();

    expected |= INT64_C(1) << a7.index();
    expected |= INT64_C(1) << b7.index();
    expected |= INT64_C(1) << c7.index();
    expected |= INT64_C(1) << d7.index();
    expected |= INT64_C(1) << e7.index();
    expected |= INT64_C(1) << f7.index();
    expected |= INT64_C(1) << g7.index();
    expected |= INT64_C(1) << h7.index();
    
    expected |= INT64_C(1) << a6.index();
    expected |= INT64_C(1) << b6.index();
    expected |= INT64_C(1) << c6.index();
    expected |= INT64_C(1) << d6.index();
    expected |= INT64_C(1) << e6.index();
    expected |= INT64_C(1) << f6.index();
    expected |= INT64_C(1) << g6.index();
    expected |= INT64_C(1) << h6.index();

    // do
    u64 threat = board.GetThreatenedMask(Set::BLACK);

    // validate
    EXPECT_EQ(expected, threat);
}

////////////////////////////////////////////////////////////////

TEST_F(ChessboardFixture, ZorbistHashing)
{ 
    // board should start out empty, so hashing these two boards should result in the same value.
    Chessboard boardOne;
    Chessboard boardTwo;
    
    ZorbistHash hash;

    u64 oneHash = hash.HashBoard(boardOne);
    u64 twoHash = hash.HashBoard(boardTwo);

    EXPECT_EQ(oneHash, twoHash);
        
    auto k = BLACKKING;
    auto q = BLACKQUEEN;
    auto b = BLACKBISHOP;
    auto n = BLACKKNIGHT;
    auto r = BLACKROOK;
    auto p = BLACKPAWN;

    // board one
    boardOne.editCastlingState() = 15;
    
    boardOne.PlacePiece(r, a8);
    boardOne.PlacePiece(n, b8);
    boardOne.PlacePiece(b, c8);
    boardOne.PlacePiece(q, d8);
    boardOne.PlacePiece(k, e8);
    boardOne.PlacePiece(b, f8);
    boardOne.PlacePiece(n, g8);
    

    boardOne.PlacePiece(p, a7);
    boardOne.PlacePiece(p, b7);
    boardOne.PlacePiece(p, c7);
    boardOne.PlacePiece(p, d7);
    boardOne.PlacePiece(p, e7);
    boardOne.PlacePiece(p, f7);
    boardOne.PlacePiece(p, g7);
    boardOne.PlacePiece(p, h7);

    // board Two
    boardTwo.editCastlingState() = 15;

    boardTwo.PlacePiece(r, a8);
    boardTwo.PlacePiece(n, b8);
    boardTwo.PlacePiece(b, c8);
    boardTwo.PlacePiece(q, d8);
    boardTwo.PlacePiece(k, e8);
    boardTwo.PlacePiece(b, f8);
    boardTwo.PlacePiece(n, g8);
    boardTwo.PlacePiece(r, h8);

    boardTwo.PlacePiece(p, a7);
    boardTwo.PlacePiece(p, b7);
    boardTwo.PlacePiece(p, c7);
    boardTwo.PlacePiece(p, d7);
    boardTwo.PlacePiece(p, e7);
    boardTwo.PlacePiece(p, f7);
    boardTwo.PlacePiece(p, g7);
    boardTwo.PlacePiece(p, h7);

    oneHash = hash.HashBoard(boardOne);
    twoHash = hash.HashBoard(boardTwo);

    EXPECT_NE(oneHash, twoHash);

    boardOne.PlacePiece(r, h8);

    oneHash = hash.HashBoard(boardOne);
    twoHash = hash.HashBoard(boardTwo);

    EXPECT_EQ(oneHash, twoHash);

}

////////////////////////////////////////////////////////////////

TEST_F(ChessboardFixture, Constructor_Copy)
{
    Chessboard board;
}

////////////////////////////////////////////////////////////////

} // namespace ElephantTest