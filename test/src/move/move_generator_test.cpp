#include <gtest/gtest.h>

#include "elephant_test_utils.h"
#include <io/fen_parser.hpp>
#include <core/game_context.hpp>
#include <move/generation/move_generator.hpp>
#include "search.hpp"

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

    using MovePredicate = std::function<bool(const PackedMove&)>;

    /**
     * @brief Build a vector of moves from a move generator.
     * Since historically, move generator was recieving a vector of moves, but we changed
     * move gen to generate "next move", we need to build a vector of moves from the generator
     * for backwards compatability.
     * @param gen Move generator to build from.
     * @param predicate Predicate to filter moves.
     * @return Vector of moves.     */
    std::vector<PackedMove> buildMoveVector(MoveGenerator& gen, MovePredicate pred = nullptr) const
    {
        std::vector<PackedMove> result;
        while (auto mv = gen.generateNextMove().move) {
            if (pred && !pred(mv))
                continue;

            result.push_back(mv);
        }

        return result;
    }

    Chessboard board;

    Search search;
    GameContext testContext;
};
////////////////////////////////////////////////////////////////

// move generator asserts if the board is empty
TEST_F(MoveGeneratorFixture, Empty)
{
    MoveGenerator gen(testContext);
    PackedMove move = gen.generateNextMove().move;
    EXPECT_EQ(0, move.read());
    EXPECT_EQ(PackedMove::NullMove(), move);
}

//{ King move generation tests
/** Most basic move generation test, a king in the middle of the board with no other pieces,
 * should have eight moves available. */
TEST_F(MoveGeneratorFixture, KingFromE4_White_EightMovesAvailable)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(WHITEKING, e4);
    board.PlacePiece(BLACKKING, e8);
    MoveGenerator gen(testContext);

    auto result = buildMoveVector(gen);
    EXPECT_EQ(8, result.size());
}

TEST_F(MoveGeneratorFixture, KingFromE1_White_FiveMovesAvailable)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(WHITEKING, e1);
    board.PlacePiece(BLACKKING, e8);
    MoveGenerator gen(testContext);

    auto result = buildMoveVector(gen);
    EXPECT_EQ(5, result.size());
}

TEST_F(MoveGeneratorFixture, KingFromE8_Black_FiveMovesAvailable)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(WHITEKING, e1);
    board.setToPlay(Set::BLACK);

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
    board.PlacePiece(BLACKKING, e8);

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
    board.PlacePiece(WHITEKING, e1);

    board.setToPlay(Set::BLACK);

    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(4, result.size());
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
// f1 is blocked by knight
// by rook on d8.
TEST_F(MoveGeneratorFixture, King_GuardedPiece_CanNotCaptureKnight)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKROOK, d8);
    board.PlacePiece(BLACKKNIGHT, d2);
    board.PlacePiece(WHITEKING, e1);

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(3, result.size());
}

// 8 [ r ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][ K ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
TEST_F(MoveGeneratorFixture, King_Castling_QueenSideCastlingAvailable)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKROOK, a8);
    board.PlacePiece(WHITEKING, e1);
    board.setCastlingState(8);

    board.setToPlay(Set::BLACK);

    // do
    MoveGenerator gen(testContext);
    auto predicate = [](const PackedMove& mv) { return mv.sourceSqr() == Square::E8; };
    auto result = buildMoveVector(gen, predicate);

    EXPECT_EQ(6, result.size());

    for (auto& mv : result) {
        if (mv.isCastling()) {
            EXPECT_EQ(Square::C8, mv.targetSqr());
        }
    }
}

// 8 [ r ][   ][   ][   ][ k ][   ][   ][ r ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][ K ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
TEST_F(MoveGeneratorFixture, King_Castling_KingAndQueenSideCastlingAvailable)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKROOK, a8);
    board.PlacePiece(BLACKROOK, h8);
    board.PlacePiece(WHITEKING, e1);
    board.setCastlingState(12);

    board.setToPlay(Set::BLACK);

    // do
    MoveGenerator gen(testContext);
    auto predicate = [](const PackedMove& mv) { return mv.sourceSqr() == Square::E8; };
    auto result = buildMoveVector(gen, predicate);

    EXPECT_EQ(7, result.size());
    int castlingCounter = 0;
    for (auto& mv : result) {
        if (mv.isCastling()) {
            castlingCounter++;
        }
    }
    EXPECT_EQ(2, castlingCounter);
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
TEST_F(MoveGeneratorFixture, King_InCheck_CanNotCastle)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKROOK, a8);
    board.PlacePiece(WHITEKNIGHT, d6);
    board.PlacePiece(WHITEKING, e1);
    board.setCastlingState(8);

    board.setToPlay(Set::BLACK);

    // do
    MoveGenerator gen(testContext);
    auto predicate = [](const PackedMove& mv) { return mv.sourceSqr() == Square::E8; };
    auto result = buildMoveVector(gen, predicate);

    EXPECT_EQ(4, result.size());
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
TEST_F(MoveGeneratorFixture, King_DoubleCheck_OnlyValidMovesAreToMoveKing)
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
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(2, result.size());
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
TEST_F(MoveGeneratorFixture, King_Checked_CanNotCaptureGuardedPiece)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKROOK, d8);
    board.PlacePiece(BLACKKNIGHT, d2);
    board.PlacePiece(BLACKKNIGHT, f3);
    board.PlacePiece(WHITEKING, e1);

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(3, result.size());
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
TEST_F(MoveGeneratorFixture, King_NotChecked_CanNotMoveIntoCheck)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKROOK, d8);
    board.PlacePiece(WHITEKING, e1);

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

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
TEST_F(MoveGeneratorFixture, King_NotChecked_CanNotMoveIntoCheckKnightVariation)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKKNIGHT, e3);
    board.PlacePiece(WHITEKING, e1);

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(3, result.size());
}

// 8 [   ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][ P ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][ K ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
TEST_F(MoveGeneratorFixture, King_Check_CaptureCheckingPawn)
{
    // setup
    auto& board = testContext.editChessboard();
    board.setToPlay(Set::BLACK);
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(WHITEPAWN, f7);
    board.PlacePiece(WHITEKING, e1);

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    // verify
    EXPECT_EQ(5, result.size());
    for (auto& mv : result) {
        if (mv.isCapture()) {
            EXPECT_EQ(Square::F7, mv.targetSqr());
        }
    }
}

