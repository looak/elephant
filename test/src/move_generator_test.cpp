#include <gtest/gtest.h>

#include "elephant_test_utils.h"
#include "fen_parser.h"
#include "game_context.h"
#include "move_generator.hpp"
#include "search.h"

namespace ElephantTest {
////////////////////////////////////////////////////////////////
/**
 * @file move_generator_test.cpp
 * @brief Fixture for testing move generator.
 * Naming convention as of May 2023: <TestedFunctionality>_<TestedColor>_<ExpectedResult>
 * @author Alexander Loodin Ek
 */
class MoveGeneratorFixture : public ::testing::Test {
public:
    virtual void SetUp(){

    };
    virtual void TearDown(){};

    /**
     * @brief Build a vector of moves from a move generator.
     * Since historically, move generator was recieving a vector of moves, but we changed
     * move gen to generate "next move", we need to build a vector of moves from the generator
     * for backwards compatability.
     * @param gen Move generator to build from.
     * @return Vector of moves.     */
    std::vector<PackedMove> buildMoveVector(MoveGenerator& gen) const
    {
        std::vector<PackedMove> result;
        while (auto mv = gen.generateNextMove() != PackedMove::NullMove()) {
            result.push_back(mv);
        }
        return result;
    }

    Search search;
    GameContext testContext;
};
////////////////////////////////////////////////////////////////

TEST_F(MoveGeneratorFixture, Empty)
{
    MoveGenerator gen(testContext);
    PackedMove move = gen.generateNextMove();
    EXPECT_EQ(0, move.read());
}

#pragma region KingMoveGenerationTests
//{ King move generation tests
/** Most basic move generation test, a king in the middle of the board with no other pieces,
 * should have eight moves available. */
TEST_F(MoveGeneratorFixture, KingFromE4_White_EightMovesAvailable)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(WHITEKING, e4);
    MoveGenerator gen(testContext);

    auto result = buildMoveVector(gen);
    EXPECT_EQ(8, result.size());
}

TEST_F(MoveGeneratorFixture, KingFromE1_White_FiveMovesAvailable)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(WHITEKING, e1);
    MoveGenerator gen(testContext);

    auto result = buildMoveVector(gen);
    EXPECT_EQ(5, result.size());
}

TEST_F(MoveGeneratorFixture, KingFromE8_Black_FiveMovesAvailable)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    testContext.editToPlay() = Set::BLACK;

    MoveGenerator gen(testContext);

    auto result = buildMoveVector(gen);
    EXPECT_EQ(5, result.size());
}

TEST_F(MoveGeneratorFixture, KingAndPawn_White_PawnBlocksOneMoveOfKingButHasDoublePushAvailable)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(WHITEKING, e1);
    board.PlacePiece(WHITEPAWN, e2);

    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(6, result.size());
}

/** setting up this position which avoids king being in check, but we should still be able to capture
 * the knight on d7 but we can't move to f8 since that'd put us in check   */
TEST_F(MoveGeneratorFixture, KingMoveGeneration_Black_KingCanCaptureOpponentKnight)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(WHITEKNIGHT, d7);

    testContext.editToPlay() = Set::BLACK;

    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(4, result.size());
}
// }
#pragma endregion  // KingMoveGenerationTests

#pragma region PawnMoveGenerationTests
/** Pawn tests todo
 * [x] Pawn can move forward
 * [ ] Pawn can capture diagonally
 * [x] Pawn can move two squares on first move
 * [x] Pawn can not move two squares on second move
 * [ ] Pawn can not move forward if blocked
 * [ ] Pawn can capture diagonally if blocked
 * [ ] Pawn can capture enpassant
 * [ ] Pawn can not capture enpassant if not enpassantable
 * [ ] Pawn can block check
 * [ ] Pawn can't move or double move if it puts king in check
 * [ ] Pawn can capture checking piece
 * [ ] Pawn can not block check if it puts king in check
 * [ ] Pawn can not capture enpassant if it puts king in check !!! */

TEST_F(MoveGeneratorFixture, PawnBasicMoves_WhiteAndBlack_NothingBlockedNoCaptures)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(WHITEPAWN, e2);
    board.PlacePiece(WHITEPAWN, h2);
    board.PlacePiece(WHITEPAWN, f3);
    board.PlacePiece(WHITEPAWN, b6);
    board.PlacePiece(BLACKPAWN, a7);
    board.PlacePiece(BLACKPAWN, c6);

    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(6, result.size());

    testContext.editToPlay() = Set::BLACK;
    MoveGenerator gen2(testContext);
    auto result2 = buildMoveVector(gen2);

    EXPECT_EQ(3, result2.size());
}

