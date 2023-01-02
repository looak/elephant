#include <gtest/gtest.h>
#include <algorithm>
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
        DefaultStartingPosition();
        GameOfTheCentury_WindmillPosition();
    };
    virtual void TearDown() {};

    
    Chessboard m_emptyChessboard; // by default a board should start empty.
    Chessboard m_defaultStartingPosition;
    Chessboard m_gameOfTheCentury;

private:
    // 8 [ r ][ n ][ b ][ q ][ k ][ b ][ n ][ r ]
    // 7 [ p ][ p ][ p ][ p ][ p ][ p ][ p ][ p ]
    // 6 [   ][   ][   ][   ][   ][   ][   ][   ]
    // 5 [   ][   ][   ][   ][   ][   ][   ][   ]
    // 4 [   ][   ][   ][   ][   ][   ][   ][   ]
    // 3 [   ][   ][   ][   ][   ][   ][   ][   ]
    // 2 [ P ][ P ][ P ][ P ][ P ][ P ][ P ][ P ]
    // 1 [ R ][ N ][ B ][ Q ][ K ][ B ][ N ][ R ]
    //     A    B    C    D    E    F    G    H
    // fen: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
    void DefaultStartingPosition()
    {
        auto K = WHITEKING;
        auto Q = WHITEQUEEN;
        auto B = WHITEBISHOP;
        auto N = WHITEKNIGHT;
        auto R = WHITEROOK;
        auto P = WHITEPAWN;

        auto k = BLACKKING;
        auto q = BLACKQUEEN;
        auto b = BLACKBISHOP;
        auto n = BLACKKNIGHT;
        auto r = BLACKROOK;
        auto p = BLACKPAWN;

        m_defaultStartingPosition.PlacePiece(R, a1);
        m_defaultStartingPosition.PlacePiece(N, b1);
        m_defaultStartingPosition.PlacePiece(B, c1);
        m_defaultStartingPosition.PlacePiece(Q, d1);
        m_defaultStartingPosition.PlacePiece(K, e1);
        m_defaultStartingPosition.PlacePiece(B, f1);
        m_defaultStartingPosition.PlacePiece(N, g1);
        m_defaultStartingPosition.PlacePiece(R, h1);

        m_defaultStartingPosition.PlacePiece(P, a2);
        m_defaultStartingPosition.PlacePiece(P, b2);
        m_defaultStartingPosition.PlacePiece(P, c2);
        m_defaultStartingPosition.PlacePiece(P, d2);
        m_defaultStartingPosition.PlacePiece(P, e2);
        m_defaultStartingPosition.PlacePiece(P, f2);
        m_defaultStartingPosition.PlacePiece(P, g2);
        m_defaultStartingPosition.PlacePiece(P, h2);

        m_defaultStartingPosition.PlacePiece(r, a8);
        m_defaultStartingPosition.PlacePiece(n, b8);
        m_defaultStartingPosition.PlacePiece(b, c8);
        m_defaultStartingPosition.PlacePiece(q, d8);
        m_defaultStartingPosition.PlacePiece(k, e8);
        m_defaultStartingPosition.PlacePiece(b, f8);
        m_defaultStartingPosition.PlacePiece(n, g8);
        m_defaultStartingPosition.PlacePiece(r, h8);

        m_defaultStartingPosition.PlacePiece(p, a7);
        m_defaultStartingPosition.PlacePiece(p, b7);
        m_defaultStartingPosition.PlacePiece(p, c7);
        m_defaultStartingPosition.PlacePiece(p, d7);
        m_defaultStartingPosition.PlacePiece(p, e7);
        m_defaultStartingPosition.PlacePiece(p, f7);
        m_defaultStartingPosition.PlacePiece(p, g7);
        m_defaultStartingPosition.PlacePiece(p, h7);

        m_defaultStartingPosition.setCastlingState(15);
    }

    // https://en.wikipedia.org/wiki/The_Game_of_the_Century_(chess)
    // "Windmill Position from s.k. game of the century between Donald Bryen as White & Bobby Fischer as Black.
    // Played at the Marshall Chess Club in New York City on October 17th 1956.
    // At move 17 Fisher begins a windmill and that is where this board is.
    
    // 8 [ r ][   ][   ][   ][ r ][ n ][ k ][   ]
    // 7 [ p ][ b ][   ][   ][   ][ p ][ p ][   ]
    // 6 [   ][   ][   ][ p ][ p ][   ][   ][ p ]
    // 5 [   ][ q ][   ][   ][   ][   ][ B ][ Q ]
    // 4 [   ][ P ][   ][ P ][   ][   ][   ][   ]
    // 3 [   ][   ][   ][   ][ N ][   ][ R ][   ]
    // 2 [ P ][   ][   ][   ][   ][ P ][ P ][ P ]
    // 1 [ R ][   ][   ][   ][ R ][   ][ K ][   ]
    //     A    B    C    D    E    F    G    H
    // fen: r3rnk1/pb3pp1/3pp2p/1q4BQ/1P1P4/4N1R1/P4PPP/4R1K1 b - - 18 1
    void GameOfTheCentury_WindmillPosition() 
    {
        auto K = WHITEKING;
        auto Q = WHITEQUEEN;
        auto B = WHITEBISHOP;
        auto N = WHITEKNIGHT;
        auto R = WHITEROOK;
        auto P = WHITEPAWN;

        auto k = BLACKKING;
        auto q = BLACKQUEEN;
        auto b = BLACKBISHOP;
        auto n = BLACKKNIGHT;
        auto r = BLACKROOK;
        auto p = BLACKPAWN;

        m_gameOfTheCentury.PlacePiece(r, a8);
        m_gameOfTheCentury.PlacePiece(r, e8);
        m_gameOfTheCentury.PlacePiece(n, f8);
        m_gameOfTheCentury.PlacePiece(k, g8);

        m_gameOfTheCentury.PlacePiece(p, a7);
        m_gameOfTheCentury.PlacePiece(b, b7);
        m_gameOfTheCentury.PlacePiece(p, f7);
        m_gameOfTheCentury.PlacePiece(p, g7);

        m_gameOfTheCentury.PlacePiece(p, d6);
        m_gameOfTheCentury.PlacePiece(p, e6);
        m_gameOfTheCentury.PlacePiece(p, h6);

        m_gameOfTheCentury.PlacePiece(q, b5);
        m_gameOfTheCentury.PlacePiece(B, g5);
        m_gameOfTheCentury.PlacePiece(Q, h5);

        m_gameOfTheCentury.PlacePiece(P, b4);
        m_gameOfTheCentury.PlacePiece(P, d4);

        m_gameOfTheCentury.PlacePiece(N, e3);
        m_gameOfTheCentury.PlacePiece(R, g3);

        m_gameOfTheCentury.PlacePiece(P, a2);
        m_gameOfTheCentury.PlacePiece(P, f2);
        m_gameOfTheCentury.PlacePiece(P, g2);
        m_gameOfTheCentury.PlacePiece(P, h2);

        m_gameOfTheCentury.PlacePiece(R, e1);
        m_gameOfTheCentury.PlacePiece(K, g1);
    }

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

    // checking all tiles that they are empty
    for (int i = 0; i < 64; i++)
    {
        auto piece = m_emptyChessboard.readPieceAt(i);
        EXPECT_EQ(expectedPiece, piece);        
    }

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

    auto [checked, count] = b.IsInCheck(Set::WHITE);
    EXPECT_TRUE(checked);
    EXPECT_EQ(1, count);
}

