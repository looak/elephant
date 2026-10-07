#include <gtest/gtest.h>

#include <position/castling_state_info.hpp>
#include <position/en_passant_state_info.hpp>

namespace ElephantTest {
////////////////////////////////////////////////////////////////

/**
 * @file castling_enpassant_test.cpp
 * @brief Fixture for testing castling and en passant functionality, combined into one since these two structs are so basic.
 * Naming convention as of October 2023: <TestedFunctionality>_<ExpectedResult>
 * @author Alexander Loodin Ek  */
class CastlingEnPassantFixture : public ::testing::Test {
public:
};
////////////////////////////////////////////////////////////////

TEST_F(CastlingEnPassantFixture, AllPermutationsOfCastlingStateInfo_ExpectReadFunctionsToReturnCorrectValues)
{
    CastlingStateInfo castlingStateInfo;    

    castlingStateInfo.write(0);
    EXPECT_EQ(CastlingState::NONE, castlingStateInfo.asFlag());
    EXPECT_FALSE(castlingStateInfo.hasAny());
    EXPECT_FALSE(castlingStateInfo.hasWhite());
    EXPECT_FALSE(castlingStateInfo.hasBlack());
    EXPECT_FALSE(castlingStateInfo.hasWhiteKingSide());
    EXPECT_FALSE(castlingStateInfo.hasWhiteQueenSide());
    EXPECT_FALSE(castlingStateInfo.hasBlackKingSide());
    EXPECT_FALSE(castlingStateInfo.hasBlackQueenSide());
    EXPECT_EQ("-", castlingStateInfo.toString());

    castlingStateInfo.write(1);
    EXPECT_EQ(CastlingState::WHITE_KINGSIDE, castlingStateInfo.asFlag());
    EXPECT_TRUE(castlingStateInfo.hasAny());
    EXPECT_TRUE(castlingStateInfo.hasWhite());
    EXPECT_FALSE(castlingStateInfo.hasBlack());
    EXPECT_TRUE(castlingStateInfo.hasWhiteKingSide());
    EXPECT_FALSE(castlingStateInfo.hasWhiteQueenSide());
    EXPECT_FALSE(castlingStateInfo.hasBlackKingSide());
    EXPECT_FALSE(castlingStateInfo.hasBlackQueenSide());
    EXPECT_EQ("K", castlingStateInfo.toString());

    castlingStateInfo.write(2);
    EXPECT_EQ(CastlingState::WHITE_QUEENSIDE, castlingStateInfo.asFlag());
    EXPECT_TRUE(castlingStateInfo.hasAny());
    EXPECT_TRUE(castlingStateInfo.hasWhite());
    EXPECT_FALSE(castlingStateInfo.hasBlack());
    EXPECT_FALSE(castlingStateInfo.hasWhiteKingSide());
    EXPECT_TRUE(castlingStateInfo.hasWhiteQueenSide());
    EXPECT_FALSE(castlingStateInfo.hasBlackKingSide());
    EXPECT_FALSE(castlingStateInfo.hasBlackQueenSide());
    EXPECT_EQ("Q", castlingStateInfo.toString());

    castlingStateInfo.write(3);
    EXPECT_EQ(CastlingState::WHITE_ALL, castlingStateInfo.asFlag());
    EXPECT_TRUE(castlingStateInfo.hasAny());
    EXPECT_TRUE(castlingStateInfo.hasWhite());
    EXPECT_FALSE(castlingStateInfo.hasBlack());
    EXPECT_TRUE(castlingStateInfo.hasWhiteKingSide());
    EXPECT_TRUE(castlingStateInfo.hasWhiteQueenSide());
    EXPECT_FALSE(castlingStateInfo.hasBlackKingSide());
    EXPECT_FALSE(castlingStateInfo.hasBlackQueenSide());
    EXPECT_EQ("KQ", castlingStateInfo.toString());

    castlingStateInfo.write(4);
    EXPECT_EQ(CastlingState::BLACK_KINGSIDE, castlingStateInfo.asFlag());
    EXPECT_TRUE(castlingStateInfo.hasAny());
    EXPECT_FALSE(castlingStateInfo.hasWhite());
    EXPECT_TRUE(castlingStateInfo.hasBlack());
    EXPECT_FALSE(castlingStateInfo.hasWhiteKingSide());
    EXPECT_FALSE(castlingStateInfo.hasWhiteQueenSide());
    EXPECT_TRUE(castlingStateInfo.hasBlackKingSide());
    EXPECT_FALSE(castlingStateInfo.hasBlackQueenSide());
    EXPECT_EQ("k", castlingStateInfo.toString());

    castlingStateInfo.write(5);
    EXPECT_EQ(CastlingState::WHITE_KINGSIDE | CastlingState::BLACK_KINGSIDE,
        castlingStateInfo.asFlag());
    EXPECT_TRUE(castlingStateInfo.hasAny());
    EXPECT_TRUE(castlingStateInfo.hasWhite());
    EXPECT_TRUE(castlingStateInfo.hasBlack());
    EXPECT_TRUE(castlingStateInfo.hasWhiteKingSide());
    EXPECT_FALSE(castlingStateInfo.hasWhiteQueenSide());
    EXPECT_TRUE(castlingStateInfo.hasBlackKingSide());
    EXPECT_FALSE(castlingStateInfo.hasBlackQueenSide());
    EXPECT_EQ("Kk", castlingStateInfo.toString());

    castlingStateInfo.write(6);
    EXPECT_EQ(CastlingState::WHITE_QUEENSIDE | CastlingState::BLACK_KINGSIDE,
        castlingStateInfo.asFlag());
    EXPECT_TRUE(castlingStateInfo.hasAny());
    EXPECT_TRUE(castlingStateInfo.hasWhite());
    EXPECT_TRUE(castlingStateInfo.hasBlack());
    EXPECT_FALSE(castlingStateInfo.hasWhiteKingSide());
    EXPECT_TRUE(castlingStateInfo.hasWhiteQueenSide());
    EXPECT_TRUE(castlingStateInfo.hasBlackKingSide());
    EXPECT_FALSE(castlingStateInfo.hasBlackQueenSide());
    EXPECT_EQ("Qk", castlingStateInfo.toString());

    castlingStateInfo.write(7);
    EXPECT_EQ(CastlingState::WHITE_ALL | CastlingState::BLACK_KINGSIDE, castlingStateInfo.asFlag());
    EXPECT_TRUE(castlingStateInfo.hasAny());
    EXPECT_TRUE(castlingStateInfo.hasWhite());
    EXPECT_TRUE(castlingStateInfo.hasBlack());
    EXPECT_TRUE(castlingStateInfo.hasWhiteKingSide());
    EXPECT_TRUE(castlingStateInfo.hasWhiteQueenSide());
    EXPECT_TRUE(castlingStateInfo.hasBlackKingSide());
    EXPECT_FALSE(castlingStateInfo.hasBlackQueenSide());
    EXPECT_EQ("KQk", castlingStateInfo.toString());

    castlingStateInfo.write(8);
    EXPECT_EQ(CastlingState::BLACK_QUEENSIDE, castlingStateInfo.asFlag());
    EXPECT_TRUE(castlingStateInfo.hasAny());
    EXPECT_FALSE(castlingStateInfo.hasWhite());
    EXPECT_TRUE(castlingStateInfo.hasBlack());
    EXPECT_FALSE(castlingStateInfo.hasWhiteKingSide());
    EXPECT_FALSE(castlingStateInfo.hasWhiteQueenSide());
    EXPECT_FALSE(castlingStateInfo.hasBlackKingSide());
    EXPECT_TRUE(castlingStateInfo.hasBlackQueenSide());
    EXPECT_EQ("q", castlingStateInfo.toString());

    castlingStateInfo.write(9);
    EXPECT_EQ(CastlingState::WHITE_KINGSIDE | CastlingState::BLACK_QUEENSIDE,
        castlingStateInfo.asFlag());
    EXPECT_TRUE(castlingStateInfo.hasAny());
    EXPECT_TRUE(castlingStateInfo.hasWhite());
    EXPECT_TRUE(castlingStateInfo.hasBlack());
    EXPECT_TRUE(castlingStateInfo.hasWhiteKingSide());
    EXPECT_FALSE(castlingStateInfo.hasWhiteQueenSide());
    EXPECT_FALSE(castlingStateInfo.hasBlackKingSide());
    EXPECT_TRUE(castlingStateInfo.hasBlackQueenSide());
    EXPECT_EQ("Kq", castlingStateInfo.toString());

    castlingStateInfo.write(10);
    EXPECT_EQ(CastlingState::WHITE_QUEENSIDE | CastlingState::BLACK_QUEENSIDE,
        castlingStateInfo.asFlag());
    EXPECT_TRUE(castlingStateInfo.hasAny());
    EXPECT_TRUE(castlingStateInfo.hasWhite());
    EXPECT_TRUE(castlingStateInfo.hasBlack());
    EXPECT_FALSE(castlingStateInfo.hasWhiteKingSide());
    EXPECT_TRUE(castlingStateInfo.hasWhiteQueenSide());
    EXPECT_FALSE(castlingStateInfo.hasBlackKingSide());
    EXPECT_TRUE(castlingStateInfo.hasBlackQueenSide());
    EXPECT_EQ("Qq", castlingStateInfo.toString());

    castlingStateInfo.write(11);
    EXPECT_EQ(CastlingState::WHITE_ALL | CastlingState::BLACK_QUEENSIDE, castlingStateInfo.asFlag());
    EXPECT_TRUE(castlingStateInfo.hasAny());
    EXPECT_TRUE(castlingStateInfo.hasWhite());
    EXPECT_TRUE(castlingStateInfo.hasBlack());
    EXPECT_TRUE(castlingStateInfo.hasWhiteKingSide());
    EXPECT_TRUE(castlingStateInfo.hasWhiteQueenSide());
    EXPECT_FALSE(castlingStateInfo.hasBlackKingSide());
    EXPECT_TRUE(castlingStateInfo.hasBlackQueenSide());
    EXPECT_EQ("KQq", castlingStateInfo.toString());

    castlingStateInfo.write(12);
    EXPECT_EQ(CastlingState::BLACK_ALL, castlingStateInfo.asFlag());
    EXPECT_TRUE(castlingStateInfo.hasAny());
    EXPECT_FALSE(castlingStateInfo.hasWhite());
    EXPECT_TRUE(castlingStateInfo.hasBlack());
    EXPECT_FALSE(castlingStateInfo.hasWhiteKingSide());
    EXPECT_FALSE(castlingStateInfo.hasWhiteQueenSide());
    EXPECT_TRUE(castlingStateInfo.hasBlackKingSide());
    EXPECT_TRUE(castlingStateInfo.hasBlackQueenSide());
    EXPECT_EQ("kq", castlingStateInfo.toString());

    castlingStateInfo.write(13);
    EXPECT_EQ(CastlingState::WHITE_KINGSIDE | CastlingState::BLACK_ALL, castlingStateInfo.asFlag());
    EXPECT_TRUE(castlingStateInfo.hasAny());
    EXPECT_TRUE(castlingStateInfo.hasWhite());
    EXPECT_TRUE(castlingStateInfo.hasBlack());
    EXPECT_TRUE(castlingStateInfo.hasWhiteKingSide());
    EXPECT_FALSE(castlingStateInfo.hasWhiteQueenSide());
    EXPECT_TRUE(castlingStateInfo.hasBlackKingSide());
    EXPECT_TRUE(castlingStateInfo.hasBlackQueenSide());
    EXPECT_EQ("Kkq", castlingStateInfo.toString());

    castlingStateInfo.write(14);
    EXPECT_EQ(CastlingState::WHITE_QUEENSIDE | CastlingState::BLACK_ALL, castlingStateInfo.asFlag());
    EXPECT_TRUE(castlingStateInfo.hasAny());
    EXPECT_TRUE(castlingStateInfo.hasWhite());
    EXPECT_TRUE(castlingStateInfo.hasBlack());
    EXPECT_FALSE(castlingStateInfo.hasWhiteKingSide());
    EXPECT_TRUE(castlingStateInfo.hasWhiteQueenSide());
    EXPECT_TRUE(castlingStateInfo.hasBlackKingSide());
    EXPECT_TRUE(castlingStateInfo.hasBlackQueenSide());
    EXPECT_EQ("Qkq", castlingStateInfo.toString());

    castlingStateInfo.write(15);
    EXPECT_EQ(CastlingState::WHITE_ALL | CastlingState::BLACK_ALL, castlingStateInfo.asFlag());
    EXPECT_TRUE(castlingStateInfo.hasAny());
    EXPECT_TRUE(castlingStateInfo.hasWhite());
    EXPECT_TRUE(castlingStateInfo.hasBlack());
    EXPECT_TRUE(castlingStateInfo.hasWhiteKingSide());
    EXPECT_TRUE(castlingStateInfo.hasWhiteQueenSide());
    EXPECT_TRUE(castlingStateInfo.hasBlackKingSide());
    EXPECT_TRUE(castlingStateInfo.hasBlackQueenSide());
    EXPECT_EQ("KQkq", castlingStateInfo.toString());
}


TEST_F(CastlingEnPassantFixture, WritingFunctionsOnCastlingStateInfo_ExpectToSetTheCorrectFlags)
{
    CastlingStateInfo castlingStateInfo;

    castlingStateInfo.grantAll();
    EXPECT_EQ(CastlingState::WHITE_ALL | CastlingState::BLACK_ALL, castlingStateInfo.asFlag());
    
    castlingStateInfo.clear();
    EXPECT_EQ(CastlingState::NONE, castlingStateInfo.asFlag());

    castlingStateInfo.grantAllWhite();
    EXPECT_EQ(CastlingState::WHITE_ALL, castlingStateInfo.asFlag());

    castlingStateInfo.grantAllBlack();
    EXPECT_EQ(CastlingState::WHITE_ALL | CastlingState::BLACK_ALL, castlingStateInfo.asFlag());

    castlingStateInfo.revokeAllWhite();
    EXPECT_EQ(CastlingState::BLACK_ALL, castlingStateInfo.asFlag());

    castlingStateInfo.revokeBlackKingSide();
    EXPECT_EQ(CastlingState::BLACK_QUEENSIDE, castlingStateInfo.asFlag());

    castlingStateInfo.revokeBlackQueenSide();
    EXPECT_EQ(CastlingState::NONE, castlingStateInfo.asFlag());
}

} // namespace ElephantTest