TEST_F(MoveGeneratorFixture, PawnBasicMoves_White_BlockedPiecesCanNotMoveForward)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(WHITEPAWN, b6);  // not blocked accounts for 1 move
    board.PlacePiece(WHITEPAWN, e2);  // blocked by white pawn on e3
    board.PlacePiece(WHITEPAWN, e3);  // blocking e2 and acounts for 1 move
    board.PlacePiece(WHITEPAWN, f2);  // not blocked accounts for 2 moves
    board.PlacePiece(WHITEPAWN, g5);  // blocked by black knight
    board.PlacePiece(WHITEPAWN, g7);  // not blocked accounts for 1 move
    board.PlacePiece(WHITEPAWN, h2);  // blocked by black pawn on h3

    board.PlacePiece(BLACKKNIGHT, g6);  // blocking g5 but not g7
    board.PlacePiece(BLACKPAWN, h3);    // blocking h2

    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(5, result.size());
}

#pragma endregion  // PawnMoveGenerationTests

#pragma region KnightMoveGenerationTests
TEST_F(MoveGeneratorFixture, KnightMoveGeneration_White_OneCaptureNonBlocked)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(WHITEKNIGHT, e4);
    board.PlacePiece(BLACKKNIGHT, f6);

    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(8, result.size());
}

TEST_F(MoveGeneratorFixture, KnightsInAllCorner_White_EightAvailableMoves)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(WHITEKNIGHT, a1);
    {  // testing a1, should only have two moves available

        MoveGenerator gen(testContext);
        auto result = buildMoveVector(gen);

        EXPECT_EQ(2, result.size());
    }
    board.PlacePiece(WHITEKNIGHT, h1);
    board.PlacePiece(WHITEKNIGHT, a8);
    board.PlacePiece(WHITEKNIGHT, h8);

    {  // testing all corner, there should be eight moves available
        MoveGenerator gen(testContext);
        auto result = buildMoveVector(gen);

        EXPECT_EQ(8, result.size());
    }
}
#pragma endregion  // KnightMoveGenerationTests

TEST_F(MoveGeneratorFixture, PawnPromotion)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKPAWN, a2);
    testContext.editToPlay() = Set::BLACK;

    // do
    auto result = search.GeneratePossibleMoves(testContext);

    // verify
    EXPECT_EQ(4, result.size());
    for (auto&& move : result) {
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
    auto result = search.GeneratePossibleMoves(testContext);

    // verify
    EXPECT_EQ(8, result.size());
    for (auto&& move : result) {
        if (move.TargetSquare == b1) {
            EXPECT_EQ(MoveFlag::Capture, move.Flags & MoveFlag::Capture);
        }
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
// TEST_F(MoveGeneratorFixture, PawnPromotionCaptureCheck)
// {
//     // setup
//     auto& board = testContext.editChessboard();
//     board.PlacePiece(BLACKPAWN, a2);
//     board.PlacePiece(WHITEROOK, b1);
//     board.PlacePiece(WHITEKING, g1);
//     testContext.editToPlay() = Set::BLACK;

//     // do
//     auto result = search.GeneratePossibleMoves(testContext);

//     // verify
//     EXPECT_EQ(8, result.size());
//     for (auto&& move : result)
//     {
//         if (move.TargetSquare == b1)
//         {
//             EXPECT_EQ(MoveFlag::Capture, move.Flags & MoveFlag::Capture);
//             if (move.PromoteToPiece == BLACKQUEEN)
//                 EXPECT_EQ(MoveFlag::Check, move.Flags & MoveFlag::Check);
//             else if (move.PromoteToPiece == BLACKROOK)
//                 EXPECT_EQ(MoveFlag::Check, move.Flags & MoveFlag::Check);
//             else
//                 EXPECT_NE(MoveFlag::Check, move.Flags & MoveFlag::Check);
//         }
//         else
//             EXPECT_NE(MoveFlag::Check, move.Flags & MoveFlag::Check);
//         EXPECT_EQ(MoveFlag::Promotion, move.Flags & MoveFlag::Promotion);
//     }

//     auto count = CountMoves(result);
//     EXPECT_EQ(8, count.Moves);
//     EXPECT_EQ(4, count.Captures);
//     EXPECT_EQ(0, count.EnPassants);
//     EXPECT_EQ(8, count.Promotions);
//     EXPECT_EQ(0, count.Castles);
//     EXPECT_EQ(2, count.Checks);
//     EXPECT_EQ(0, count.Checkmates);
// }

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
    auto result = search.GeneratePossibleMoves(testContext);

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
    auto result = search.GeneratePossibleMoves(testContext);

    EXPECT_EQ(3, result.size());
}

// 8 [ r ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][ N ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][ K ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// valid king moves:
// d1, d2, e2, f1
// can not castle since we are in check.
TEST_F(MoveGeneratorFixture, KingCanNotCastleWhileInCheck)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKROOK, a8);
    board.PlacePiece(WHITEKNIGHT, d6);
    board.PlacePiece(WHITEKING, e1);
    board.setCastlingState(8);

    testContext.editToPlay() = Set::BLACK;

    // do
    auto result = search.GeneratePossibleMoves(testContext);

    EXPECT_EQ(4, result.size());

    // count king moves
    MoveCount::Predicate predicate = [](const Move& mv) {
        static ChessPiece b = BLACKKING;
        if (mv.Piece == b)
            return true;

        return false;
    };
    auto count = CountMoves(result, predicate);
    EXPECT_EQ(4, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);
}