TEST_F(ChessboardFixture, KingNotCheckedByOpRook)
{
    Chessboard b;
    b.PlacePiece(BLACKKING, e8);
    b.PlacePiece(WHITEROOK, f1);

    auto [checked, count] = b.IsInCheck(Set::WHITE);
    EXPECT_FALSE(checked);
    EXPECT_EQ(0, count);
}

TEST_F(ChessboardFixture, KingsNotChecked)
{
    Chessboard b;
    b.PlacePiece(BLACKKING, e8);
    b.PlacePiece(WHITEKING, e1);

    auto result = b.IsInCheck(Set::WHITE);
    EXPECT_FALSE(std::get<0>(result));
    EXPECT_EQ(0, std::get<1>(result));

    result = b.IsInCheck(Set::WHITE);
    EXPECT_FALSE(std::get<0>(result));
    EXPECT_EQ(0, std::get<1>(result));
}

// 8 [   ][   ][   ][   ][ k ][   ][ r ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][ b ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][   ][   ][ K ][   ]
//     A    B    C    D    E    F    G    H
TEST_F(ChessboardFixture, MultipleChecks_BishopAndRook)
{
    Chessboard board;
    auto b = BLACKBISHOP;
    auto r = BLACKROOK;
    auto k = BLACKKING;
    auto K = WHITEKING;

    board.PlacePiece(b, e3);
    board.PlacePiece(r, g8);
    board.PlacePiece(k, e8);
    board.PlacePiece(K, g1);

    auto result = board.IsInCheck(Set::WHITE);
    EXPECT_TRUE(std::get<0>(result));
    EXPECT_EQ(2, std::get<1>(result));

    result = board.IsInCheck(Set::BLACK);
    EXPECT_FALSE(std::get<0>(result));
    EXPECT_EQ(0, std::get<1>(result));

    EXPECT_FALSE(board.IsInCheckmate(Set::BLACK));
    EXPECT_FALSE(board.IsInCheckmate(Set::WHITE));
    EXPECT_FALSE(board.IsInStalemate(Set::BLACK));
    EXPECT_FALSE(board.IsInStalemate(Set::WHITE));
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

    board.setCastlingState(15);
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
    
    u64 oneHash = ZorbistHash::Instance().HashBoard(boardOne);
    u64 twoHash = ZorbistHash::Instance().HashBoard(boardTwo);

    EXPECT_EQ(oneHash, twoHash);
        
    auto k = BLACKKING;
    auto q = BLACKQUEEN;
    auto b = BLACKBISHOP;
    auto n = BLACKKNIGHT;
    auto r = BLACKROOK;
    auto p = BLACKPAWN;
    auto R = WHITEROOK;

    // board one
    boardOne.PlacePiece(r, a8);
    boardTwo.PlacePiece(R, a8);

    oneHash = ZorbistHash::Instance().HashBoard(boardOne);
    twoHash = ZorbistHash::Instance().HashBoard(boardTwo);

    EXPECT_NE(oneHash, twoHash);
    EXPECT_EQ(boardTwo.readHash(), twoHash);
    EXPECT_EQ(boardOne.readHash(), oneHash);
    EXPECT_NE(boardOne.readHash(), boardTwo.readHash());

    // board one    
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
    boardTwo.PlacePiece(r, a8, true);
    boardTwo.PlacePiece(n, b8);
    boardTwo.PlacePiece(b, c8);
    boardTwo.PlacePiece(q, d8);
    boardTwo.PlacePiece(k, e8);
    boardTwo.PlacePiece(b, f8);
    boardTwo.PlacePiece(n, g8);
    boardTwo.PlacePiece(r, h8);

    boardTwo.PlacePiece(p, h7);
    boardTwo.PlacePiece(p, g7);
    boardTwo.PlacePiece(p, f7);
    boardTwo.PlacePiece(p, e7);
    boardTwo.PlacePiece(p, d7);
    boardTwo.PlacePiece(p, c7);
    boardTwo.PlacePiece(p, b7);
    boardTwo.PlacePiece(p, a7);

    oneHash = ZorbistHash::Instance().HashBoard(boardOne);
    twoHash = ZorbistHash::Instance().HashBoard(boardTwo);

    EXPECT_NE(oneHash, twoHash);
    EXPECT_EQ(boardTwo.readHash(), twoHash);
    EXPECT_EQ(boardOne.readHash(), oneHash);
    EXPECT_NE(boardOne.readHash(), boardTwo.readHash());

    boardOne.PlacePiece(r, h8);

    oneHash = ZorbistHash::Instance().HashBoard(boardOne);
    twoHash = ZorbistHash::Instance().HashBoard(boardTwo);

    EXPECT_EQ(oneHash, twoHash);
    EXPECT_EQ(boardTwo.readHash(), twoHash);
    EXPECT_EQ(boardOne.readHash(), oneHash);
    EXPECT_EQ(boardOne.readHash(), boardTwo.readHash());

    boardTwo.setCastlingState(12);

    oneHash = ZorbistHash::Instance().HashBoard(boardOne);
    twoHash = ZorbistHash::Instance().HashBoard(boardTwo);

    EXPECT_NE(oneHash, twoHash);
    EXPECT_EQ(boardTwo.readHash(), twoHash);
    EXPECT_EQ(boardOne.readHash(), oneHash);
    EXPECT_NE(boardOne.readHash(), boardTwo.readHash());

    boardOne.setCastlingState(12);

    oneHash = ZorbistHash::Instance().HashBoard(boardOne);
    twoHash = ZorbistHash::Instance().HashBoard(boardTwo);

    EXPECT_EQ(oneHash, twoHash);
    EXPECT_EQ(boardTwo.readHash(), twoHash);
    EXPECT_EQ(boardOne.readHash(), oneHash);
    EXPECT_EQ(boardOne.readHash(), boardTwo.readHash());

    boardOne.setEnPassant(c7);

    oneHash = ZorbistHash::Instance().HashBoard(boardOne);
    twoHash = ZorbistHash::Instance().HashBoard(boardTwo);

    EXPECT_NE(oneHash, twoHash);
    EXPECT_EQ(boardTwo.readHash(), twoHash);
    EXPECT_EQ(boardOne.readHash(), oneHash);
    EXPECT_NE(boardOne.readHash(), boardTwo.readHash());
    
    boardTwo.setEnPassant(c7);

    oneHash = ZorbistHash::Instance().HashBoard(boardOne);
    twoHash = ZorbistHash::Instance().HashBoard(boardTwo);

    EXPECT_EQ(oneHash, twoHash);
    EXPECT_EQ(boardTwo.readHash(), twoHash);
    EXPECT_EQ(boardOne.readHash(), oneHash);
    EXPECT_EQ(boardOne.readHash(), boardTwo.readHash());

    boardOne.setEnPassant(e4);

    oneHash = ZorbistHash::Instance().HashBoard(boardOne);    

    EXPECT_NE(oneHash, twoHash);
    EXPECT_EQ(boardOne.readHash(), oneHash);
    EXPECT_NE(boardOne.readHash(), boardTwo.readHash());
}