// 8 [ r ][   ][   ][   ][ k ][ x ][   ][   ]
// 7 [   ][   ][   ][ x ][   ][ x ][   ][   ]
// 6 [   ][   ][ N ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][ K ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
TEST_F(MoveGeneratorFixture, King_Castling_CanNotQueenSideCastleBecauseItsBlockedByKnight)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKROOK, a8);
    board.PlacePiece(WHITEKNIGHT, c6);
    board.PlacePiece(WHITEKING, e1);
    board.setCastlingState(8);

    board.setToPlay(Set::BLACK);

    // do
    MoveGenerator gen(testContext);
    auto predicate = [](const PackedMove& mv) { return mv.sourceSqr() == Square::E8; };
    auto result = buildMoveVector(gen, predicate);

    EXPECT_EQ(3, result.size());
}

// 8 [ r ][   ][   ][   ][ k ][   ][   ][ r ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][ B ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][ K ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
TEST_F(MoveGeneratorFixture, King_Castling_CanNotCastleInCheck)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKROOK, a8);
    board.PlacePiece(BLACKROOK, h8);
    board.PlacePiece(WHITEBISHOP, b5);
    board.PlacePiece(WHITEKING, e1);
    board.setCastlingState(12);

    board.setToPlay(Set::BLACK);

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(4, result.size());
}

TEST_F(MoveGeneratorFixture, King_Castling_MoreCastlingIssues)
{
    // setup
    std::string fen = "1B2k2r/1b4bq/8/8/8/8/r7/R3K2R w KQ - 2 2";
    FENParser::deserialize(fen.c_str(), testContext);

    // verify
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(23, result.size());
}

/** Pawn tests todo
 * [x] Pawn can move forward
 * [x] Pawn can capture diagonally
 * [x] Pawn can move two squares on first move
 * [x] Pawn can not move two squares on second move
 * [x] Pawn can not move forward if blocked
 * [x] Pawn can capture diagonally if blocked
 * [x] Pawn can capture enpassant
 * [x] Pawn can not capture enpassant if not enpassantable
 * [/] Pawn can block check // tested in position fixture
 * [/] Pawn can't move or double move if it puts king in check // tested in position fixture
 * [/] Pawn can capture checking piece // tested in position fixture
 * [x] Pawn can not block check if it puts king in check
 * [x] Pawn can not capture enpassant if it puts king in check !!! */

/*
 *   +------------------------+
 * 8 | .  .  .  .  k  .  .  . |
 * 7 | p  .  .  .  .  .  .  . |
 * 6 | .  P  p  .  .  .  .  . |
 * 5 | .  .  .  .  .  .  .  . |
 * 4 | .  .  .  .  .  .  .  . |
 * 3 | .  .  .  p  .  P  .  . |
 * 2 | .  p  .  .  P  .  P  . |
 * 1 | .  .  .  .  K  .  .  . |
 *   +------------------------+
 *     a  b  c  d  e  f  g  h     */
TEST_F(MoveGeneratorFixture, PawnBasicMoves_WhiteAndBlack_NothingBlockedSomeCaptures)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(WHITEPAWN, e2);
    board.PlacePiece(WHITEPAWN, h2);
    board.PlacePiece(WHITEPAWN, f3);
    board.PlacePiece(WHITEPAWN, b6);
    board.PlacePiece(BLACKPAWN, a7);
    board.PlacePiece(BLACKPAWN, c6);
    board.PlacePiece(BLACKPAWN, b2);
    board.PlacePiece(BLACKPAWN, d3);
    board.PlacePiece(WHITEKING, e1);
    board.PlacePiece(BLACKKING, e8);

    auto predicate = [](const PackedMove& mv) {
        return !mv.isCapture() && !(mv.sourceSqr() == Square::E1) && !(mv.sourceSqr() == Square::E8);
    };

    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen, predicate);

    EXPECT_EQ(6, result.size());

    board.setToPlay(Set::BLACK);
    MoveGenerator gen2(testContext);
    auto result2 = buildMoveVector(gen2, predicate);

    EXPECT_EQ(8, result2.size());

    auto captures = [&](const PackedMove& mv) { return mv.isCapture(); };
    MoveGenerator gen3(testContext);
    auto whiteCaptureMoves = buildMoveVector(gen3, captures);
    EXPECT_EQ(2, whiteCaptureMoves.size());

    MoveGenerator gen4(testContext);
    auto blackCaptureMoves = buildMoveVector(gen4, captures);
    EXPECT_EQ(2, blackCaptureMoves.size());
}

/*
 *   a b c d e f g h
 * 8 . . . . k . . .
 * 7 . . . . . . P .
 * 6 . P . . . . n .
 * 5 . . . . . . P .
 * 4 . . . . . . . .
 * 3 . . . . P . . p
 * 2 . . . . P P . P
 * 1 . . . . K . . .  */
TEST_F(MoveGeneratorFixture, PawnBasicMoves_White_BlockedPiecesCanNotMoveForward)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(WHITEPAWN, b6);  // not blocked accounts for 1 move
    board.PlacePiece(WHITEPAWN, e2);  // blocked by white pawn on e3
    board.PlacePiece(WHITEPAWN, e3);  // blocking e2 and acounts for 1 move
    board.PlacePiece(WHITEPAWN, f2);  // not blocked accounts for 2 moves
    board.PlacePiece(WHITEPAWN, g5);  // blocked by black knight
    board.PlacePiece(WHITEPAWN, g7);  // not blocked accounts for 4 move's 1 for each promotion
    board.PlacePiece(WHITEPAWN, h2);  // blocked by black pawn on h3

    board.PlacePiece(BLACKKNIGHT, g6);  // blocking g5 but not g7
    board.PlacePiece(BLACKPAWN, h3);    // blocking h2

    board.PlacePiece(WHITEKING, e1);
    board.PlacePiece(BLACKKING, e8);

    auto predicate = [](const PackedMove& mv) { return mv.sourceSqr() != Square::E1 && mv.sourceSqr() != Square::E8; };
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen, predicate);

    EXPECT_EQ(8, result.size());
}

TEST_F(MoveGeneratorFixture, PawnBasicMoves_WhiteAndBlack_SimpleCaptures)
{
    // setup
    std::string fen("8/8/8/5pp1/2Pp1P2/2P1P3/8/8 w - - 0 1");
    FENParser::deserialize(fen.c_str(), testContext);
    testContext.editChessboard().PlacePiece(WHITEKING, e1);
    testContext.editChessboard().PlacePiece(BLACKKING, e8);

    auto predicate = [](const PackedMove& mv) { return mv.isCapture(); };

    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);
    EXPECT_EQ(10, result.size());

    MoveGenerator gen2(testContext);
    auto captures = buildMoveVector(gen2, predicate);
    EXPECT_EQ(3, captures.size());

    testContext.editChessboard().setToPlay(Set::BLACK);
    MoveGenerator gen_black(testContext);
    auto result_black = buildMoveVector(gen_black, predicate);
    EXPECT_EQ(3, result_black.size());
}