// 8 [ r ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][ N ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][ K ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// valid king moves:
// d1, d2, e2, f1
// can not castle since we are in check.
TEST_F(MoveGeneratorFixture, KingCanNotCastleBecauseItsBlockedByKnight)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKROOK, a8);
    board.PlacePiece(WHITEKNIGHT, c6);
    board.PlacePiece(WHITEKING, e1);
    board.setCastlingState(8);

    testContext.editToPlay() = Set::BLACK;

    // do
    auto result = search.GeneratePossibleMoves(testContext);

    // count king moves
    MoveCount::Predicate predicate = [](const Move& mv) {
        static ChessPiece b = BLACKKING;
        if (mv.Piece == b)
            return true;

        return false;
    };
    auto count = CountMoves(result, predicate);
    EXPECT_EQ(3, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);
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
    auto result = search.GeneratePossibleMoves(testContext);

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
    auto result = search.GeneratePossibleMoves(testContext);

    EXPECT_EQ(2, result.size());
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
TEST_F(MoveGeneratorFixture, KingCanNotMoveIntoCheck)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKROOK, d8);
    board.PlacePiece(WHITEKING, e1);

    // do
    auto result = search.GeneratePossibleMoves(testContext);

    EXPECT_EQ(3, result.size());
}

// 8 [   ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][ n ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][ K ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// valid moves:
// d2, e2, f2
TEST_F(MoveGeneratorFixture, KingCanNotMoveIntoCheck_KnightVariation)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKKNIGHT, e3);
    board.PlacePiece(WHITEKING, e1);

    // do
    auto result = search.GeneratePossibleMoves(testContext);

    EXPECT_EQ(3, result.size());
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
    auto result = search.GeneratePossibleMoves(testContext);

    // verify
    for (auto&& move : result) {
        if (move.TargetSquare == f7) {
            testContext.MakeMove(move);
            EXPECT_EQ(move.Flags, MoveFlag::Capture);
            break;
        }
    }

    auto count = CountMoves(result);
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
    auto result = search.GeneratePossibleMoves(testContext);

    // verify
    auto count = CountMoves(result);
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
    auto result = search.GeneratePossibleMoves(testContext);

    for (auto& move : result) {
        testContext.MakeMove(move);
        testContext.UnmakeMove(move);
    }

    MoveCount::Predicate predicate = [](const Move& mv) {
        static ChessPiece b = BLACKBISHOP;
        if (mv.Piece == b)
            return true;

        return false;
    };
    // verify
    auto count = CountMoves(result, predicate);
    EXPECT_EQ(1, count.Moves);
    EXPECT_EQ(1, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);

    count = CountMoves(result);
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
    auto result = search.GeneratePossibleMoves(testContext);

    MoveCount::Predicate predicate = [](const Move& mv) {
        static ChessPiece b = BLACKBISHOP;
        if (mv.Piece == b)
            return true;

        return false;
    };
    // verify
    auto count = CountMoves(result, predicate);
    EXPECT_EQ(0, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);

    count = CountMoves(result);
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
    auto result = search.GeneratePossibleMoves(testContext);

    MoveCount::Predicate predicate = [](const Move& mv) {
        static ChessPiece b = BLACKBISHOP;
        if (mv.Piece == b)
            return true;

        return false;
    };
    // verify
    auto count = CountMoves(result, predicate);
    EXPECT_EQ(0, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);

    count = CountMoves(result);
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
    auto result = search.GeneratePossibleMoves(testContext);

    MoveCount::Predicate predicate = [](const Move& mv) {
        static ChessPiece Q = WHITEQUEEN;
        if (mv.Piece == Q)
            return true;

        return false;
    };
    // verify
    auto count = CountMoves(result, predicate);
    EXPECT_EQ(7, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);

    MoveCount::Predicate pawnPredicate = [](const Move& mv) {
        static ChessPiece P = WHITEPAWN;
        if (mv.Piece == P)
            return true;

        return false;
    };

    count = CountMoves(result, pawnPredicate);
    EXPECT_EQ(6, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);

    count = CountMoves(result);
    EXPECT_EQ(13, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);
}

