#include <gtest/gtest.h>

#include <core/chessboard.hpp>
#include <position/hash_zobrist.hpp>

#include "chess_positions.hpp"


namespace ElephantTest {
////////////////////////////////////////////////////////////////
class ChessboardFixture : public ::testing::Test {
public:
    virtual void SetUp() {};
    virtual void TearDown() {}; 

};

////////////////////////////////////////////////////////////////

TEST_F(ChessboardFixture, Constructor_Copy)
{
    Chessboard defaultBoard;
    chess_positions::defaultStartingPosition(defaultBoard.editPosition());

    Chessboard copy(defaultBoard);

    u64 copyBoardHash = zobrist::computeBoardHash(copy);
    u64 defaultBoardHash = zobrist::computeBoardHash(defaultBoard);

    EXPECT_EQ(copyBoardHash, defaultBoardHash);
    EXPECT_EQ(copyBoardHash, copy.readPosition().hash());
    EXPECT_EQ(defaultBoardHash, defaultBoard.readPosition().hash());

    Chessboard gameOfCentury;
    chess_positions::windmillPosition(gameOfCentury.editPosition());
    Chessboard scndCopy(gameOfCentury);
    u64 scndCopyHash = zobrist::computeBoardHash(scndCopy);
    u64 goatGame = zobrist::computeBoardHash(gameOfCentury);

    EXPECT_EQ(goatGame, gameOfCentury.readPosition().hash());
    EXPECT_EQ(scndCopyHash, scndCopy.readPosition().hash());
    EXPECT_EQ(goatGame, scndCopyHash);

    Bitboard orgMat = gameOfCentury.readPosition().material().combine<Set::BLACK>();
    Bitboard cpyMat = scndCopy.readPosition().material().combine<Set::BLACK>();
    EXPECT_EQ(orgMat.count(), cpyMat.count());
    EXPECT_EQ(orgMat, cpyMat);

    orgMat = gameOfCentury.readPosition().material().combine<Set::WHITE>();
    cpyMat = scndCopy.readPosition().material().combine<Set::WHITE>();
    EXPECT_EQ(orgMat.count(), cpyMat.count());
    EXPECT_EQ(orgMat, cpyMat);
}

////////////////////////////////////////////////////////////////

}  // namespace ElephantTest