TEST_F(MoveGeneratorFixture, PawnPromotion)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(WHITEKING, e1);
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKPAWN, a2);
    board.setToPlay(Set::BLACK);

    // do
    MoveGenerator gen(testContext);
    auto predicate = [&](const PackedMove& mv) { return mv.isPromotion(); };
    auto result = buildMoveVector(gen, predicate);

    bool found[4]{false, false, false, false};
    EXPECT_EQ(4, result.size());
    for (auto&& move : result) {
        EXPECT_TRUE(move.isPromotion());
        i16 indx = move.readPromoteToPieceType() - 2;
        found[indx] = true;
    }
    EXPECT_TRUE(found[0]);  // knight
    EXPECT_TRUE(found[1]);  // bishop
    EXPECT_TRUE(found[2]);  // rook
    EXPECT_TRUE(found[3]);  // queen
}

TEST_F(MoveGeneratorFixture, PawnPromotionCapture)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKPAWN, a2);
    board.PlacePiece(WHITEROOK, b1);
    board.PlacePiece(WHITEKING, e1);
    board.setToPlay(Set::BLACK);

    // do
    MoveGenerator gen(testContext);
    auto predicate = [&](const PackedMove& mv) { return mv.isPromotion(); };
    auto result = buildMoveVector(gen, predicate);

    // verify
    EXPECT_EQ(8, result.size());

    bool found[8] = {false};
    for (auto&& move : result) {
        i16 indx = move.readPromoteToPieceType() - 2 + (4 * (int)move.isCapture());
        EXPECT_TRUE(move.isPromotion());
        if (move.targetSqr() == Square::B1) {
            EXPECT_TRUE(move.isCapture());
        }

        found[indx] = true;
    }

    EXPECT_TRUE(found[0]);  // knight
    EXPECT_TRUE(found[1]);  // bishop
    EXPECT_TRUE(found[2]);  // rook
    EXPECT_TRUE(found[3]);  // queen
    EXPECT_TRUE(found[4]);  // knight capture
    EXPECT_TRUE(found[5]);  // bishop capture
    EXPECT_TRUE(found[6]);  // rook capture
    EXPECT_TRUE(found[7]);  // queen capture
}

TEST_F(MoveGeneratorFixture, KnightMoveGeneration_White_OneCaptureNonBlocked)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(WHITEKNIGHT, e4);
    board.PlacePiece(BLACKKNIGHT, f6);
    board.PlacePiece(WHITEKING, e1);
    board.PlacePiece(BLACKKING, e8);

    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(13, result.size());
}

TEST_F(MoveGeneratorFixture, KnightsInAllCorner_White_EightAvailableMoves)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(WHITEKING, e1);
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(WHITEKNIGHT, a1);
    {  // testing a1, should only have two moves available

        MoveGenerator gen(testContext);
        auto result = buildMoveVector(gen);

        EXPECT_EQ(7, result.size());
    }
    board.PlacePiece(WHITEKNIGHT, h1);
    board.PlacePiece(WHITEKNIGHT, a8);
    board.PlacePiece(WHITEKNIGHT, h8);

    {  // testing all corner, there should be eight moves available
        MoveGenerator gen(testContext);
        auto result = buildMoveVector(gen);

        EXPECT_EQ(13, result.size());
    }
}

// 8 [   ][   ][   ][   ][ k ][   ][   ][   ]
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
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKPAWN, a2);
    board.PlacePiece(WHITEROOK, b1);
    board.PlacePiece(WHITEKING, g1);
    board.setToPlay(Set::BLACK);

    // do
    auto count = search.Perft(testContext, 1);

    // verify
    EXPECT_EQ(13, count.Nodes);
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
TEST_F(MoveGeneratorFixture, King_Threat_CantMoveIntoThreatenedSquares)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(WHITEKING, e1);
    board.PlacePiece(BLACKROOK, d8);

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(3, result.size());
}

// 8 [   ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][ r ][   ][   ][   ][   ]
// 6 [   ][   ][ K ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][   ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
TEST_F(MoveGeneratorFixture, King_Threat_CantCaptureGuardedByKing)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(WHITEKING, c6);
    board.PlacePiece(BLACKROOK, d7);

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(3, result.size());
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
// Only valid move for bishop is to capture the pawn.
TEST_F(MoveGeneratorFixture, Bishop_KingInCheck_BishopOnlyHasOneMove)
{
    // setup
    auto& board = testContext.editChessboard();
    board.setToPlay(Set::BLACK);
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKBISHOP, g8);
    board.PlacePiece(WHITEPAWN, f7);
    board.PlacePiece(WHITEROOK, f1);
    board.PlacePiece(WHITEKING, e1);

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(5, result.size());
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

TEST_F(MoveGeneratorFixture, Bishop_Pinned_NoValidMoves)
{
    // setup
    auto& board = testContext.editChessboard();
    board.setToPlay(Set::BLACK);
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKBISHOP, e5);
    board.PlacePiece(WHITEROOK, e1);
    board.PlacePiece(WHITEKING, d1);

    // do
    auto predicate = [](const PackedMove& mv) { return mv.sourceSqr() == Square::E5; };
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen, predicate);

    EXPECT_EQ(0, result.size());
}

TEST_F(MoveGeneratorFixture, Bishop_Pinned_NotAllowedToMoveFromOnePinToAnother)
{
    // setup
    std::string fen("6k1/8/8/b7/1R6/8/8/q1B1K3 w - - 0 1");
    FENParser::deserialize(fen.c_str(), testContext);

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);
    EXPECT_EQ(5, result.size());
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
TEST_F(MoveGeneratorFixture, Bishop_Pinned_NotAllowedToCapture)
{
    // setup
    auto& board = testContext.editChessboard();
    board.setToPlay(Set::BLACK);
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKBISHOP, e5);
    board.PlacePiece(WHITEROOK, e1);
    board.PlacePiece(WHITEKNIGHT, g3);
    board.PlacePiece(WHITEKING, d1);

    // do
    auto predicate = [](const PackedMove& mv) { return mv.sourceSqr() == Square::E5; };
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen, predicate);

    EXPECT_EQ(0, result.size());
}