// 8 [   ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][ b ][   ][   ][   ][ n ][ b ][ N ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][ P ][   ]
// 1 [   ][   ][   ][   ][   ][   ][ K ][   ]
//     A    B    C    D    E    F    G    H
// valid moves:
TEST_F(MoveGeneratorFixture, OnlyValidMoveIsKing)
{
    // setup
    testContext.editToPlay() = Set::WHITE;
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKBISHOP, b6);
    board.PlacePiece(BLACKBISHOP, g6);
    board.PlacePiece(BLACKKNIGHT, f6);
    board.PlacePiece(WHITEKNIGHT, h6);
    board.PlacePiece(WHITEPAWN, g2);
    board.PlacePiece(WHITEKING, g1);

    // do
    bool checked = board.isChecked(Set::WHITE);
    auto moves = search.GeneratePossibleMoves(testContext);

    // verify
    EXPECT_TRUE(checked);
    auto count = CountMoves(moves);
    EXPECT_EQ(3, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);
}

// 8 [   ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][ b ][   ][   ][   ][ n ][ r ][ N ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][ P ][   ]
// 1 [   ][   ][   ][   ][   ][   ][ K ][   ]
//     A    B    C    D    E    F    G    H
// valid moves:
TEST_F(MoveGeneratorFixture, OnlyValidMoveIsKing_RookVarient)
{
    // setup
    testContext.editToPlay() = Set::WHITE;
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKBISHOP, b6);
    board.PlacePiece(BLACKROOK, g6);
    board.PlacePiece(BLACKKNIGHT, f6);
    board.PlacePiece(WHITEKNIGHT, h6);
    board.PlacePiece(WHITEPAWN, g2);
    board.PlacePiece(WHITEKING, g1);

    // do
    bool checked = board.isChecked(Set::WHITE);
    auto moves = search.GeneratePossibleMoves(testContext);

    // verify
    EXPECT_TRUE(checked);
    auto count = CountMoves(moves);
    EXPECT_EQ(3, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);
}

// 8 [   ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][ r ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][ P ][   ]
// 1 [   ][   ][   ][   ][   ][   ][ K ][   ]
//     A    B    C    D    E    F    G    H
// valid moves:
TEST_F(MoveGeneratorFixture, PawnShouldHaveTwoMoves)
{
    // setup
    testContext.editToPlay() = Set::WHITE;
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    // board.PlacePiece(BLACKBISHOP, b6);
    board.PlacePiece(BLACKROOK, g6);
    board.PlacePiece(WHITEPAWN, g2);
    board.PlacePiece(WHITEKING, g1);

    // do
    bool checked = board.isChecked(Set::WHITE);

    // verify
    EXPECT_FALSE(checked);

    // do
    auto moves = search.GeneratePossibleMoves(testContext);

    // verify
    MoveCount::Predicate predicate = [](const Move& mv) {
        static ChessPiece P = WHITEPAWN;
        if (mv.Piece == P)
            return true;

        return false;
    };

    auto count = CountMoves(moves, predicate);
    EXPECT_EQ(2, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);
}

