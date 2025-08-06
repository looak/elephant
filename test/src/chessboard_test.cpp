#include <gtest/gtest.h>

#include <algorithm>
#include "chessboard.h"
#include "elephant_test_utils.h"
#include <serializing/fen_parser.hpp>
#include "game_context.h"
#include <position/hash_zorbist.hpp>

namespace ElephantTest {
////////////////////////////////////////////////////////////////
class ChessboardFixture : public ::testing::Test {
public:
    virtual void SetUp()
    {
        DefaultStartingPosition();
        GameOfTheCentury_WindmillPosition();
    };
    virtual void TearDown() {};

    Chessboard m_emptyChessboard;  // by default a board should start empty.
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
    void DefaultStartingPosition() { SetupDefaultStartingPosition(m_defaultStartingPosition); }

    // https://en.wikipedia.org/wiki/The_Game_of_the_Century_(chess)
    // "Windmill Position from s.k. game of the century between Donald Bryen as White & Bobby
    // Fischer as Black. Played at the Marshall Chess Club in New York City on October 17th 1956. At
    // move 17 Fisher begins a windmill and that is where this board is.

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
    ChessPiece expectedPiece;  // empty, default, 0;

    // checking all tiles that they are empty
    for (int i = 0; i < 64; i++) {
        auto piece = m_emptyChessboard.readPieceAt((Square)i);
        EXPECT_EQ(expectedPiece, piece);
    }