// 8 [ R ][   ][   ][   ][   ][   ][   ][ r ]
// 7 [   ][   ][   ][   ][ k ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][ K ][   ][   ][ R ]
//     A    B    C    D    E    F    G    H
// found an edge case where a pinned piece would be allowed to capture
// a different piece than the one pinning it.
TEST_F(MoveGeneratorFixture, Rook_Captures_TwoRooksCanCaptureSamePiece)
{
    // setup
    auto& board = testContext.editChessboard();
    board.setToPlay(Set::WHITE);
    board.PlacePiece(BLACKKING, e7);
    board.PlacePiece(BLACKROOK, h8);

    board.PlacePiece(WHITEKING, e1);
    board.PlacePiece(WHITEROOK, a8);
    board.PlacePiece(WHITEROOK, h1);

    // do
    MoveGenerator gen(testContext.readChessboard().readPosition(), Set::WHITE, PieceType::ROOK, MoveTypes::CAPTURES_ONLY);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(2, result.size());
}

// 8 [   ][   ][   ][   ][   ][ k ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][ b ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][ R ][   ][   ][   ][   ]
// 1 [ q ][ R ][   ][   ][ K ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// found an edge case where a pinned piece would be allowed to capture
// a different piece than the one pinning it.
TEST_F(MoveGeneratorFixture, Rook_Pinned_CantMoveRookFromOnePinToAnother)
{
    // setup
    auto& board = testContext.editChessboard();
    board.setToPlay(Set::WHITE);
    board.PlacePiece(BLACKKING, f8);
    board.PlacePiece(BLACKQUEEN, a1);
    board.PlacePiece(BLACKBISHOP, c3);

    board.PlacePiece(WHITEKING, e1);
    board.PlacePiece(WHITEROOK, d2);
    board.PlacePiece(WHITEROOK, b1);

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(7, result.size());
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

TEST_F(MoveGeneratorFixture, Queen_StartingPos_BlockedByPawns)
{
    // setup
    auto& board = testContext.editChessboard();
    board.setToPlay(Set::WHITE);
    board.PlacePiece(WHITEQUEEN, d1);
    board.PlacePiece(WHITEPAWN, c2);
    board.PlacePiece(WHITEPAWN, d2);
    board.PlacePiece(WHITEPAWN, e2);
    board.PlacePiece(WHITEKING, e1);
    board.PlacePiece(BLACKKING, e8);

    // do
    auto predicate = [](const PackedMove& mv) { return mv.sourceSqr() == Square::D1; };
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen, predicate);

    EXPECT_EQ(3, result.size());
}

TEST_F(MoveGeneratorFixture, Queen_Pinned_CantMoveFromOnePinToAntoher)
{
    // setup
    std::string fen("6k1/8/8/b7/1R6/8/8/q1Q1K3 w - - 0 1");
    FENParser::deserialize(fen.c_str(), testContext);

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);
    EXPECT_EQ(8, result.size());
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
TEST_F(MoveGeneratorFixture, King_InCheck_OnlyValidMoveIsKing)
{
    // setup
    auto& board = testContext.editChessboard();
    board.setToPlay(Set::WHITE);
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKBISHOP, b6);
    board.PlacePiece(BLACKBISHOP, g6);
    board.PlacePiece(BLACKKNIGHT, f6);
    board.PlacePiece(WHITEKNIGHT, h6);
    board.PlacePiece(WHITEPAWN, g2);
    board.PlacePiece(WHITEKING, g1);

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(3, result.size());
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
TEST_F(MoveGeneratorFixture, King_InCheck_OnlyValidMoveIsKing_RookVarient)
{
    // setup
    auto& board = testContext.editChessboard();
    board.setToPlay(Set::WHITE);
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKBISHOP, b6);
    board.PlacePiece(BLACKROOK, g6);
    board.PlacePiece(BLACKKNIGHT, f6);
    board.PlacePiece(WHITEKNIGHT, h6);
    board.PlacePiece(WHITEPAWN, g2);
    board.PlacePiece(WHITEKING, g1);

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(3, result.size());
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
TEST_F(MoveGeneratorFixture, Pawn_Pinned_ShouldHaveTwoMoves)
{
    // setup
    auto& board = testContext.editChessboard();
    board.setToPlay(Set::WHITE);
    board.PlacePiece(BLACKKING, e8);
    // board.PlacePiece(BLACKBISHOP, b6);
    board.PlacePiece(BLACKROOK, g6);
    board.PlacePiece(WHITEPAWN, g2);
    board.PlacePiece(WHITEKING, g1);

    // do
    auto predicate = [](const PackedMove& mv) { return mv.sourceSqr() == Square::G2; };
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen, predicate);

    EXPECT_EQ(2, result.size());
}

TEST_F(MoveGeneratorFixture, Pawn_Pinned_NotAbleToMoveFromOnePinToAnother)
{
    // setup
    std::string fen = "6k1/8/8/b7/1R6/8/8/q2PK3 w - - 0 1";
    FENParser::deserialize(fen.c_str(), testContext);

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(4, result.size());
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
TEST_F(MoveGeneratorFixture, Pawn_NotPinned_ShouldNotHaveAnyMoves)
{
    // setup
    auto& board = testContext.editChessboard();
    board.setToPlay(Set::WHITE);
    board.PlacePiece(BLACKKING, e8);
    // board.PlacePiece(BLACKBISHOP, b6);
    board.PlacePiece(BLACKROOK, g6);
    board.PlacePiece(WHITEPAWN, g2);
    board.PlacePiece(WHITEKING, g1);

    // do
    auto predicate = [](const PackedMove& mv) { return mv.sourceSqr() == Square::G2; };
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen, predicate);

    EXPECT_EQ(2, result.size());
}

// 8 [   ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][ b ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][ P ][   ][   ]
// 1 [   ][   ][   ][   ][   ][   ][ K ][   ]
//     A    B    C    D    E    F    G    H
// valid moves:
TEST_F(MoveGeneratorFixture, Pawn_Pinned_ShouldNotHaveAnyMovesCheckedVariation)
{
    // setup
    std::string fen = "r3k2r/Pppp1ppp/1b3nbN/nPP5/BB2P3/4qN2/Pp1P2PP/R2Q1RK1 w kq - 1 2";
    FENParser::deserialize(fen.c_str(), testContext);

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(3, result.size());
}

// 8 [   ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][ p ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][ B ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][   ][   ][ K ][   ]
//     A    B    C    D    E    F    G    H
// valid moves:
TEST_F(MoveGeneratorFixture, Pawn_Pinned_ShouldNotHaveAnyMovesBlackVariation)
{
    // setup
    auto& board = testContext.editChessboard();
    board.setToPlay(Set::WHITE);
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(WHITEBISHOP, b5);
    board.PlacePiece(BLACKPAWN, d7);
    board.PlacePiece(WHITEKING, g1);

    // do
    auto predicate = [](const PackedMove& mv) { return mv.sourceSqr() == Square::D7; };
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen, predicate);

    EXPECT_EQ(0, result.size());
}

