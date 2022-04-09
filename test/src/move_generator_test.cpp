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

    auto count =  moveGenerator.CountMoves(result);
    EXPECT_EQ(6, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);
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
    testContext.editToPlay() = Set::BLACK;

    // do
    auto result = moveGenerator.GeneratePossibleMoves(testContext);
    
    // verify
    EXPECT_EQ(4, result.size());
    for (auto&& move : result)
    {        
        EXPECT_EQ(MoveFlag::Promotion, move.Flags & MoveFlag::Promotion);
    } 
}

TEST_F(MoveGeneratorFixture, PawnPromotionCapture)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKPAWN, a2);
    board.PlacePiece(WHITEROOK, b1);
    testContext.editToPlay() = Set::BLACK;

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

// 8 [   ][   ][   ][   ][   ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [ p ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][ R ][   ][   ][   ][   ][ K ][   ]
//     A    B    C    D    E    F    G    H
// valid moves:
// a1=Q, a1=R, a1=B, a1=N, b1=Q+ b1=R+, b1=B, b1=N
// 8 promotions, 4 of which are captures, two of which are checks.
TEST_F(MoveGeneratorFixture, PawnPromotionCaptureCheck)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKPAWN, a2);
    board.PlacePiece(WHITEROOK, b1);
    board.PlacePiece(WHITEKING, g1);
    testContext.editToPlay() = Set::BLACK;

    // do 
    auto result = moveGenerator.GeneratePossibleMoves(testContext);

    // verify
    EXPECT_EQ(8, result.size());
    for (auto&& move : result)
    {
        if (move.TargetSquare == b1)
        {
            EXPECT_EQ(MoveFlag::Capture, move.Flags & MoveFlag::Capture);
            if (move.Promote == BLACKQUEEN || move.Promote == BLACKROOK)
                EXPECT_EQ(MoveFlag::Check, move.Flags & MoveFlag::Check);
            else
                EXPECT_NE(MoveFlag::Check, move.Flags & MoveFlag::Check);
        }
        else
            EXPECT_NE(MoveFlag::Check, move.Flags & MoveFlag::Check);
        EXPECT_EQ(MoveFlag::Promotion, move.Flags & MoveFlag::Promotion);
    }

    auto count =  moveGenerator.CountMoves(result);
    EXPECT_EQ(8, count.Moves);
    EXPECT_EQ(4, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(8, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(2, count.Checks);
    EXPECT_EQ(0, count.Checkmates);
}

// 8 [   ][   ][   ][ r ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][ K ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// valid moves:
// e2, f2, f1
TEST_F(MoveGeneratorFixture, Check)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKROOK, d8);
    board.PlacePiece(WHITEKING, e1);
    
    // do 
    auto result = moveGenerator.GeneratePossibleMoves(testContext);

    EXPECT_EQ(3, result.size());
}

// 8 [   ][   ][   ][ r ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][ n ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][ K ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// valid moves:
// e2, f2, d1
// can not capture knight on d2 since it is guarded
// by rook on d8.
TEST_F(MoveGeneratorFixture, GuardedPiece)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKROOK, d8);
    board.PlacePiece(BLACKKNIGHT, d2);
    board.PlacePiece(WHITEKING, e1);
    
    // do 
    auto result = moveGenerator.GeneratePossibleMoves(testContext);

    EXPECT_EQ(3, result.size());
}

// 8 [   ][   ][   ][ r ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][ n ][   ][   ]
// 2 [   ][   ][   ][ n ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][ K ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// valid moves:
// e2, f2, d1
// can not capture knight on d2 since it is guarded
// by rook on d8.
TEST_F(MoveGeneratorFixture, CheckGuardedPiece)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKROOK, d8);
    board.PlacePiece(BLACKKNIGHT, d2);
    board.PlacePiece(BLACKKNIGHT, f3);
    board.PlacePiece(WHITEKING, e1);
    
    // do 
    auto result = moveGenerator.GeneratePossibleMoves(testContext);

    EXPECT_EQ(3, result.size());
}

// 8 [   ][   ][   ][ r ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][ r ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][ n ][   ][   ]
// 2 [   ][   ][   ][ n ][   ][   ][ P ][   ]
// 1 [   ][   ][   ][   ][ K ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// valid moves:
// f2, d1
TEST_F(MoveGeneratorFixture, CheckGuardedPiece_OnlyValidMovesAreToMoveKing)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKROOK, d8);
    board.PlacePiece(BLACKROOK, e7);
    board.PlacePiece(BLACKKNIGHT, d2);
    board.PlacePiece(BLACKKNIGHT, f3);
    board.PlacePiece(WHITEKING, e1);
    board.PlacePiece(WHITEPAWN, g2);
    
    // do 
    auto result = moveGenerator.GeneratePossibleMoves(testContext);

    EXPECT_EQ(2, result.size());
}

// 8 [   ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][ P ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][   ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
TEST_F(MoveGeneratorFixture, BlackCaptureFromCheck)
{
    // setup
    testContext.editToPlay() = Set::BLACK;
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(WHITEPAWN, f7);
    
    // do 
    auto result = moveGenerator.GeneratePossibleMoves(testContext);

    // verify
    for (auto&& move : result)
    {
        if (move.TargetSquare == f7)
        {
            EXPECT_EQ(move.Flags, MoveFlag::Capture);
            break;
        }
    }
    
    auto count =  moveGenerator.CountMoves(result);
    EXPECT_EQ(5, count.Moves);
    EXPECT_EQ(1, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);
}


