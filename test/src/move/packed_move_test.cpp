#include <gtest/gtest.h>
#include <move/move.hpp>

namespace ElephantTest {

////////////////////////////////////////////////////////////////
/**
 * @file packed_move_test.cpp
 * @brief Tests for the PackedMove class.
 * @author Alexander Loodin Ek    */
class PackedMoveTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

////////////////////////////////////////////////////////////////
// Constructor Tests
TEST_F(PackedMoveTest, DefaultConstructor_CreatesNullMove)
{
    PackedMove move{};
    
    EXPECT_TRUE(move.isNull());
    EXPECT_EQ(0, move.read());
}

TEST_F(PackedMoveTest, PackedConstructor_SetsInternalValue)
{
    u16 packedValue = 0x1234;
    PackedMove move(packedValue);
    
    EXPECT_EQ(packedValue, move.read());
    EXPECT_FALSE(move.isNull());
}

TEST_F(PackedMoveTest, SquareConstructor_SetsSourceAndTarget)
{
    PackedMove move(Square::E2, Square::E4);
    
    EXPECT_EQ(Square::E2, move.sourceSqr());
    EXPECT_EQ(Square::E4, move.targetSqr());
    EXPECT_FALSE(move.isNull());
}

TEST_F(PackedMoveTest, CopyConstructor_CopiesCorrectly)
{
    PackedMove original(Square::A1, Square::H8);
    PackedMove copy(original);
    
    EXPECT_EQ(original.read(), copy.read());
    EXPECT_EQ(original.sourceSqr(), copy.sourceSqr());
    EXPECT_EQ(original.targetSqr(), copy.targetSqr());
}

// Square Accessor Tests
TEST_F(PackedMoveTest, SourceSquare_AllSquares)
{
    for (int sq = 0; sq < 64; ++sq) {
        PackedMove move;
        move.setSource(static_cast<Square>(sq));
        
        EXPECT_EQ(sq, move.source());
        EXPECT_EQ(static_cast<Square>(sq), move.sourceSqr());
    }
}

TEST_F(PackedMoveTest, TargetSquare_AllSquares)
{
    for (int sq = 0; sq < 64; ++sq) {
        PackedMove move;
        move.setTarget(static_cast<Square>(sq));
        
        EXPECT_EQ(sq, move.target());
        EXPECT_EQ(static_cast<Square>(sq), move.targetSqr());
    }
}

TEST_F(PackedMoveTest, SourceAndTarget_Combined)
{
    PackedMove move;
    move.setSource(Square::A1);  // 0
    move.setTarget(Square::H8);  // 63
    
    EXPECT_EQ(Square::A1, move.sourceSqr());
    EXPECT_EQ(Square::H8, move.targetSqr());
    EXPECT_EQ(0, move.source());
    EXPECT_EQ(63, move.target());
}

// Flag Tests
TEST_F(PackedMoveTest, QuietMove_DefaultFlags)
{
    PackedMove move(Square::E2, Square::E4);
    
    EXPECT_TRUE(move.isQuiet());
    EXPECT_FALSE(move.isCapture());
    EXPECT_FALSE(move.isPromotion());
    EXPECT_FALSE(move.isCastling());
    EXPECT_FALSE(move.isEnPassant());
    EXPECT_EQ(0, move.flags());
}

TEST_F(PackedMoveTest, CaptureFlag_SetAndCheck)
{
    PackedMove move(Square::E4, Square::D5);
    move.setCapture(true);
    
    EXPECT_TRUE(move.isCapture());
    EXPECT_FALSE(move.isQuiet());
    EXPECT_EQ(CAPTURES, move.flags());
    
    move.setCapture(false);
    EXPECT_FALSE(move.isCapture());
    EXPECT_TRUE(move.isQuiet());
}

TEST_F(PackedMoveTest, EnPassantFlag_SetAndCheck)
{
    PackedMove move(Square::E5, Square::D6);
    move.setEnPassant(true);
    
    EXPECT_TRUE(move.isEnPassant());
    EXPECT_TRUE(move.isCapture());  // En passant implies capture
    EXPECT_EQ(EN_PASSANT_CAPTURE, move.flags());
    
    move.setEnPassant(false);
    EXPECT_FALSE(move.isEnPassant());
}

bool testPiecePromotion(PackedMove move, ChessPiece pieceToTest)
{
    move.setPromoteTo(pieceToTest);
    
    EXPECT_EQ(pieceToTest.getType(), static_cast<PieceType>(move.readPromoteToPieceType()));
    EXPECT_EQ(PROMOTIONS, move.flags() & PROMOTIONS);

    return move.isPromotion();
}

TEST_F(PackedMoveTest, PromotionFlags_AllPieceTypes)
{
    PackedMove move(Square::A7, Square::A8);

    EXPECT_TRUE(testPiecePromotion(move, piece_constants::white_queen));
    EXPECT_TRUE(testPiecePromotion(move, piece_constants::black_knight));
    EXPECT_TRUE(testPiecePromotion(move, piece_constants::white_bishop));
    EXPECT_TRUE(testPiecePromotion(move, piece_constants::black_rook));
}

TEST_F(PackedMoveTest, PromotionWithCapture_CombinedFlags)
{
    PackedMove move(Square::B7, Square::C8);
    move.setPromoteTo(piece_constants::black_queen);
    move.setCapture(true);
    
    EXPECT_TRUE(move.isPromotion());
    EXPECT_TRUE(move.isCapture());
    EXPECT_EQ(PieceType::QUEEN, static_cast<PieceType>(move.readPromoteToPieceType()));    
}

TEST_F(PackedMoveTest, CastlingFlags_KingSide)
{
    PackedMove move(Square::E1, Square::G1);
    move.setCastleKingSide(true);
    
    EXPECT_TRUE(move.isCastling());
    EXPECT_EQ(KING_CASTLE, move.flags());
    
    move.setCastleKingSide(false);
    EXPECT_FALSE(move.isCastling());
}

TEST_F(PackedMoveTest, CastlingFlags_QueenSide)
{
    PackedMove move(Square::E1, Square::C1);
    move.setCastleQueenSide(true);
    
    EXPECT_TRUE(move.isCastling());
    EXPECT_EQ(QUEEN_CASTLE, move.flags());
    
    move.setCastleQueenSide(false);
    EXPECT_FALSE(move.isCastling());
}

// Static Methods Tests
TEST_F(PackedMoveTest, NullMove_IsNull)
{
    PackedMove nullMove = PackedMove::NullMove();
    
    EXPECT_TRUE(nullMove.isNull());
    EXPECT_EQ(0, nullMove.read());
    EXPECT_FALSE(nullMove);  // operator bool
}

// Operator Tests
TEST_F(PackedMoveTest, EqualityOperator_SameMoves)
{
    PackedMove move1(Square::E2, Square::E4);
    PackedMove move2(Square::E2, Square::E4);
    
    EXPECT_TRUE(move1 == move2);
    EXPECT_FALSE(move1 != move2);
}

TEST_F(PackedMoveTest, EqualityOperator_DifferentMoves)
{
    PackedMove move1(Square::E2, Square::E4);
    PackedMove move2(Square::E2, Square::E3);
    
    EXPECT_FALSE(move1 == move2);
    EXPECT_TRUE(move1 != move2);
}

TEST_F(PackedMoveTest, BoolOperator_NullAndValidMoves)
{
    PackedMove nullMove;
    PackedMove validMove(Square::A1, Square::A2);
    
    EXPECT_FALSE(nullMove);
    EXPECT_TRUE(validMove);
}

// String Conversion Tests
TEST_F(PackedMoveTest, ToString_SimpleMove)
{
    PackedMove move(Square::E2, Square::E4);
    
    EXPECT_EQ("e2e4", move.toString());
}

TEST_F(PackedMoveTest, ToString_PromotionMove)
{
    PackedMove move(Square::A7, Square::A8);
    move.setPromoteTo(piece_constants::black_queen);
    
    EXPECT_EQ("a7a8q", move.toString());
}

// Edge Cases and Boundary Tests
TEST_F(PackedMoveTest, MaxSquareValues_SourceAndTarget)
{
    PackedMove move;
    move.setSource(63);  // H8
    move.setTarget(63);  // H8
    
    EXPECT_EQ(63, move.source());
    EXPECT_EQ(63, move.target());
    EXPECT_EQ(Square::H8, move.sourceSqr());
    EXPECT_EQ(Square::H8, move.targetSqr());
}

TEST_F(PackedMoveTest, BitMaskIntegrity_SourceDoesNotAffectTarget)
{
    PackedMove move;
    move.setTarget(Square::H8);  // Set target first
    move.setSource(Square::A1);  // Set source after
    
    EXPECT_EQ(Square::A1, move.sourceSqr());
    EXPECT_EQ(Square::H8, move.targetSqr());
}

TEST_F(PackedMoveTest, BitMaskIntegrity_FlagsDoNotAffectSquares)
{
    PackedMove move(Square::D4, Square::E5);
    move.setCapture(true);
    move.setEnPassant(true);
    
    EXPECT_EQ(Square::D4, move.sourceSqr());
    EXPECT_EQ(Square::E5, move.targetSqr());
    EXPECT_TRUE(move.isCapture());
    EXPECT_TRUE(move.isEnPassant());
}

TEST_F(PackedMoveTest, PackedValueRoundTrip_PreservesData)
{
    PackedMove original(Square::F3, Square::G5);
    original.setCapture(true);
    original.setPromoteTo(4);  // Rook
    
    u16 packed = original.read();
    PackedMove reconstructed(packed);
    
    EXPECT_EQ(original.sourceSqr(), reconstructed.sourceSqr());
    EXPECT_EQ(original.targetSqr(), reconstructed.targetSqr());
    EXPECT_EQ(original.isCapture(), reconstructed.isCapture());
    EXPECT_EQ(original.isPromotion(), reconstructed.isPromotion());
    EXPECT_EQ(original.readPromoteToPieceType(), reconstructed.readPromoteToPieceType());
}

} // namespace ElephantTest