// 8 [   ][   ][   ][   ][   ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [ K ][ P ][   ][   ][   ][   ][   ][ r ]
// 4 [   ][ R ][   ][   ][ P ][   ][   ][ k ]
// 3 [   ][   ][   ][   ][   ][ p ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][   ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// valid moves:
TEST_F(MoveGeneratorFixture, Pawn_Pinned_FromSideOddSituation)
{
    // setup
    auto& board = testContext.editChessboard();
    board.setToPlay(Set::WHITE);
    board.PlacePiece(WHITEKING, a5);
    board.PlacePiece(WHITEPAWN, b5);
    board.PlacePiece(WHITEROOK, b4);
    board.PlacePiece(WHITEPAWN, e4);

    board.PlacePiece(BLACKKING, h4);
    board.PlacePiece(BLACKROOK, h5);

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(10, result.size());
}

TEST_F(MoveGeneratorFixture, Pawn_NotCheckingKing)
{
    // setup
    std::string fen = "r4b2/1p4p1/p5k1/2p5/6pK/4Pq2/P1n2P1P/3R3R w - - 6 34";
    FENParser::deserialize(fen.c_str(), testContext);

    // do
    MoveGenerator gen(testContext);
    gen.generate();

    // verify
    EXPECT_FALSE(gen.isChecked());
    EXPECT_FALSE(gen.readKingPinThreats<Set::WHITE>().isChecked());
}

TEST_F(MoveGeneratorFixture, Pawn_White_CheckingKing)
{
    // setup
    auto& board = testContext.editChessboard();
    board.setToPlay(Set::BLACK);
    board.PlacePiece(BLACKKING, g6);
    board.PlacePiece(WHITEPAWN, h5);
    board.PlacePiece(WHITEKING, h4);

    // do
    MoveGenerator gen(testContext);
    gen.generate();

    // verify
    EXPECT_TRUE(gen.isChecked());
    EXPECT_TRUE(gen.readKingPinThreats<Set::BLACK>().isChecked());
    EXPECT_FALSE(gen.readKingPinThreats<Set::WHITE>().isChecked());
}

TEST_F(MoveGeneratorFixture, Pawn_Black_CheckingKing)
{
    // setup
    auto& board = testContext.editChessboard();
    board.setToPlay(Set::WHITE);
    board.PlacePiece(BLACKKING, g6);
    board.PlacePiece(BLACKPAWN, g5);
    board.PlacePiece(WHITEKING, h4);

    // do
    MoveGenerator gen(testContext);
    gen.generate();

    // verify
    EXPECT_TRUE(gen.isChecked());
    EXPECT_TRUE(gen.readKingPinThreats<Set::WHITE>().isChecked());
    EXPECT_FALSE(gen.readKingPinThreats<Set::BLACK>().isChecked());
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
    std::string fen = "8/2p5/3p4/KP5r/1R3pPk/8/4P3/8 b - g3 0 1";
    FENParser::deserialize(fen.c_str(), testContext);
    const auto& board = testContext.readChessboard();

    EXPECT_EQ(Square::G3, board.readPosition().readEnPassant().readSquare());

    // do
    {
        MoveGenerator gen(testContext);
        auto result = buildMoveVector(gen);

        for (auto mv : result) {
            if (mv.targetSqr() == Square::G3 && mv.sourceSqr() == Square::F4) {
                FAIL() << "There should be no moves to G3 from Black in this setup.";
            }
        }

        // verify
        EXPECT_EQ(17, result.size());
    }
    {
        MoveGenerator gen(board.readPosition(), Set::BLACK, PieceType::PAWN, MoveTypes::CAPTURES_ONLY);
        auto result = buildMoveVector(gen);
        EXPECT_EQ(0, result.size());
    }

    MoveGenerator gen(board.readPosition(), Set::BLACK, PieceType::PAWN);
    auto result = buildMoveVector(gen);
    EXPECT_EQ(4, result.size());
}

TEST_F(MoveGeneratorFixture, PinnedPawn_Black_SimilarAsAboveButMorePieces)
{
    std::string fen = "8/2p5/3p4/KP5r/1R2Pp1k/8/6P1/8 b - e3 0 1";
    FENParser::deserialize(fen.c_str(), testContext);
    const auto& board = testContext.readChessboard();

    EXPECT_EQ(Square::E3, board.readPosition().readEnPassant().readSquare());

    // do
    {
        MoveGenerator gen(testContext);
        auto result = buildMoveVector(gen);

        for (auto mv : result) {
            if (mv.targetSqr() == Square::E3 && mv.sourceSqr() == Square::F4) {
                FAIL() << "There should be no moves to E3 from Black in this setup.";
            }
        }

        // verify
        EXPECT_EQ(16, result.size());
    }
    {
        MoveGenerator gen(board.readPosition(), Set::BLACK, PieceType::PAWN, MoveTypes::CAPTURES_ONLY);
        auto result = buildMoveVector(gen);
        EXPECT_EQ(0, result.size());
    }

    MoveGenerator gen(board.readPosition(), Set::BLACK, PieceType::PAWN);
    auto result = buildMoveVector(gen);
    EXPECT_EQ(4, result.size());
}

TEST_F(MoveGeneratorFixture, PinnedPawn_White_CanNotCaptureEnPassantSinceItWouldPutUsInCheck)
{
    std::string fen = "8/8/3p4/KPp4r/1R2Pp1k/8/6P1/8 w - c6 0 2";
    FENParser::deserialize(fen.c_str(), testContext);
    const auto& board = testContext.readChessboard();

    EXPECT_EQ(Square::C6, board.readPosition().readEnPassant().readSquare());

    // do
    {
        MoveGenerator gen(testContext);
        auto result = buildMoveVector(gen);

        for (auto mv : result) {
            if (mv.targetSqr() == Square::C6) {
                FAIL() << "There should be no moves to C6 from White in this setup.";
            }
        }

        // verify
        EXPECT_EQ(13, result.size());
    }
    {
        MoveGenerator gen(board.readPosition(), Set::WHITE, PieceType::PAWN, MoveTypes::CAPTURES_ONLY);
        auto result = buildMoveVector(gen);
        EXPECT_EQ(0, result.size());
    }

    MoveGenerator gen(board.readPosition(), Set::WHITE, PieceType::PAWN);
    auto result = buildMoveVector(gen);
    EXPECT_EQ(4, result.size());
}