// 8 [   ][   ][   ][   ][   ][   ][ b ][   ]
// 7 [   ][   ][   ][   ][   ][ x ][   ][ x ]
// 6 [   ][   ][   ][   ][ x ][   ][   ][   ]
// 5 [   ][   ][   ][ x ][   ][   ][   ][   ]
// 4 [   ][   ][ x ][   ][   ][   ][   ][   ]
// 3 [   ][ x ][   ][   ][   ][   ][   ][   ]
// 2 [ x ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][   ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
TEST_F(MoveGeneratorFixture, BlackBishop)
{
    // setup
    testContext.editToPlay() = Set::BLACK;
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKBISHOP, g8);
    
    // do 
    auto result = moveGenerator.GeneratePossibleMoves(testContext);

    // verify
    auto count =  moveGenerator.CountMoves(result);
    EXPECT_EQ(7, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);
}

// 8 [   ][   ][   ][   ][ k ][   ][ b ][   ]
// 7 [   ][   ][   ][   ][   ][ P ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][   ][ R ][   ][   ]
//     A    B    C    D    E    F    G    H
// valid moves:
// -- Bxf7
// Only valid move for bishop is to capture the pawn.
TEST_F(MoveGeneratorFixture, BlackBishopOnlyHasOneMove)
{
    // setup
    testContext.editToPlay() = Set::BLACK;
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKBISHOP, g8);
    board.PlacePiece(WHITEPAWN, f7);
    board.PlacePiece(WHITEROOK, f1);
    
    // do 
    auto result = moveGenerator.GeneratePossibleMoves(testContext);

    MoveCount::Predicate predicate = [](const Move& mv) 
    {
        static ChessPiece b = BLACKBISHOP;
        if (mv.Piece == b)
            return true;
        
        return false;
    };
    // verify
    auto count =  moveGenerator.CountMoves(result, predicate);
    EXPECT_EQ(1, count.Moves);
    EXPECT_EQ(1, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);

    count =  moveGenerator.CountMoves(result);
    EXPECT_EQ(5, count.Moves);
    EXPECT_EQ(1, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);
}


// 8 [   ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][ b ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][ R ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// valid moves:

TEST_F(MoveGeneratorFixture, BlackBishopNoValidMoves)
{
    // setup
    testContext.editToPlay() = Set::BLACK;
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKBISHOP, e5);
    board.PlacePiece(WHITEROOK, e1);
    
    // do 
    auto result = moveGenerator.GeneratePossibleMoves(testContext);

    MoveCount::Predicate predicate = [](const Move& mv) 
    {
        static ChessPiece b = BLACKBISHOP;
        if (mv.Piece == b)
            return true;
        
        return false;
    };
    // verify
    auto count =  moveGenerator.CountMoves(result, predicate);
    EXPECT_EQ(0, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);

    count =  moveGenerator.CountMoves(result);
    EXPECT_EQ(5, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);
}

// 8 [   ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][ b ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][ N ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][ R ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// found an edge case where a pinned piece would be allowed to capture
// a different piece than the one pinning it.
TEST_F(MoveGeneratorFixture, BlackBishopNoValidMoves_ThreateningAPiece)
{
    // setup
    testContext.editToPlay() = Set::BLACK;
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKBISHOP, e5);
    board.PlacePiece(WHITEROOK, e1);
    board.PlacePiece(WHITEKNIGHT, g3);
    
    // do 
    auto result = moveGenerator.GeneratePossibleMoves(testContext);

    MoveCount::Predicate predicate = [](const Move& mv) 
    {
        static ChessPiece b = BLACKBISHOP;
        if (mv.Piece == b)
            return true;
        
        return false;
    };
    // verify
    auto count =  moveGenerator.CountMoves(result, predicate);
    EXPECT_EQ(0, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);

    count =  moveGenerator.CountMoves(result);
    EXPECT_EQ(5, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);
}

// 8 [   ][   ][   ][   ][   ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][ P ][ P ][ P ][   ][   ][   ]
// 1 [   ][   ][   ][ Q ][   ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// valid moves:

TEST_F(MoveGeneratorFixture, WhiteQueenBlockedByPawns)
{
    // setup
    testContext.editToPlay() = Set::WHITE;
    auto& board = testContext.editChessboard();
    board.PlacePiece(WHITEQUEEN, d1);
    board.PlacePiece(WHITEPAWN, c2);
    board.PlacePiece(WHITEPAWN, d2);
    board.PlacePiece(WHITEPAWN, e2);

    // do 
    auto result = moveGenerator.GeneratePossibleMoves(testContext);

    MoveCount::Predicate predicate = [](const Move& mv) 
    {
        static ChessPiece Q = WHITEQUEEN;
        if (mv.Piece == Q)
            return true;
        
        return false;
    };
    // verify
    auto count =  moveGenerator.CountMoves(result, predicate);
    EXPECT_EQ(7, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);

    MoveCount::Predicate pawnPredicate = [](const Move& mv) 
    {
        static ChessPiece P = WHITEPAWN;
        if (mv.Piece == P)
            return true;
        
        return false;
    };

    count =  moveGenerator.CountMoves(result, pawnPredicate);
    EXPECT_EQ(6, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);

    count =  moveGenerator.CountMoves(result);
    EXPECT_EQ(13, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);
}





} // namespace ElephantTest