// 8 [   ][   ][   ][   ][   ][   ][   ][   ]
// 7 [   ][   ][   ][ B ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][ p ][   ][   ][   ][   ][   ][   ]
// 4 [ k ][   ][   ][   ][   ][ p ][   ][ R ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][ P ][   ][   ][   ]
// 1 [   ][   ][   ][   ][   ][ K ][   ][   ]
//     A    B    C    D    E    F    G    H
// sequence of moves: e4 fxe3 is illegal because it puts
// king in check.
TEST_F(MoveGeneratorFixture, PinnedPawn_Black_CanNotCaptureEnPassant)
{
    // setup
    testContext.editToPlay() = Set::BLACK;
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, a4);
    board.PlacePiece(BLACKPAWN, f4);
    board.PlacePiece(BLACKPAWN, b5);
    board.PlacePiece(WHITEPAWN, e2);
    board.PlacePiece(WHITEROOK, h4);
    board.PlacePiece(WHITEKING, f1);
    board.PlacePiece(WHITEBISHOP, d7);

    // move pawn to e4
    Move move(e2, e4);
    board.MakeMove(move);

    // do
    auto moves = search.GeneratePossibleMoves(testContext);

    // b5 thinks in can move to b4 because both of those are in the pinned mask i.e. legal moves.
    // need to seperate my pinned masks so a piece can't move from one pin to another.

    // verify
    MoveCount::Predicate predicate = [](const Move& mv) {
        static ChessPiece P = BLACKPAWN;
        if (mv.Piece == P)
            return true;

        return false;
    };

    auto count = CountMoves(moves, predicate);
    EXPECT_EQ(1, count.Moves);
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
// 4 [ k ][   ][   ][   ][   ][ p ][   ][ R ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][ P ][   ]
// 1 [   ][   ][   ][   ][   ][ K ][   ][   ]
//     A    B    C    D    E    F    G    H
// sequence of moves: g4 fxe3 is illegal because it puts
// king in check.
TEST_F(MoveGeneratorFixture, PinnedPawnEnPassant_g2)
{
    // setup
    testContext.editToPlay() = Set::BLACK;
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, a4);
    board.PlacePiece(BLACKPAWN, f4);
    board.PlacePiece(WHITEPAWN, g2);
    board.PlacePiece(WHITEROOK, h4);
    board.PlacePiece(WHITEKING, f1);

    // move pawn to e4
    Move move(g2, g4);
    board.MakeMove(move);

    // do
    auto moves = search.GeneratePossibleMoves(testContext);

    // verify
    MoveCount::Predicate predicate = [](const Move& mv) {
        static ChessPiece P = BLACKPAWN;
        if (mv.Piece == P)
            return true;

        return false;
    };

    auto count = CountMoves(moves, predicate);
    EXPECT_EQ(1, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);
}

// fen r3k2r/p1pNqpb1/bn2pnp1/3P4/1p2P3/2N2Q1p/PPPBBPPP/R3K2R b KQkq - 0 1
// 8 [ r ][   ][   ][ k ][   ][   ][ r ][   ]
// 7 [ p ][   ][ p ][ N ][ q ][ p ][ b ][   ]
// 6 [ b ][ n ][   ][   ][ p ][ n ][ p ][   ]
// 5 [   ][   ][   ][ P ][   ][   ][   ][   ]
// 4 [   ][ p ][   ][   ][ P ][   ][   ][   ]
// 3 [   ][ N ][   ][   ][   ][ Q ][   ][ p ]
// 2 [ P ][ P ][ P ][ B ][ B ][ P ][ P ][ P ]
// 1 [ R ][   ][   ][   ][ K ][   ][   ][ R ]
//     A    B    C    D    E    F    G    H
// sequence of moves: exd5 and then unmake it