////////////////////////////////////////////////////////////////

TEST_F(ChessboardFixture, Constructor_Copy)
{
    Chessboard copy(m_defaultStartingPosition);

    u64 copyBoardHash = ZorbistHash::Instance().HashBoard(copy);
    u64 defaultBoardHash = ZorbistHash::Instance().HashBoard(m_defaultStartingPosition);

    EXPECT_EQ(copyBoardHash, defaultBoardHash);
    EXPECT_EQ(copyBoardHash, copy.readHash());
    EXPECT_EQ(defaultBoardHash, m_defaultStartingPosition.readHash());

    Chessboard scndCopy(m_gameOfTheCentury);
    u64 scndCopyHash = ZorbistHash::Instance().HashBoard(scndCopy);
    u64 goatGame = ZorbistHash::Instance().HashBoard(m_gameOfTheCentury);

    EXPECT_EQ(goatGame, m_gameOfTheCentury.readHash());
    EXPECT_EQ(scndCopyHash, scndCopy.readHash());
    EXPECT_EQ(goatGame, scndCopyHash);

    u64 orgMask = m_gameOfTheCentury.GetKingMask(Set::BLACK);
    u64 cpyMask = scndCopy.GetKingMask(Set::BLACK);
    EXPECT_EQ(orgMask, cpyMask);

    orgMask = m_gameOfTheCentury.GetKingMask(Set::WHITE);
    cpyMask = scndCopy.GetKingMask(Set::WHITE);
    EXPECT_EQ(orgMask, cpyMask);
    
    orgMask = m_gameOfTheCentury.GetSlidingMask(Set::BLACK);
    cpyMask = scndCopy.GetSlidingMask(Set::BLACK);
    EXPECT_GT(orgMask, 0);
    EXPECT_EQ(orgMask, cpyMask);

    orgMask = m_gameOfTheCentury.GetSlidingMask(Set::WHITE);
    cpyMask = scndCopy.GetSlidingMask(Set::WHITE);
    EXPECT_EQ(orgMask, cpyMask);

    orgMask = m_gameOfTheCentury.GetThreatenedMask(Set::BLACK);
    cpyMask = scndCopy.GetThreatenedMask(Set::BLACK);
    EXPECT_EQ(orgMask, cpyMask);

    orgMask = m_gameOfTheCentury.GetThreatenedMask(Set::WHITE);
    cpyMask = scndCopy.GetThreatenedMask(Set::WHITE);
    EXPECT_EQ(orgMask, cpyMask);

    const Material* orgMat = &m_gameOfTheCentury.readMaterial(Set::BLACK);
	const Material* cpyMat = &scndCopy.readMaterial(Set::BLACK);
	EXPECT_EQ(orgMat->getValue(), cpyMat->getValue());
	EXPECT_EQ(orgMat->getCount(), cpyMat->getCount());

	orgMat = &m_gameOfTheCentury.readMaterial(Set::WHITE);
    cpyMat = &scndCopy.readMaterial(Set::WHITE);
    EXPECT_EQ(orgMat->getValue(), cpyMat->getValue());
    EXPECT_EQ(orgMat->getCount(), cpyMat->getCount());
}