TEST_F(MoveGeneratorFixture, Pawn_NotPinned_CanCaptureEnPassantWhileKingIsOnEPRank)
{
    std::string fen = "8/8/8/K7/4Pp1k/8/6P1/8 b - e3 0 2";
    FENParser::deserialize(fen.c_str(), testContext);
    const auto& board = testContext.readChessboard();

    EXPECT_EQ(Square::E3, board.readPosition().readEnPassant().readSquare());

    // do
    MoveGenerator gen(board.readPosition(), Set::BLACK, PieceType::PAWN);
    auto result = buildMoveVector(gen);
    EXPECT_EQ(2, result.size());
}

TEST_F(MoveGeneratorFixture, Pawn_NotPinned_CanCaptureEnPassantWhileKingIsOnEPRankMorePawnsVariation)
{
    std::string fen = "8/8/8/K7/1R2PpPk/8/8/8 b - g3 0 2";
    FENParser::deserialize(fen.c_str(), testContext);
    const auto& board = testContext.readChessboard();

    EXPECT_EQ(Square::G3, board.readPosition().readEnPassant().readSquare());

    // do
    MoveGenerator gen(board.readPosition(), Set::BLACK, PieceType::PAWN);
    auto result = buildMoveVector(gen);
    EXPECT_EQ(2, result.size());
}

TEST_F(MoveGeneratorFixture, Knight_Move_NothingSpecial)
{
    // setup
    auto& board = testContext.editChessboard();
    board.setToPlay(Set::BLACK);
    board.PlacePiece(BLACKKING, a4);
    board.PlacePiece(BLACKKNIGHT, e6);
    board.PlacePiece(WHITEKING, f1);

    // do
    auto predicate = [](const PackedMove& mv) { return mv.sourceSqr() == Square::E6; };
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen, predicate);

    EXPECT_EQ(8, result.size());
}

// TEST_F(MoveGeneratorFixture, ScholarsMate)
// {
//     // setup
//     std::string fen = "r1bqkbnr/ppp2Qpp/2np4/4p3/2B1P3/8/PPPP1PPP/RNB1K1NR b KQkq - 0 4";
//     FENParser::deserialize(fen.c_str(), testContext);

//     // verify
//     EXPECT_TRUE(testContext.readChessboard().isCheckmated(Set::BLACK));
// }

// TEST_F(MoveGeneratorFixture, ScholarsMateQueenMovesIntoMate)
// {
//     // setup
//     std::string fen = "r1bqkbnr/ppp2ppp/2np4/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 2 4";
//     FENParser::deserialize(fen.c_str(), testContext);

//     // do
//     auto moves = search.GeneratePossibleMoves(testContext);

//     for (auto& mv : moves) {
//         testContext.MakeMove(mv);
//         testContext.UnmakeMove(mv);
//     }

//     // verify
//     MoveCount::Predicate predicate = [](const Move& mv) {
//         static ChessPiece P = WHITEQUEEN;
//         if (mv.Piece == P)
//             return true;

//         return false;
//     };

//     auto count = CountMoves(moves, predicate);
//     EXPECT_EQ(13, count.Moves);
//     EXPECT_EQ(3, count.Captures);
//     EXPECT_EQ(0, count.EnPassants);
//     EXPECT_EQ(0, count.Promotions);
//     EXPECT_EQ(0, count.Castles);
//     EXPECT_EQ(2, count.Checks);
//     EXPECT_EQ(1, count.Checkmates);
// }

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
TEST_F(MoveGeneratorFixture, Bishop_KingInCheck_BlockingOrCapturingCheckingPiece)
{
    std::string fen = "r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1";
    FENParser::deserialize(fen.c_str(), testContext);

    // setup
    PackedMove mv(Square::A1, Square::A8);
    mv.setCapture(true);
    testContext.editChessboard().MakeMove<false>(mv);
    testContext.editChessboard().setToPlay(Set::BLACK);

    // verify
    EXPECT_EQ(WHITEROOK, testContext.readChessboard().readPieceAt(Square::A8));
    // EXPECT_TRUE(testContext.readChessboard().isChecked(testContext.readToPlay()));

    // do
    auto predicate = [](const PackedMove& mv) { return mv.sourceSqr() == Square::B7; };
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen, predicate);

    EXPECT_EQ(2, result.size());
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
    auto& board = testContext.editChessboard();
    board.setToPlay(Set::BLACK);
    board.PlacePiece(BLACKKING, b4);
    board.PlacePiece(WHITEROOK, h4);
    board.PlacePiece(WHITEKING, f1);

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(6, result.size());
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

    PackedMove Qxd1(Square::D8, Square::D1);
    Qxd1.setCapture(true);
    testContext.MakeMove(Qxd1);

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(0, result.size());
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
TEST_F(MoveGeneratorFixture, Pawn_DoubleMoveCheck_EnPassantCaptureNotAvailableBecauseOfPin)
{
    // setup
    std::string fen("8/8/8/3k4/3pP3/8/8/1K1R2B1 b - e3 0 1");
    FENParser::deserialize(fen.c_str(), testContext);

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(7, result.size());
}

