#include <gtest/gtest.h>
#include "elephant_test_utils.h"
#include "game_context.h"
#include "fen_parser.h"
#include "move_generator.h"

namespace ElephantTest
{
////////////////////////////////////////////////////////////////
class MoveGeneratorFixture : public ::testing::Test
{
public:
    virtual void SetUp()
    {

    };
    virtual void TearDown() {};

    MoveGenerator moveGenerator;
    GameContext testContext;
};
////////////////////////////////////////////////////////////////

TEST_F(MoveGeneratorFixture, Empty)
{
    auto result = moveGenerator.GeneratePossibleMoves(testContext);
    EXPECT_EQ(0, result.size());
}

TEST_F(MoveGeneratorFixture, WhiteKing)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(WHITEKING, e1);

    auto result = moveGenerator.GeneratePossibleMoves(testContext);
    EXPECT_EQ(5, result.size());
}

TEST_F(MoveGeneratorFixture, WhiteKingAndPawn)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(WHITEKING, e1);
    board.PlacePiece(WHITEPAWN, e2);

    auto result = moveGenerator.GeneratePossibleMoves(testContext);
    EXPECT_EQ(6, result.size());
}

TEST_F(MoveGeneratorFixture, KnightOneCapture)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(WHITEKNIGHT, e4);
    board.PlacePiece(BLACKKNIGHT, f6);    

    auto result = moveGenerator.GeneratePossibleMoves(testContext);
    EXPECT_EQ(8, result.size());

    for (auto&& move : result)
    {
        if (move.TargetSquare == f6)
        {
            EXPECT_EQ(move.Flags, MoveFlag::Capture);
            break;
        }
    }    
}

TEST_F(MoveGeneratorFixture, PawnPromotion)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKPAWN, a2);    
    testContext.editToPlay() = PieceSet::BLACK;

    // do
    auto result = moveGenerator.GeneratePossibleMoves(testContext);
    
    // verify
    EXPECT_EQ(4, result.size());
    for (auto&& move : result)
    {        
        EXPECT_EQ(move.Flags, MoveFlag::Promotion);        
    } 
}

TEST_F(MoveGeneratorFixture, PawnPromotionCapture)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKPAWN, a2);
    board.PlacePiece(WHITEROOK, b1);
    testContext.editToPlay() = PieceSet::BLACK;

    // do 
    auto result = moveGenerator.GeneratePossibleMoves(testContext);

    // verify
    EXPECT_EQ(8, result.size());
    for (auto&& move : result)
    {
        if (move.TargetSquare == b1)
            EXPECT_EQ(MoveFlag::Capture, move.Flags & MoveFlag::Capture);
        EXPECT_EQ(MoveFlag::Promotion, move.Flags & MoveFlag::Promotion);
    } 
}


} // namespace ElephantTest