////////////////////////////////////////////////////////////////

TEST_F(ChessboardFixture, ValidateMaterial)
{
    const auto* orgMat = &m_defaultStartingPosition.readMaterial(Set::WHITE);

    EXPECT_EQ(8u, orgMat->getPieceCount(WHITEPAWN));
    EXPECT_EQ(2u, orgMat->getPieceCount(WHITEKNIGHT));
    EXPECT_EQ(2u, orgMat->getPieceCount(WHITEBISHOP));
    EXPECT_EQ(2u, orgMat->getPieceCount(WHITEROOK));
    EXPECT_EQ(1u, orgMat->getPieceCount(WHITEQUEEN));
    EXPECT_EQ(1u, orgMat->getPieceCount(WHITEKING));
    
	EXPECT_TRUE(VerifyListsContainSameNotations({ a2, b2, c2, d2, e2, f2, g2, h2 }, orgMat->getPlacementsOfPiece(WHITEPAWN)));
	EXPECT_TRUE(VerifyListsContainSameNotations({ a1, h1 }, orgMat->getPlacementsOfPiece(WHITEROOK)));
	EXPECT_TRUE(VerifyListsContainSameNotations({ b1, g1 }, orgMat->getPlacementsOfPiece(WHITEKNIGHT)));
	EXPECT_TRUE(VerifyListsContainSameNotations({ c1, f1 }, orgMat->getPlacementsOfPiece(WHITEBISHOP)));

    EXPECT_EQ(d1, orgMat->getPlacementsOfPiece(WHITEQUEEN)[0]);
    EXPECT_EQ(e1, orgMat->getPlacementsOfPiece(WHITEKING)[0]);

    orgMat = &m_defaultStartingPosition.readMaterial(Set::BLACK);

	EXPECT_EQ(8u, orgMat->getPieceCount(BLACKPAWN));
	EXPECT_EQ(2u, orgMat->getPieceCount(BLACKKNIGHT));
	EXPECT_EQ(2u, orgMat->getPieceCount(BLACKBISHOP));
	EXPECT_EQ(2u, orgMat->getPieceCount(BLACKROOK));
	EXPECT_EQ(1u, orgMat->getPieceCount(BLACKQUEEN));
	EXPECT_EQ(1u, orgMat->getPieceCount(BLACKKING));

	EXPECT_TRUE(VerifyListsContainSameNotations({ a7, b7, c7, d7, e7, f7, g7, h7 }, orgMat->getPlacementsOfPiece(BLACKPAWN)));
	EXPECT_TRUE(VerifyListsContainSameNotations({ a8, h8 }, orgMat->getPlacementsOfPiece(BLACKROOK)));
	EXPECT_TRUE(VerifyListsContainSameNotations({ b8, g8 }, orgMat->getPlacementsOfPiece(BLACKKNIGHT)));
	EXPECT_TRUE(VerifyListsContainSameNotations({ c8, f8 }, orgMat->getPlacementsOfPiece(BLACKBISHOP)));

	EXPECT_EQ(d8, orgMat->getPlacementsOfPiece(BLACKQUEEN)[0]);
	EXPECT_EQ(e8, orgMat->getPlacementsOfPiece(BLACKKING)[0]);
        
    // 8 [ r ][   ][   ][   ][ r ][ n ][ k ][   ]
    // 7 [ p ][ b ][   ][   ][   ][ p ][ p ][   ]
    // 6 [   ][   ][   ][ p ][ p ][   ][   ][ p ]
    // 5 [   ][ q ][   ][   ][   ][   ][ B ][ Q ]
    // 4 [   ][ P ][   ][ P ][   ][   ][   ][   ]
    // 3 [   ][   ][   ][   ][ N ][   ][ R ][   ]
    // 2 [ P ][   ][   ][   ][   ][ P ][ P ][ P ]
    // 1 [ R ][   ][   ][   ][ R ][   ][ K ][   ]
    //     A    B    C    D    E    F    G    H
	orgMat = &m_gameOfTheCentury.readMaterial(Set::WHITE);
    
    EXPECT_EQ(6u, orgMat->getPieceCount(WHITEPAWN));
    EXPECT_EQ(1u, orgMat->getPieceCount(WHITEKNIGHT));
    EXPECT_EQ(1u, orgMat->getPieceCount(WHITEBISHOP));
    EXPECT_EQ(2u, orgMat->getPieceCount(WHITEROOK));
    EXPECT_EQ(1u, orgMat->getPieceCount(WHITEQUEEN));
    EXPECT_EQ(1u, orgMat->getPieceCount(WHITEKING));

    EXPECT_TRUE(VerifyListsContainSameNotations({ a2, b4, d4, f2, g2, h2 }, orgMat->getPlacementsOfPiece(WHITEPAWN)));
    EXPECT_TRUE(VerifyListsContainSameNotations({ e1, g3 }, orgMat->getPlacementsOfPiece(WHITEROOK)));
    
    EXPECT_EQ(g5, orgMat->getPlacementsOfPiece(WHITEBISHOP)[0]);
    EXPECT_EQ(e3, orgMat->getPlacementsOfPiece(WHITEKNIGHT)[0]);
    EXPECT_EQ(h5, orgMat->getPlacementsOfPiece(WHITEQUEEN)[0]);
    EXPECT_EQ(g1, orgMat->getPlacementsOfPiece(WHITEKING)[0]);

    orgMat = &m_gameOfTheCentury.readMaterial(Set::BLACK);    

    EXPECT_EQ(6u, orgMat->getPieceCount(BLACKPAWN));
    EXPECT_EQ(1u, orgMat->getPieceCount(BLACKKNIGHT));
    EXPECT_EQ(1u, orgMat->getPieceCount(BLACKBISHOP));
    EXPECT_EQ(2u, orgMat->getPieceCount(BLACKROOK));
    EXPECT_EQ(1u, orgMat->getPieceCount(BLACKQUEEN));
    EXPECT_EQ(1u, orgMat->getPieceCount(BLACKKING));

    EXPECT_TRUE(VerifyListsContainSameNotations({ a7, d6, e6, f7, g7, h6 }, orgMat->getPlacementsOfPiece(BLACKPAWN)));
    EXPECT_TRUE(VerifyListsContainSameNotations({ a8, e8 }, orgMat->getPlacementsOfPiece(BLACKROOK)));

    EXPECT_EQ(b7, orgMat->getPlacementsOfPiece(BLACKBISHOP)[0]);
    EXPECT_EQ(f8, orgMat->getPlacementsOfPiece(BLACKKNIGHT)[0]);
    EXPECT_EQ(b5, orgMat->getPlacementsOfPiece(BLACKQUEEN)[0]);
    EXPECT_EQ(g8, orgMat->getPlacementsOfPiece(BLACKKING)[0]);
}

////////////////////////////////////////////////////////////////

} // namespace ElephantTest
