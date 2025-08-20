#include <position/position_proxy.hpp>
#include <gtest/gtest.h>
#include "elephant_test_utils.h"
#include <core/chessboard.hpp>

namespace ElephantTest {
////////////////////////////////////////////////////////////////

/**
 * @file position_proxy_test.cpp
 * @brief Fixture for testing position proxy structure with both reading and writing capabilities.
 * Naming convention as of July 2025: <TestedFunctionality>_<ExpectedResult>
 * @author Alexander Loodin Ek
 */
class PositionProxyFixture : public ::testing::Test {
public:

};

////////////////////////////////////////////////////////////////
TEST_F(PositionProxyFixture, PositionReader_CreationAndIsEmpty_ShouldBeFalse)
{
    Chessboard board;
    SetupDefaultStartingPosition(board);

    auto positionReader = board.readPosition().read();
    EXPECT_FALSE(positionReader.empty()) << "Position should not be empty.";    
}

/**
 * @brief Tests the PositionProxy's ability iterate over the board and read and write pieces through the iterator.
 */
TEST_F(PositionProxyFixture, PositionIterator_FromA1toH8)
{
    Chessboard board;
    const auto positionReader = board.readPosition().read();
    auto itr = positionReader.begin();

    auto otherItr = positionReader.begin();
    for (int r = 0; r < 8; r++) {
        EXPECT_EQ(r, otherItr.rank());
        for (int f = 0; f < 8; f++) {
            auto notation = Notation(f, r);
            byte expectedIndex = notation.index();
            EXPECT_EQ(expectedIndex, *otherItr.square());
            EXPECT_EQ(f, otherItr.file());
            EXPECT_EQ(r, otherItr.rank());
            otherItr++;
        }
    }
}

TEST_F(PositionProxyFixture, PositionIterator_MutableIterator)
{
    Position position;

    const auto positionReader = position.read();

    auto itr = positionReader.begin();

    auto positionEditor = position.edit();
    auto mutItr = positionEditor.begin();
    mutItr++;
    mutItr.set(ChessPiece(Set::WHITE, PieceType::PAWN));
    positionEditor[Square::A1] = ChessPiece(Set::WHITE, PieceType::PAWN);

    EXPECT_EQ(itr.get(), WHITEPAWN);
    itr++;
    EXPECT_EQ(itr.get(), WHITEPAWN);
    
    EXPECT_EQ(positionReader[Square::A1], WHITEPAWN);
}

TEST_F(PositionProxyFixture, PositionIterator_IterratingExtended_ArbitraryIncrements)
{
     Chessboard board;
    auto positionReader = board.readPosition().read();
    auto itr = positionReader.begin();

    byte expectedIndex = 0;
    byte expectedRank = 0;
    byte expectedFile = 0;
    EXPECT_FALSE(itr.end());
    EXPECT_EQ(expectedIndex, *itr.square());
    EXPECT_EQ(expectedRank, itr.rank());
    EXPECT_EQ(expectedFile, itr.file());

    itr += 2;
    expectedIndex = 2;
    expectedRank = 0;
    expectedFile = 2;
    EXPECT_EQ(expectedIndex, *itr.square());
    EXPECT_EQ(expectedRank, itr.rank());
    EXPECT_EQ(expectedFile, itr.file());

    itr += 8;
    expectedIndex = 10;
    expectedRank = 1;
    expectedFile = 2;
    EXPECT_EQ(expectedIndex, *itr.square());
    EXPECT_EQ(expectedRank, itr.rank());
    EXPECT_EQ(expectedFile, itr.file());

    itr += 16;
    expectedIndex = 26;
    expectedRank = 3;
    expectedFile = 2;
    EXPECT_EQ(expectedIndex, *itr.square());
    EXPECT_EQ(expectedRank, itr.rank());
    EXPECT_EQ(expectedFile, itr.file());

    itr += 7;
    expectedIndex = 33;
    expectedRank = 4;
    expectedFile = 1;
    EXPECT_EQ(expectedIndex, *itr.square());
    EXPECT_EQ(expectedRank, itr.rank());
    EXPECT_EQ(expectedFile, itr.file());

    itr += 7;
    expectedIndex = 40;
    expectedRank = 5;
    expectedFile = 0;
    EXPECT_EQ(expectedIndex, *itr.square());
    EXPECT_EQ(expectedRank, itr.rank());
    EXPECT_EQ(expectedFile, itr.file());

    itr += 7;
    expectedIndex = 47;
    expectedRank = 5;
    expectedFile = 7;
    EXPECT_EQ(expectedIndex, *itr.square());
    EXPECT_EQ(expectedRank, itr.rank());
    EXPECT_EQ(expectedFile, itr.file());

    itr += 7;
    expectedIndex = 54;
    expectedRank = 6;
    expectedFile = 6;
    EXPECT_EQ(expectedIndex, *itr.square());
    EXPECT_EQ(expectedRank, itr.rank());
    EXPECT_EQ(expectedFile, itr.file());

    itr += 7;
    expectedIndex = 61;
    expectedRank = 7;
    expectedFile = 5;
    EXPECT_EQ(expectedIndex, *itr.square());
    EXPECT_EQ(expectedRank, itr.rank());
    EXPECT_EQ(expectedFile, itr.file());

    itr += 7;
    expectedIndex = 64;
    expectedRank = 8;
    expectedFile = 0;
    EXPECT_EQ(expectedIndex, *itr.square());
    EXPECT_EQ(expectedRank, itr.rank());
    EXPECT_EQ(expectedFile, itr.file());

    auto scnditr = positionReader.begin();

    scnditr += 11;
    expectedIndex = 11;
    expectedRank = 1;
    expectedFile = 3;
    EXPECT_EQ(expectedIndex, *scnditr.square());
    EXPECT_EQ(expectedRank, scnditr.rank());
    EXPECT_EQ(expectedFile, scnditr.file());
}

TEST_F(PositionProxyFixture, PositionIterator_Equality)
{
    // implement const vs non-const equality operator
    Chessboard board;
    auto positionReader = board.readPosition().read();
    Chessboard b;
    auto positionReaderB = b.readPosition().read();

    const Chessboard cb;
    auto positionReaderC = cb.readPosition().read();

    auto itrA = positionReaderB.begin();
    auto itrD = positionReaderB.begin();
    auto itrB = positionReader.begin();
    auto itrC = positionReaderC.begin();

    EXPECT_NE(positionReaderB.begin(), positionReaderB.end());
    EXPECT_EQ(positionReaderB.begin(), positionReaderB.begin());
    EXPECT_EQ(positionReaderB.end(), positionReaderB.end());

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

TEST_F(PositionProxyFixture, PositionEditPolicy_placePiecesAndHashing)
{
    Position position;
    auto K = WHITEKING;
    auto Q = WHITEQUEEN;
    auto R = WHITEROOK;
    auto B = WHITEBISHOP;
    auto N = WHITEKNIGHT;
    auto P = WHITEPAWN;

    auto k = BLACKKING;
    auto q = BLACKQUEEN;
    auto r = BLACKROOK;
    auto b = BLACKBISHOP;
    auto n = BLACKKNIGHT;
    auto p = BLACKPAWN;

    auto reader = position.read();
    auto editor = position.edit();

    auto whiteKing = reader.readMaterial().king<Set::WHITE>();
    u64 oldHash = reader.readHash();

    // validate emptiness
    EXPECT_EQ(0, whiteKing.count());
    EXPECT_EQ(0, oldHash);

    // do
    editor.placePieces(K, a1);
    whiteKing = reader.readMaterial().king<Set::WHITE>();

    // validate
    EXPECT_EQ(K, reader[Square::A1]);
    EXPECT_EQ(1, whiteKing.count());
    EXPECT_NE(oldHash, reader.readHash());
    EXPECT_TRUE(whiteKing[Square::A1]);

    // do some more
    editor.placePieces(r, Square::C6, r, Square::C7, r, Square::G4);

    // validate
    const auto& blackRooks = reader.readMaterial().rooks<Set::BLACK>();
    EXPECT_EQ(3, blackRooks.count());
    EXPECT_TRUE(blackRooks[Square::C6]);
    EXPECT_TRUE(blackRooks[Square::C7]);
    EXPECT_TRUE(blackRooks[Square::G4]);
    EXPECT_EQ(r, reader[Square::C6]);
    EXPECT_EQ(r, reader[Square::C7]);
    EXPECT_EQ(r, reader[Square::G4]);
    EXPECT_NE(oldHash, reader.readHash());

    // expect white king to still be there.
    EXPECT_EQ(K, reader[Square::A1]);
    EXPECT_EQ(1, whiteKing.count());

    Position startPostion;    
    startPostion.edit().placePieces(r, a8, n, b8, b, c8, q, d8, k, e8, b, f8, n, g8, r, h8);
    startPostion.edit().placePieces(p, a7, p, b7, p, c7, p, d7, p, e7, p, f7, p, g7, p, h7);
    startPostion.edit().placePieces(P, a2, P, b2, P, c2, P, d2, P, e2, P, f2, P, g2, P, h2);
    startPostion.edit().placePieces(R, a1, N, b1, B, c1, Q, d1, K, e1, B, f1, N, g1, R, h1);
    // startPostion.setCastlingState(15);

    // for (const auto& itr : m_defaultStartingPosition) {
    //     EXPECT_EQ(itr.get(), startPostion.readPieceAt(itr.square()));
    // }

    // EXPECT_EQ(startPostion.readHash(), m_defaultStartingPosition.readHash());
}

} // namespace ElephantTest