TEST_F(MoveGeneratorFixture, UnmakePawnCapture)
{
    // setup
    std::string fen("r3k2r/p1pNqpb1/bn2pnp1/3P4/1p2P3/2N2Q1p/PPPBBPPP/R3K2R b KQkq - 0 1");
    FENParser::deserialize(fen.c_str(), testContext);
    const auto& blkMat = testContext.readChessboard().readMaterial(Set::BLACK);
    const auto& whtMat = testContext.readChessboard().readMaterial(Set::WHITE);
    u64 orgHash = testContext.readChessboard().readHash();

    // verify
    EXPECT_EQ(BLACKPAWN, testContext.readChessboard().readPieceAt(e6));
    EXPECT_EQ(WHITEPAWN, testContext.readChessboard().readPieceAt(d5));

    EXPECT_TRUE(VerifyListsContainSameNotations({a7, b4, c7, e6, f7, g6, h3}, blkMat.buildPlacementsOfPiece(BLACKPAWN)));
    EXPECT_TRUE(VerifyListsContainSameNotations({a2, b2, c2, d5, e4, f2, g2, h2}, whtMat.buildPlacementsOfPiece(WHITEPAWN)));

    // do
    Move move(e6, d5);  // pawn capture
    bool result = testContext.MakeMove(move);

    // verify
    EXPECT_TRUE(result);
    EXPECT_TRUE(VerifyListsContainSameNotations({a7, b4, c7, d5, f7, g6, h3}, blkMat.buildPlacementsOfPiece(BLACKPAWN)));
    EXPECT_TRUE(VerifyListsContainSameNotations({a2, b2, c2, e4, f2, g2, h2}, whtMat.buildPlacementsOfPiece(WHITEPAWN)));
    EXPECT_EQ(BLACKPAWN, testContext.readChessboard().readPieceAt(d5));
    EXPECT_NE(orgHash, testContext.readChessboard().readHash());
    EXPECT_TRUE(move.isCapture());
    EXPECT_FALSE(move.isEnPassant());

    // unmake
    testContext.UnmakeMove(move);

    // verify
    EXPECT_EQ(BLACKPAWN, testContext.readChessboard().readPieceAt(e6));
    EXPECT_EQ(WHITEPAWN, testContext.readChessboard().readPieceAt(d5));

    EXPECT_TRUE(VerifyListsContainSameNotations({a7, b4, c7, e6, f7, g6, h3}, blkMat.buildPlacementsOfPiece(BLACKPAWN)));
    EXPECT_TRUE(VerifyListsContainSameNotations({a2, b2, c2, d5, e4, f2, g2, h2}, whtMat.buildPlacementsOfPiece(WHITEPAWN)));
    EXPECT_EQ(orgHash, testContext.readChessboard().readHash());
}

TEST_F(MoveGeneratorFixture, KnightMovements)
{
    // setup
    testContext.editToPlay() = Set::BLACK;
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, a4);
    board.PlacePiece(BLACKKNIGHT, e6);
    board.PlacePiece(WHITEKING, f1);

    // do
    auto moves = search.GeneratePossibleMoves(testContext);

    // verify
    MoveCount::Predicate predicate = [](const Move& mv) {
        static ChessPiece P = BLACKKNIGHT;
        if (mv.Piece == P)
            return true;

        return false;
    };

    auto count = CountMoves(moves, predicate);
    EXPECT_EQ(8, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);
}

TEST_F(MoveGeneratorFixture, ScholarsMate)
{
    // setup
    std::string fen = "r1bqkbnr/ppp2Qpp/2np4/4p3/2B1P3/8/PPPP1PPP/RNB1K1NR b KQkq - 0 4";
    FENParser::deserialize(fen.c_str(), testContext);

    // verify
    EXPECT_TRUE(testContext.readChessboard().isCheckmated(Set::BLACK));
}