/**
* 8 [ r ][   ][   ][   ][ k ][   ][   ][ r ]
* 7 [ p ][   ][ p ][ p ][ q ][ p ][ b ][   ]
* 6 [ b ][ n ][   ][   ][ p ][ n ][ p ][   ]
* 5 [   ][ B ][   ][ P ][ N ][   ][   ][   ]
* 4 [   ][ p ][   ][   ][ P ][   ][   ][   ]
* 3 [   ][   ][ N ][   ][   ][ Q ][   ][ p ]
* 2 [ P ][ P ][ P ][ B ][   ][ P ][ P ][ P ]
* 1 [ R ][   ][   ][   ][ K ][   ][   ][ R ]
*     A    B    C    D    E    F    G    H
Some Edge Case issue where pawn was allowed to "capture" empty square. */
TEST_F(MoveGeneratorFixture, Pawn_Capture_CanNotCaptureNonOpSquareWhilePinned)
{
    // setup
    std::string fen("r3k2r/p1ppqpb1/bn2pnp1/1B1PN3/1p2P3/2N2Q1p/PPPB1PPP/R3K2R b KQkq - 1 1");
    FENParser::deserialize(fen.c_str(), testContext);

    // do
    MoveGenerator gen(testContext.readChessboard().readPosition(), Set::BLACK, PieceType::PAWN);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(7, result.size());
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
TEST_F(MoveGeneratorFixture, Pawn_MoveC3C2_MoveSuccessfull)
{
    // setup
    std::string fen("8/8/1k6/2b5/3P4/2p2K2/8/8 b - - 2 2");
    FENParser::deserialize(fen.c_str(), testContext);

    // do
    MoveGenerator gen(testContext);
    auto predicate = [](const PackedMove& mv) { return mv.sourceSqr() == Square::C3; };
    auto result = buildMoveVector(gen, predicate);

    EXPECT_EQ(1, result.size());
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
TEST_F(MoveGeneratorFixture, Knight_Nxb3_IllegalMoveSincePinned)
{
    std::string fen("8/8/8/3k4/3n4/1B6/8/1K1R4 b - - 0 1");
    FENParser::deserialize(fen.c_str(), testContext);

    // do
    MoveGenerator gen(testContext);
    auto predicate = [](const PackedMove& mv) { return mv.sourceSqr() == Square::D4; };
    auto result = buildMoveVector(gen, predicate);

    EXPECT_EQ(0, result.size());
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
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(4, result.size());
}

/**
 * 8 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 7 [   ][ k ][   ][   ][   ][   ][   ][   ]
 * 6 [   ][   ][   ][ep ][   ][   ][   ][   ]
 * 5 [   ][   ][ P ][ p ][   ][   ][   ][   ]
 * 4 [   ][   ][ K ][   ][   ][   ][   ][   ]
 * 3 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 2 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 1 [   ][   ][   ][   ][   ][   ][   ][   ]
 *     A    B    C    D    E    F    G    H
 * fen: 8/1k6/8/2Pp4/2K5/8/8/8 w - d6 5 4*/
TEST_F(MoveGeneratorFixture, PawnDoubleMoveCheck_White_EnPassantCaptureAvailable)
{
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, b7);
    board.PlacePiece(BLACKPAWN, d5);
    board.PlacePiece(WHITEPAWN, c5);
    board.PlacePiece(WHITEKING, c4);

    board.setEnPassant(d6);

    auto kingMask = board.readPosition().calcKingMask<Set::WHITE>();
    EXPECT_TRUE(kingMask.isChecked());

    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(8, result.size());
}
// /**
// * 8 [   ][   ][   ][   ][   ][   ][   ][   ]
// * 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// * 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// * 5 [   ][   ][   ][ k ][   ][   ][   ][   ]
// * 4 [   ][   ][   ][ p ][ P ][   ][   ][   ]
// * 3 [   ][   ][   ][   ][ep ][   ][   ][   ]
// * 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// * 1 [   ][ K ][   ][   ][ R ][   ][ B ][   ]
// *     A    B    C    D    E    F    G    H
// 8/8/8/3k4/3pP3/8/8/1K2R1B1 b - e3 0 1
// Pawn is guarded by white rook in this scenario*/
TEST_F(MoveGeneratorFixture, PawnDoubleMoveCheck_Black_EnPassantCaptureAvailableForPawn)
{
    auto& board = testContext.editChessboard();

    board.PlacePiece(BLACKKING, d5);
    board.PlacePiece(BLACKPAWN, d4);
    board.PlacePiece(WHITEKING, b1);
    board.PlacePiece(WHITEPAWN, e4);
    board.PlacePiece(WHITEROOK, e1);
    board.PlacePiece(WHITEBISHOP, g1);

    board.setEnPassant(e3);

    auto kingMask = board.readPosition().calcKingMask<Set::BLACK>();
    EXPECT_TRUE(kingMask.isChecked());

    testContext.editChessboard().setToPlay(Set::BLACK);
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(7, result.size());
}

// /**
// * 8 [   ][   ][   ][   ][   ][   ][   ][   ]
// * 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// * 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// * 5 [   ][   ][   ][ k ][   ][   ][   ][   ]
// * 4 [   ][   ][   ][ p ][ P ][   ][   ][   ]
// * 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// * 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// * 1 [   ][ K ][   ][ R ][   ][   ][ B ][   ]
// *     A    B    C    D    E    F    G    H
// 8/8/8/3k4/3pP3/8/8/1K2R1B1 b - e3 0 1
// Pawn is pinned by white rook in this scenario.  */
TEST_F(MoveGeneratorFixture, PawnDoubleMoveCheck_Black_EnPassantCaptureNotAvailableBecauseOfPin)
{
    auto& board = testContext.editChessboard();

    board.PlacePiece(BLACKKING, d5);
    board.PlacePiece(BLACKPAWN, d4);
    board.PlacePiece(WHITEKING, b1);
    board.PlacePiece(WHITEPAWN, e4);
    board.PlacePiece(WHITEROOK, d1);
    board.PlacePiece(WHITEBISHOP, g1);

    board.setEnPassant(e3);

    auto kingMask = board.readPosition().calcKingMask<Set::BLACK>();
    EXPECT_TRUE(kingMask.isChecked());

    testContext.editChessboard().setToPlay(Set::BLACK);
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    EXPECT_EQ(7, result.size());

    auto itr = std::find_if(result.begin(), result.end(), [](const PackedMove& mv) { return mv.sourceSqr() == Square::D4; });
    EXPECT_EQ(itr, result.end()) << "There shouldn't be any pawn moves amongst the available moves";
}
/**
* 8 [   ][   ][   ][ r ][ k ][   ][   ][   ]
* 7 [   ][   ][   ][   ][   ][   ][   ][   ]
* 6 [   ][   ][   ][ Q ][   ][   ][   ][   ]
* 5 [   ][   ][   ][   ][   ][   ][   ][   ]
* 4 [   ][   ][   ][   ][   ][ q ][   ][   ]
* 3 [   ][   ][   ][   ][   ][   ][   ][   ]
* 2 [   ][   ][   ][ K ][   ][   ][   ][   ]
* 1 [   ][   ][   ][   ][   ][   ][   ][   ]
*     A    B    C    D    E    F    G    H
3rk3/8/3Q4/8/5q2/8/3K4/8 w - - 3 3 */
TEST_F(MoveGeneratorFixture, PinnedQueen_White_CanNotMoveQueen)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, e8);
    board.PlacePiece(BLACKROOK, d8);
    board.PlacePiece(BLACKQUEEN, f4);
    board.PlacePiece(WHITEQUEEN, d6);
    board.PlacePiece(WHITEKING, d2);

    // verify
    auto kingMask = board.readPosition().calcKingMask<Set::WHITE>();
    auto bkingMask = board.readPosition().calcKingMask<Set::BLACK>();

    EXPECT_TRUE(kingMask.isChecked());
    EXPECT_FALSE(bkingMask.isChecked());

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    // verify
    EXPECT_EQ(6, result.size());
}