    EXPECT_TRUE(m_emptyChessboard.readCastlingState().hasNone());
}

/**
8  [r][n][b][q][k][ ][ ][r]
7  [p][p][p][ ][b][p][p][p]
6  [ ][ ][ ][ ][p][n][ ][ ]
5  [ ][ ][ ][p][ ][ ][ ][ ]
4  [ ][ ][P][P][ ][ ][ ][ ]
3  [ ][ ][N][ ][ ][N][ ][ ]
2  [P][P][ ][ ][P][P][P][P]
1  [R][ ][B][Q][K][B][ ][R]
    A  B  C  D  E  F  G  H  */
TEST_F(ChessboardFixture, Chessboard_MakeMoves)
{
    Chessboard board(m_defaultStartingPosition);
    board.MakeMoves("c4", "e6", "Nf3", "d5", "d4", "Nf6", "Nc3", "Be7");

    std::string expectedFen("rnbqk2r/ppp1bppp/4pn2/3p4/2PP4/2N2N2/PP2PPPP/R1BQKB1R w KQkq - 0 1");
    GameContext context;
    FENParser::deserialize(expectedFen.c_str(), context);

    // LOG_INFO() << board.toString();
    EXPECT_EQ(context.readChessboard().readHash(), board.readHash());
}

/**
8  [r][n][b][q][k][b][ ][r]
7  [p][p][p][ ][.][p][p][p]
6  [ ][ ][ ][ ][p][n][ ][ ]
5  [ ][ ][ ][p][ ][ ][ ][ ]
4  [ ][ ][P][P][ ][ ][ ][ ]
3  [ ][ ][N][ ][ ][N][ ][ ]
2  [P][P][ ][ ][P][P][P][P]
1  [R][ ][B][Q][K][B][ ][R]
    A  B  C  D  E  F  G  H  */
TEST_F(ChessboardFixture, Chessboard_MakeMoves_Black)
{
    Chessboard board(m_defaultStartingPosition);
    board.MakeMoves("c4", "e6", "Nf3", "d5", "d4", "Nf6", "Nc3");

    std::string expectedFen("rnbqkb1r/ppp2ppp/4pn2/3p4/2PP4/2N2N2/PP2PPPP/R1BQKB1R b KQkq - 0 1");
    GameContext context;
    FENParser::deserialize(expectedFen.c_str(), context);

    // LOG_INFO() << board.toString();
    EXPECT_EQ(context.readChessboard().readHash(), board.readHash());
}

TEST_F(ChessboardFixture, Notation_Equality)
{
    Notation expected(0, 0);
    EXPECT_EQ(expected, expected);

    expected = Notation(0xf, 0xf);
    auto pos = Notation::BuildPosition('z', 1);  // invalid position
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
    pos = Notation::BuildPosition('z', 1);  // invalid position
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

#ifdef EG_DEBUGGING
    expected = 0xff;
    pos = Notation::BuildPosition('z', 1);  // invalid position
    EXPECT_EQ(expected, pos.index());

    Notation defaultValue;
    EXPECT_EQ(expected, defaultValue.index());
#endif
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

    for (int r = 0; r < 8; r++) {
        for (int f = 0; f < 8; f++) {
            expected = Notation(f, r);
            pos = Notation(expected.index());
            EXPECT_EQ(expected, pos);
        }
    }
}

TEST_F(ChessboardFixture, ChessboardIterator_Iterrating)
{
    auto itr = m_emptyChessboard.begin();
    // const Chessboard constBoard;

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
    for (int r = 0; r < 8; r++) {
        EXPECT_EQ(r, otherItr.rank());
        for (int f = 0; f < 8; f++) {
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
    // const Chessboard constBoard;

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
    // EXPECT_EQ(itrC, itrA);
    EXPECT_EQ(itrA, itrA);
    EXPECT_NE(itrA, itrB);
    // EXPECT_NE(itrC, itrB);
    // arbitrery adds
    itrA++;
    itrA++;
    itrA++;
    itrA++;
    itrB++;
    itrB++;
    itrB++;
    itrB++;
    itrC++;
    itrC++;
    itrC++;
    itrC++;

    // EXPECT_EQ(itrC, itrA);
    EXPECT_EQ(itrA, itrA);
    EXPECT_NE(itrA, itrB);
    EXPECT_EQ(itrC, itrC);
    itrA++;
    itrA++;
    itrB++;
    itrC++;
    itrC++;

    // EXPECT_EQ(itrC, itrA);
    EXPECT_EQ(itrA, itrA);
    EXPECT_NE(itrA, itrB);
    EXPECT_EQ(itrC, itrC);
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
    EXPECT_EQ(n, boardOne.readPieceAt(Square::B8));
    boardOne.PlacePiece(b, c8);
    EXPECT_EQ(b, boardOne.readPieceAt(Square::C8));
    boardOne.PlacePiece(q, d8);
    EXPECT_EQ(q, boardOne.readPieceAt(Square::D8));
    boardOne.PlacePiece(k, e8);
    EXPECT_EQ(k, boardOne.readPieceAt(Square::E8));
    boardOne.PlacePiece(b, f8);
    EXPECT_EQ(b, boardOne.readPieceAt(Square::F8));
    boardOne.PlacePiece(n, g8);
    EXPECT_EQ(n, boardOne.readPieceAt(Square::G8));

    boardOne.PlacePiece(p, a7);
    EXPECT_EQ(p, boardOne.readPieceAt(Square::A7));
    boardOne.PlacePiece(p, b7);
    EXPECT_EQ(p, boardOne.readPieceAt(Square::B7));
    boardOne.PlacePiece(p, c7);
    EXPECT_EQ(p, boardOne.readPieceAt(Square::C7));
    boardOne.PlacePiece(p, d7);
    EXPECT_EQ(p, boardOne.readPieceAt(Square::D7));
    boardOne.PlacePiece(p, e7);
    EXPECT_EQ(p, boardOne.readPieceAt(Square::E7));
    boardOne.PlacePiece(p, f7);
    EXPECT_EQ(p, boardOne.readPieceAt(Square::F7));
    boardOne.PlacePiece(p, g7);
    EXPECT_EQ(p, boardOne.readPieceAt(Square::G7));
    boardOne.PlacePiece(p, h7);
    EXPECT_EQ(p, boardOne.readPieceAt(Square::H7));

    // board Two
    boardTwo.PlacePiece(r, a8, true);
    EXPECT_EQ(r, boardTwo.readPieceAt(Square::A8));
    boardTwo.PlacePiece(n, b8);
    EXPECT_EQ(n, boardTwo.readPieceAt(Square::B8));
    boardTwo.PlacePiece(b, c8);
    EXPECT_EQ(b, boardTwo.readPieceAt(Square::C8));
    boardTwo.PlacePiece(q, d8);
    EXPECT_EQ(q, boardTwo.readPieceAt(Square::D8));
    boardTwo.PlacePiece(k, e8);
    EXPECT_EQ(k, boardTwo.readPieceAt(Square::E8));
    boardTwo.PlacePiece(b, f8);
    EXPECT_EQ(b, boardTwo.readPieceAt(Square::F8));
    boardTwo.PlacePiece(n, g8);
    EXPECT_EQ(n, boardTwo.readPieceAt(Square::G8));
    boardTwo.PlacePiece(r, h8);
    EXPECT_EQ(r, boardTwo.readPieceAt(Square::H8));

    boardTwo.PlacePiece(p, h7);
    EXPECT_EQ(p, boardTwo.readPieceAt(Square::H7));
    boardTwo.PlacePiece(p, g7);
    EXPECT_EQ(p, boardTwo.readPieceAt(Square::G7));
    boardTwo.PlacePiece(p, f7);
    EXPECT_EQ(p, boardTwo.readPieceAt(Square::F7));
    boardTwo.PlacePiece(p, e7);
    EXPECT_EQ(p, boardTwo.readPieceAt(Square::E7));
    boardTwo.PlacePiece(p, d7);
    EXPECT_EQ(p, boardTwo.readPieceAt(Square::D7));
    boardTwo.PlacePiece(p, c7);
    EXPECT_EQ(p, boardTwo.readPieceAt(Square::C7));
    boardTwo.PlacePiece(p, b7);
    EXPECT_EQ(p, boardTwo.readPieceAt(Square::B7));
    boardTwo.PlacePiece(p, a7);
    EXPECT_EQ(p, boardTwo.readPieceAt(Square::A7));

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

    Bitboard orgMat = m_gameOfTheCentury.readPosition().readMaterial().combine<Set::BLACK>();
    Bitboard cpyMat = scndCopy.readPosition().readMaterial().combine<Set::BLACK>();
    EXPECT_EQ(orgMat.count(), cpyMat.count());
    EXPECT_EQ(orgMat, cpyMat);

    orgMat = m_gameOfTheCentury.readPosition().readMaterial().combine<Set::WHITE>();
    cpyMat = scndCopy.readPosition().readMaterial().combine<Set::WHITE>();
    EXPECT_EQ(orgMat.count(), cpyMat.count());
    EXPECT_EQ(orgMat, cpyMat);
}

TEST_F(ChessboardFixture, CastlingStateInfoTest)
{
    EXPECT_TRUE(m_defaultStartingPosition.readCastlingState().hasAll());
    EXPECT_TRUE(m_gameOfTheCentury.readCastlingState().hasNone());

    const auto& castlingState = m_defaultStartingPosition.readPosition().refCastling();

    m_defaultStartingPosition.setCastlingState(0);
    EXPECT_EQ(CastlingState::NONE, m_defaultStartingPosition.readCastlingState().read());
    EXPECT_FALSE(castlingState.hasAny());
    EXPECT_FALSE(castlingState.hasWhite());
    EXPECT_FALSE(castlingState.hasBlack());
    EXPECT_FALSE(castlingState.hasWhiteKingSide());
    EXPECT_FALSE(castlingState.hasWhiteQueenSide());
    EXPECT_FALSE(castlingState.hasBlackKingSide());
    EXPECT_FALSE(castlingState.hasBlackQueenSide());

    m_defaultStartingPosition.setCastlingState(1);
    EXPECT_EQ(CastlingState::WHITE_KINGSIDE, m_defaultStartingPosition.readCastlingState().read());
    EXPECT_TRUE(castlingState.hasAny());
    EXPECT_TRUE(castlingState.hasWhite());
    EXPECT_FALSE(castlingState.hasBlack());
    EXPECT_TRUE(castlingState.hasWhiteKingSide());
    EXPECT_FALSE(castlingState.hasWhiteQueenSide());
    EXPECT_FALSE(castlingState.hasBlackKingSide());
    EXPECT_FALSE(castlingState.hasBlackQueenSide());

    m_defaultStartingPosition.setCastlingState(2);
    EXPECT_EQ(CastlingState::WHITE_QUEENSIDE, m_defaultStartingPosition.readCastlingState().read());
    EXPECT_TRUE(castlingState.hasAny());
    EXPECT_TRUE(castlingState.hasWhite());
    EXPECT_FALSE(castlingState.hasBlack());
    EXPECT_FALSE(castlingState.hasWhiteKingSide());
    EXPECT_TRUE(castlingState.hasWhiteQueenSide());
    EXPECT_FALSE(castlingState.hasBlackKingSide());
    EXPECT_FALSE(castlingState.hasBlackQueenSide());

    m_defaultStartingPosition.setCastlingState(3);
    EXPECT_EQ(CastlingState::WHITE_ALL, m_defaultStartingPosition.readCastlingState().read());
    EXPECT_TRUE(castlingState.hasAny());
    EXPECT_TRUE(castlingState.hasWhite());
    EXPECT_FALSE(castlingState.hasBlack());
    EXPECT_TRUE(castlingState.hasWhiteKingSide());
    EXPECT_TRUE(castlingState.hasWhiteQueenSide());
    EXPECT_FALSE(castlingState.hasBlackKingSide());
    EXPECT_FALSE(castlingState.hasBlackQueenSide());

    m_defaultStartingPosition.setCastlingState(4);
    EXPECT_EQ(CastlingState::BLACK_KINGSIDE, m_defaultStartingPosition.readCastlingState().read());
    EXPECT_TRUE(castlingState.hasAny());
    EXPECT_FALSE(castlingState.hasWhite());
    EXPECT_TRUE(castlingState.hasBlack());
    EXPECT_FALSE(castlingState.hasWhiteKingSide());
    EXPECT_FALSE(castlingState.hasWhiteQueenSide());
    EXPECT_TRUE(castlingState.hasBlackKingSide());
    EXPECT_FALSE(castlingState.hasBlackQueenSide());

    m_defaultStartingPosition.setCastlingState(5);
    EXPECT_EQ(CastlingState::WHITE_KINGSIDE | CastlingState::BLACK_KINGSIDE,
        m_defaultStartingPosition.readCastlingState().read());
    EXPECT_TRUE(castlingState.hasAny());
    EXPECT_TRUE(castlingState.hasWhite());
    EXPECT_TRUE(castlingState.hasBlack());
    EXPECT_TRUE(castlingState.hasWhiteKingSide());
    EXPECT_FALSE(castlingState.hasWhiteQueenSide());
    EXPECT_TRUE(castlingState.hasBlackKingSide());
    EXPECT_FALSE(castlingState.hasBlackQueenSide());

    m_defaultStartingPosition.setCastlingState(6);
    EXPECT_EQ(CastlingState::WHITE_QUEENSIDE | CastlingState::BLACK_KINGSIDE,
        m_defaultStartingPosition.readCastlingState().read());
    EXPECT_TRUE(castlingState.hasAny());
    EXPECT_TRUE(castlingState.hasWhite());
    EXPECT_TRUE(castlingState.hasBlack());
    EXPECT_FALSE(castlingState.hasWhiteKingSide());
    EXPECT_TRUE(castlingState.hasWhiteQueenSide());
    EXPECT_TRUE(castlingState.hasBlackKingSide());
    EXPECT_FALSE(castlingState.hasBlackQueenSide());

    m_defaultStartingPosition.setCastlingState(7);
    EXPECT_EQ(CastlingState::WHITE_ALL | CastlingState::BLACK_KINGSIDE, m_defaultStartingPosition.readCastlingState().read());
    EXPECT_TRUE(castlingState.hasAny());
    EXPECT_TRUE(castlingState.hasWhite());
    EXPECT_TRUE(castlingState.hasBlack());
    EXPECT_TRUE(castlingState.hasWhiteKingSide());
    EXPECT_TRUE(castlingState.hasWhiteQueenSide());
    EXPECT_TRUE(castlingState.hasBlackKingSide());
    EXPECT_FALSE(castlingState.hasBlackQueenSide());

    m_defaultStartingPosition.setCastlingState(8);
    EXPECT_EQ(CastlingState::BLACK_QUEENSIDE, m_defaultStartingPosition.readCastlingState().read());
    EXPECT_TRUE(castlingState.hasAny());
    EXPECT_FALSE(castlingState.hasWhite());
    EXPECT_TRUE(castlingState.hasBlack());
    EXPECT_FALSE(castlingState.hasWhiteKingSide());
    EXPECT_FALSE(castlingState.hasWhiteQueenSide());
    EXPECT_FALSE(castlingState.hasBlackKingSide());
    EXPECT_TRUE(castlingState.hasBlackQueenSide());

    m_defaultStartingPosition.setCastlingState(9);
    EXPECT_EQ(CastlingState::WHITE_KINGSIDE | CastlingState::BLACK_QUEENSIDE,
        m_defaultStartingPosition.readCastlingState().read());
    EXPECT_TRUE(castlingState.hasAny());
    EXPECT_TRUE(castlingState.hasWhite());
    EXPECT_TRUE(castlingState.hasBlack());
    EXPECT_TRUE(castlingState.hasWhiteKingSide());
    EXPECT_FALSE(castlingState.hasWhiteQueenSide());
    EXPECT_FALSE(castlingState.hasBlackKingSide());
    EXPECT_TRUE(castlingState.hasBlackQueenSide());

    m_defaultStartingPosition.setCastlingState(10);
    EXPECT_EQ(CastlingState::WHITE_QUEENSIDE | CastlingState::BLACK_QUEENSIDE,
        m_defaultStartingPosition.readCastlingState().read());
    EXPECT_TRUE(castlingState.hasAny());
    EXPECT_TRUE(castlingState.hasWhite());
    EXPECT_TRUE(castlingState.hasBlack());
    EXPECT_FALSE(castlingState.hasWhiteKingSide());
    EXPECT_TRUE(castlingState.hasWhiteQueenSide());
    EXPECT_FALSE(castlingState.hasBlackKingSide());
    EXPECT_TRUE(castlingState.hasBlackQueenSide());

    m_defaultStartingPosition.setCastlingState(11);
    EXPECT_EQ(CastlingState::WHITE_ALL | CastlingState::BLACK_QUEENSIDE, m_defaultStartingPosition.readCastlingState().read());
    EXPECT_TRUE(castlingState.hasAny());
    EXPECT_TRUE(castlingState.hasWhite());
    EXPECT_TRUE(castlingState.hasBlack());
    EXPECT_TRUE(castlingState.hasWhiteKingSide());
    EXPECT_TRUE(castlingState.hasWhiteQueenSide());
    EXPECT_FALSE(castlingState.hasBlackKingSide());
    EXPECT_TRUE(castlingState.hasBlackQueenSide());

    m_defaultStartingPosition.setCastlingState(12);
    EXPECT_EQ(CastlingState::BLACK_ALL, m_defaultStartingPosition.readCastlingState().read());
    EXPECT_TRUE(castlingState.hasAny());
    EXPECT_FALSE(castlingState.hasWhite());
    EXPECT_TRUE(castlingState.hasBlack());
    EXPECT_FALSE(castlingState.hasWhiteKingSide());
    EXPECT_FALSE(castlingState.hasWhiteQueenSide());
    EXPECT_TRUE(castlingState.hasBlackKingSide());
    EXPECT_TRUE(castlingState.hasBlackQueenSide());

    m_defaultStartingPosition.setCastlingState(13);
    EXPECT_EQ(CastlingState::WHITE_KINGSIDE | CastlingState::BLACK_ALL, m_defaultStartingPosition.readCastlingState().read());
    EXPECT_TRUE(castlingState.hasAny());
    EXPECT_TRUE(castlingState.hasWhite());
    EXPECT_TRUE(castlingState.hasBlack());
    EXPECT_TRUE(castlingState.hasWhiteKingSide());
    EXPECT_FALSE(castlingState.hasWhiteQueenSide());
    EXPECT_TRUE(castlingState.hasBlackKingSide());
    EXPECT_TRUE(castlingState.hasBlackQueenSide());

    m_defaultStartingPosition.setCastlingState(14);
    EXPECT_EQ(CastlingState::WHITE_QUEENSIDE | CastlingState::BLACK_ALL, m_defaultStartingPosition.readCastlingState().read());
    EXPECT_TRUE(castlingState.hasAny());
    EXPECT_TRUE(castlingState.hasWhite());
    EXPECT_TRUE(castlingState.hasBlack());
    EXPECT_FALSE(castlingState.hasWhiteKingSide());
    EXPECT_TRUE(castlingState.hasWhiteQueenSide());
    EXPECT_TRUE(castlingState.hasBlackKingSide());
    EXPECT_TRUE(castlingState.hasBlackQueenSide());

    m_defaultStartingPosition.setCastlingState(15);
    EXPECT_EQ(CastlingState::WHITE_ALL | CastlingState::BLACK_ALL, m_defaultStartingPosition.readCastlingState().read());
    EXPECT_TRUE(castlingState.hasAny());
    EXPECT_TRUE(castlingState.hasWhite());
    EXPECT_TRUE(castlingState.hasBlack());
    EXPECT_TRUE(castlingState.hasWhiteKingSide());
    EXPECT_TRUE(castlingState.hasWhiteQueenSide());
    EXPECT_TRUE(castlingState.hasBlackKingSide());
    EXPECT_TRUE(castlingState.hasBlackQueenSide());
}

////////////////////////////////////////////////////////////////

}  // namespace ElephantTest