TEST_F(MoveGeneratorFixture, ScholarsMateQueenMovesIntoMate)
{
    // setup
    std::string fen = "r1bqkbnr/ppp2ppp/2np4/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 2 4";
    FENParser::deserialize(fen.c_str(), testContext);

    // do
    auto moves = search.GeneratePossibleMoves(testContext);

    for (auto& mv : moves) {
        testContext.MakeMove(mv);
        testContext.UnmakeMove(mv);
    }

    // verify
    MoveCount::Predicate predicate = [](const Move& mv) {
        static ChessPiece P = WHITEQUEEN;
        if (mv.Piece == P)
            return true;

        return false;
    };

    auto count = CountMoves(moves, predicate);
    EXPECT_EQ(13, count.Moves);
    EXPECT_EQ(3, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(2, count.Checks);
    EXPECT_EQ(1, count.Checkmates);
}

// 8 [ r ][   ][   ][ k ][   ][   ][   ][ r ]
// 7 [   ][ b ][   ][   ][   ][   ][ b ][ q ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][ B ]
// 1 [ R ][   ][   ][   ][ K ][   ][   ][ R ]
//     A    B    C    D    E    F    G    H
// r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1
// after Rxa8 bishop should have two available moves
TEST_F(MoveGeneratorFixture, BishopBlockingOrCapturingCheckingPiece)
{
    std::string fen = "r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1";
    FENParser::deserialize(fen.c_str(), testContext);

    // setup
    Move move(a1, a8);
    // this result can be true when no move was made.
    bool result = testContext.MakeMove(move);

    // verify
    EXPECT_TRUE(result);
    EXPECT_EQ(WHITEROOK, testContext.readChessboard().readPieceAt(a8));
    EXPECT_TRUE(testContext.readChessboard().isChecked(testContext.readToPlay()));

    auto moves = search.GeneratePossibleMoves(testContext);

    for (auto& mv : moves) {
        testContext.MakeMove(mv);
        testContext.UnmakeMove(mv);
    }

    auto predicate = [](const Move& mv) {
        static ChessPiece b = BLACKBISHOP;
        if (mv.Piece == b)
            return true;

        return false;
    };

    auto count = CountMoves(moves, predicate);
    EXPECT_EQ(2, count.Moves);
    EXPECT_EQ(1, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);
}

TEST_F(MoveGeneratorFixture, MoreCastlingIssues)
{
    // setup
    std::string fen = "1B2k2r/1b4bq/8/8/8/8/r7/R3K2R w KQ - 2 2";
    FENParser::deserialize(fen.c_str(), testContext);

    // verify
    auto moves = search.GeneratePossibleMoves(testContext);

    auto count = CountMoves(moves);
    EXPECT_EQ(23, count.Moves);

    auto predicate = [](const Move& mv) {
        static ChessPiece b = WHITEKING;
        if (mv.Piece == b)
            return true;

        return false;
    };

    count = CountMoves(moves, predicate);
    EXPECT_EQ(4, count.Moves);

    auto predicateRook = [](const Move& mv) {
        static ChessPiece b = WHITEROOK;
        if (mv.Piece == b)
            return true;

        return false;
    };

    count = CountMoves(moves, predicateRook);
    EXPECT_EQ(4 + 8, count.Moves);
}

/**
 * 8 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 7 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 6 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 5 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 4 [   ][ k ][   ][   ][   ][   ][   ][ R ]
 * 3 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 2 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 1 [   ][   ][   ][   ][   ][ K ][   ][   ]
 *     A    B    C    D    E    F    G    H */
TEST_F(MoveGeneratorFixture, KingCheckedByRook)
{
    // setup
    testContext.editToPlay() = Set::BLACK;
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, b4);
    board.PlacePiece(WHITEROOK, h4);
    board.PlacePiece(WHITEKING, f1);

    // do
    auto moves = search.GeneratePossibleMoves(testContext);

    EXPECT_EQ(6, moves.size());
}

/**
 * 8 [   ][   ][   ][ q ][ k ][   ][   ][   ]
 * 7 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 6 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 5 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 4 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 3 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 2 [   ][   ][   ][   ][   ][ P ][ P ][ P ]
 * 1 [   ][   ][   ][ R ][   ][   ][ K ][   ]
 *     A    B    C    D    E    F    G    H */
TEST_F(MoveGeneratorFixture, Checkmate_NoMoreMoves)
{
    // setup
    std::string fen("3qk3/8/8/8/8/8/5PPP/3R2K1 b - - 0 1");
    FENParser::deserialize(fen.c_str(), testContext);

    Move Qxd1(d8, d1);
    testContext.MakeMove(Qxd1);

    // do
    auto moves = search.GeneratePossibleMoves(testContext);
    EXPECT_EQ(0, moves.size());
}
/**
 * 8 [   ][   ][ k ][   ][   ][   ][   ][   ]
 * 7 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 6 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 5 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 4 [   ][   ][   ][ p ][   ][   ][ p ][   ]
 * 3 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 2 [   ][   ][ P ][   ][   ][ P ][   ][   ]
 * 1 [   ][   ][ K ][   ][   ][   ][   ][   ]
 *     A    B    C    D    E    F    G    H */
TEST_F(MoveGeneratorFixture, EnPassantMoves_Both_UndoCaptureAndUndo)
{
    // setup
    std::string fen("2k5/8/8/8/3p2p1/8/2P2P2/2K5 w - - 0 1");

    FENParser::deserialize(fen.c_str(), testContext);

    search.Perft(testContext, 3);
}