/**
 * 8 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 7 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 6 [   ][ k ][   ][   ][   ][   ][   ][   ]
 * 5 [   ][   ][ b ][   ][   ][   ][   ][   ]
 * 4 [   ][   ][ p ][ P ][   ][   ][   ][   ]
 * 3 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 2 [   ][   ][   ][   ][   ][ K ][   ][   ]
 * 1 [   ][   ][   ][   ][   ][   ][   ][   ]
 *     A    B    C    D    E    F    G    H
 * fen: 8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1
 */
TEST_F(MoveGeneratorFixture, PawnDobuleMove_Black_BlackBishopToCapturePawn)
{
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, b6);
    board.PlacePiece(BLACKPAWN, c4);
    board.PlacePiece(BLACKBISHOP, c5);

    board.PlacePiece(WHITEKING, f2);
    board.PlacePiece(WHITEPAWN, d4);

    board.setEnPassant(d3);

    auto kingMask = board.readPosition().calcKingMask<Set::WHITE>();
    auto bkingMask = board.readPosition().calcKingMask<Set::BLACK>();

    EXPECT_FALSE(kingMask.isChecked());
    EXPECT_FALSE(bkingMask.isChecked());

    // do
    testContext.editChessboard().setToPlay(Set::BLACK);
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    // verify
    EXPECT_EQ(15, result.size());
}

/**
* 8 [   ][   ][   ][ r ][   ][ k ][   ][   ]
* 7 [   ][   ][   ][   ][   ][   ][   ][   ]
* 6 [   ][   ][   ][ N ][   ][   ][   ][   ]
* 5 [   ][   ][   ][   ][   ][ q ][   ][   ]
* 4 [   ][   ][   ][   ][   ][   ][   ][   ]
* 3 [   ][   ][   ][ K ][   ][   ][   ][   ]
* 2 [   ][   ][   ][   ][   ][   ][   ][   ]
* 1 [   ][   ][   ][   ][   ][   ][   ][   ]
*     A    B    C    D    E    F    G    H
3rk3/8/3Q4/8/5q2/8/3K4/8 w - - 3 3 */
TEST_F(MoveGeneratorFixture, Knight_Pinned_CanNotCaptureCheckingPiece)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, f8);
    board.PlacePiece(BLACKROOK, d8);
    board.PlacePiece(BLACKQUEEN, f5);
    board.PlacePiece(WHITEKNIGHT, d6);
    board.PlacePiece(WHITEKING, d3);

    // verify
    auto kingMask = board.readPosition().calcKingMask<Set::WHITE>();
    auto bkingMask = board.readPosition().calcKingMask<Set::BLACK>();

    EXPECT_TRUE(kingMask.isChecked());
    EXPECT_FALSE(bkingMask.isChecked());

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    // verify
    EXPECT_EQ(6, result.size());
    auto itr = std::find_if(result.begin(), result.end(), [](const PackedMove& mv) { return mv.sourceSqr() == Square::D6; });
    EXPECT_EQ(itr, result.end()) << "There shouldn't be any knight moves amongst the available moves";
}

TEST_F(MoveGeneratorFixture, Knight_Pinned_CanNotMoveFromOnePinToAnother)
{
    // setup
    std::string fen("6k1/8/8/b7/1R6/8/8/q2NK3 w - - 0 1");
    FENParser::deserialize(fen.c_str(), testContext);

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);
    EXPECT_EQ(4, result.size());
}

/**
* 8 [   ][   ][   ][   ][   ][ k ][   ][   ]
* 7 [   ][   ][   ][   ][   ][   ][   ][   ]
* 6 [   ][   ][   ][ N ][   ][   ][   ][   ]
* 5 [   ][   ][   ][   ][   ][ q ][   ][   ]
* 4 [   ][   ][   ][   ][   ][   ][   ][   ]
* 3 [   ][   ][   ][ K ][   ][   ][   ][   ]
* 2 [   ][   ][   ][   ][   ][   ][   ][   ]
* 1 [   ][   ][   ][   ][   ][   ][   ][   ]
*     A    B    C    D    E    F    G    H
3rk3/8/3Q4/8/5q2/8/3K4/8 w - - 3 3 */
TEST_F(MoveGeneratorFixture, Knight_CaptureCheckingQueen_ShouldOnlyHaveOneMoveAvailable)
{
    // setup
    auto& board = testContext.editChessboard();
    board.PlacePiece(BLACKKING, f8);
    board.PlacePiece(BLACKQUEEN, f5);
    board.PlacePiece(WHITEKNIGHT, d6);
    board.PlacePiece(WHITEKING, d3);

    // verify
    auto kingMask = board.readPosition().calcKingMask<Set::WHITE>();
    auto bkingMask = board.readPosition().calcKingMask<Set::BLACK>();

    EXPECT_TRUE(kingMask.isChecked());
    EXPECT_FALSE(bkingMask.isChecked());

    // do
    MoveGenerator gen(testContext);
    auto result = buildMoveVector(gen);

    // verify
    EXPECT_EQ(8, result.size());

    int counter = 0;
    for (auto& mv : result) {
        if (mv.sourceSqr() == Square::D6)
            counter++;
    }
    EXPECT_EQ(2, counter) << "There should only exist two knight moves amongst the available moves";
}
/*
    8  [r][ ][ ][ ][k][ ][ ][r]
    7  [p][ ][p][p][q][p][b][ ]
    6  [b][n][ ][ ][p][n][p][ ]
    5  [ ][ ][ ][P][N][ ][ ][ ]
    4  [ ][p][ ][ ][P][ ][ ][ ]
    3  [ ][ ][N][ ][ ][Q][ ][p]
    2  [P][P][P][B][B][P][P][P]
    1  [R][ ][ ][ ][K][ ][ ][R]
        A  B  C  D  E  F  G  H
r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 **/
TEST_F(MoveGeneratorFixture, PerftTestPositionTwo_CaptureMoves_ShouldHaveEightCaptureMoves)
{
    // setup
    char inputFen[] = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    FENParser::deserialize(inputFen, testContext);

    // do
    MoveGenerator gen(testContext.readChessboard().readPosition(), Set::WHITE, PieceType::NONE, MoveTypes::CAPTURES_ONLY);

    // verify
    auto result = buildMoveVector(gen);
    EXPECT_EQ(8, result.size());
}

TEST_F(MoveGeneratorFixture, NewApiTest)
{
    // setup
    char inputFen[] = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    FENParser::deserialize(inputFen, testContext);

    // do
    MoveGenerator gen(testContext.readChessboard().readPosition(), Set::WHITE, PieceType::NONE, MoveTypes::CAPTURES_ONLY);

    // verify
    auto result = buildMoveVector(gen);
    EXPECT_EQ(8, result.size());
}

}  // namespace ElephantTest