/**
* 8 [   ][   ][   ][   ][   ][   ][   ][   ]
* 7 [   ][   ][   ][   ][   ][   ][   ][   ]
* 6 [   ][   ][   ][   ][   ][   ][   ][   ]
* 5 [   ][   ][   ][ k ][   ][   ][   ][   ]
* 4 [   ][   ][   ][ p ][ P ][   ][   ][   ]
* 3 [   ][   ][   ][   ][   ][   ][   ][   ]
* 2 [   ][   ][   ][   ][   ][   ][   ][   ]
* 1 [   ][ K ][   ][ R ][   ][   ][ B ][   ]
*     A    B    C    D    E    F    G    H
8/8/8/3k4/3pP3/8/8/1K1R2B1 b - e3 0 1
Pawn is pinned by white rook in this scenario*/
TEST_F(MoveGeneratorFixture, PawnDoubleMoveCheck_Black_EnPassantCaptureNotAvailableBecauseOfPin)
{
    // setup
    std::string fen("8/8/8/3k4/3pP3/8/8/1K1R2B1 b - e3 0 1");
    FENParser::deserialize(fen.c_str(), testContext);

    // do
    auto moves = search.GeneratePossibleMoves(testContext);

    EXPECT_EQ(7, moves.size());
}

/**
 * 8 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 7 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 6 [   ][ k ][   ][   ][   ][   ][   ][   ]
 * 5 [   ][   ][ b ][   ][   ][   ][   ][   ]
 * 4 [   ][   ][   ][ P ][   ][   ][   ][   ]
 * 3 [   ][   ][ p ][   ][   ][ K ][   ][   ]
 * 2 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 1 [   ][   ][   ][   ][   ][   ][   ][   ]
 *     A    B    C    D    E    F    G    H
 * fen: 8/8/1k6/2b5/3P4/2p2K2/8/8 b - - 2 2*/
TEST_F(MoveGeneratorFixture, PawnMoveC3C2_Black_MoveSuccessfull)
{
    // setup
    std::string fen("8/8/1k6/2b5/3P4/2p2K2/8/8 b - - 2 2");
    FENParser::deserialize(fen.c_str(), testContext);

    // do
    auto moves = search.GeneratePossibleMoves(testContext);

    auto pawnMv = std::find_if(moves.begin(), moves.end(), [](const Move& mv) { return mv.TargetSquare == c2; });
    ASSERT_NE(moves.end(), pawnMv);
    EXPECT_EQ(BLACKPAWN, pawnMv->Piece);
    EXPECT_EQ(c3, pawnMv->SourceSquare);
}

/**
 * 8 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 7 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 6 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 5 [   ][   ][   ][ k ][   ][   ][   ][   ]
 * 4 [   ][   ][   ][ n ][   ][   ][   ][   ]
 * 3 [   ][ B ][   ][   ][   ][   ][   ][   ]
 * 2 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 1 [   ][ K ][   ][ R ][   ][   ][   ][   ]
 *     A    B    C    D    E    F    G    H
 * fen: 8/8/8/3k4/3n4/1B6/8/1K1R4 b - - 0 1*/
TEST_F(MoveGeneratorFixture, Nxb3_Black_IllegalMoveSincePinned)
{
    std::string fen("8/8/8/3k4/3n4/1B6/8/1K1R4 b - - 0 1");
    FENParser::deserialize(fen.c_str(), testContext);

    // do
    auto moves = search.GeneratePossibleMoves(testContext);

    auto knightMv = std::find_if(moves.begin(), moves.end(), [](const Move& mv) { return mv.TargetSquare == b3; });
    EXPECT_EQ(moves.end(), knightMv);
    EXPECT_EQ(5, moves.size());
}

/**
 * 8  [ ][ ][ ][ ][ ][ ][ ][ ]
 * 7  [K][ ][ ][ ][ ][ ][ ][r]
 * 6  [ ][ ][ ][p][ ][ ][ ][ ]
 * 5  [ ][P][p][ ][ ][ ][ ][ ]
 * 4  [ ][R][ ][ ][ ][p][ ][k]
 * 3  [ ][ ][ ][ ][ ][ ][ ][ ]
 * 2  [ ][ ][ ][ ][P][ ][P][ ]
 * 1  [ ][ ][ ][ ][ ][ ][ ][ ]
 *     A  B  C  D  E  F  G  H
 * fen:  8/K6r/3p4/1Pp5/1R3p1k/8/4P1P1/8 w - c6 4 3    */
TEST_F(MoveGeneratorFixture, KingInCheck_White_SpecialCasedPawnMovementsNotAvailable)
{
    std::string fen("8/K6r/3p4/1Pp5/1R3p1k/8/4P1P1/8 w - c6 4 3");
    FENParser::deserialize(fen.c_str(), testContext);

    // do
    auto moves = search.GeneratePossibleMoves(testContext);

    EXPECT_EQ(4, moves.size());
}

}  // namespace ElephantTest