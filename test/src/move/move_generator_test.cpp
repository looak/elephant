#include <gtest/gtest.h>
#include <system/platform.hpp>

#include <algorithm>
#include <functional>
#include <string_view>
#include <vector>

#include <io/fen_parser.hpp>
#include <core/game_context.hpp>
#include <move/generation/move_generator.hpp>


#include "bitboard_test_helpers.hpp"

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
    virtual void SetUp() {
        testParams.setAll(true);
    };
    virtual void TearDown() {};

    using MovePredicate = std::function<bool(const PackedMove&)>;

    template<Set t_set>
    Bitboard buildMoveMask(MoveGenerator<t_set>& gen, MovePredicate pred = nullptr) const
    {
        Bitboard result;
        while (auto mv = gen.pop().move) {
            if (pred && !pred(mv))
                continue;

            result[mv.targetSqr()] = true;
        }
        return result;
    }

    // every move the generator hands out for us in the current position, those matching pred when one is given.
    template<Set us>
    std::vector<PackedMove> collectMoves(MovePredicate pred = nullptr)
    {
        MoveGenerator<us> gen(testContext.readChessPosition(), testParams);
        std::vector<PackedMove> result;
        while (PackedMove mv = gen.pop().move) {
            if (!pred || pred(mv))
                result.push_back(mv);
        }
        return result;
    }

    // sets the board up from fen and returns the moves of the side to move, those matching pred when one is given.
    std::vector<PackedMove> legalMoves(std::string_view fen, MovePredicate pred = nullptr)
    {
        EXPECT_TRUE(io::fen_parser::deserialize(fen, testContext.editChessboard())) << fen;
        if (testContext.readToPlay() == Set::WHITE)
            return collectMoves<Set::WHITE>(pred);
        return collectMoves<Set::BLACK>(pred);
    }

    bool sideToMoveIsChecked()
    {
        if (testContext.readToPlay() == Set::WHITE)
            return WhiteMoveGen(testContext.readChessPosition(), testParams).isChecked();
        return BlackMoveGen(testContext.readChessPosition(), testParams).isChecked();
    }

    static MovePredicate from(Square source)
    {
        return [source](const PackedMove& mv) { return mv.sourceSqr() == source; };
    }

    MovePredicate pieceType(PieceType type)
    {
        return [this, type](const PackedMove& mv) { return testContext.readChessPosition().pieceAt(mv.sourceSqr()).getType() == type; };
    }

    static bool isCapture(const PackedMove& mv) { return mv.isCapture(); }
    static bool isPromotion(const PackedMove& mv) { return mv.isPromotion(); }
    static bool isCastling(const PackedMove& mv) { return mv.isCastling(); }
    static bool isEnPassant(const PackedMove& mv) { return mv.isEnPassant(); }

    static size_t countIf(const std::vector<PackedMove>& moves, MovePredicate pred)
    {
        return static_cast<size_t>(std::count_if(moves.begin(), moves.end(), pred));
    }

    template<typename... Squares>
    bool contains_exactly_target_squares(const std::vector<PackedMove>& moves, Squares... inputTargetSquares)
    {
        if (moves.size() != sizeof...(inputTargetSquares)) {
            return false;
        }

        if constexpr (sizeof...(inputTargetSquares) == 0) {
            return true;
        }
        else {
            // Put the variadic arguments into a container (std::array is efficient).
            std::array<Square, sizeof...(inputTargetSquares)> targets = { inputTargetSquares... };

            // Extract the target squares from the vector of moves.
            std::vector<Square> moveTargetSquares;
            moveTargetSquares.reserve(moves.size());
            for (const auto& move : moves) {
                moveTargetSquares.push_back(move.targetSqr());
            }

            // Sort both containers. This makes them easy to compare and handles any order differences and duplicates correctly.
            std::sort(targets.begin(), targets.end());
            std::sort(moveTargetSquares.begin(), moveTargetSquares.end());

            return std::equal(targets.begin(), targets.end(), moveTargetSquares.begin());
        }
    }

    GameContext testContext;
    MoveGenParams testParams;
};
////////////////////////////////////////////////////////////////

// move generator asserts if the board is empty
TEST_F(MoveGeneratorFixture, Empty)
{
    MoveGenerator<Set::WHITE> whiteGen(testContext.readChessPosition(), testParams);
    MoveGenerator<Set::BLACK> blackGen(testContext.readChessPosition(), testParams);

    PackedMove whiteMove = whiteGen.pop().move;
    PackedMove blackMove = blackGen.pop().move;

    EXPECT_TRUE(whiteMove.isNull());
    EXPECT_TRUE(blackMove.isNull());
}

/** Most basic move generation test, a king in the middle of the board with no other pieces,
* should have eight moves available. */
TEST_F(MoveGeneratorFixture, KingFromE4_White_EightMovesAvailable)
{
    using enum Square;

    // setup
    PositionEditor editor = testContext.editChessPosition();
    editor.placePieces(
        piece_constants::white_king, Square::E4
    );

    // expected
    Bitboard expected = BitboardResultFactory::buildExpectedBoard(
        D5, E5, F5,
        D4,/*K*/F4,
        D3, E3, F3
    );

    WhiteMoveGen gen(testContext.readChessPosition(), testParams);

    Bitboard result = buildMoveMask(gen);
    EXPECT_EQ(expected, result);
}

/** Most basic move generation test, a king in the middle of the board with no other pieces,
* should have eight moves available. */
TEST_F(MoveGeneratorFixture, BlackKingE4_EightMovesAvailable)
{
    using enum Square;

    // setup
    PositionEditor editor = testContext.editChessPosition();
    editor.placePieces(
        piece_constants::black_king, Square::E4
    );

    // expected
    Bitboard expected = BitboardResultFactory::buildExpectedBoard(
        D5, E5, F5,
        D4,/*K*/F4,
        D3, E3, F3
    );

    BlackMoveGen gen(testContext.readChessPosition(), testParams);

    Bitboard result = buildMoveMask(gen);
    EXPECT_EQ(expected, result);
}

TEST_F(MoveGeneratorFixture, KingFromE1_White_FiveMovesAvailable)
{
    auto result = legalMoves("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
    EXPECT_EQ(5u, result.size());
}

TEST_F(MoveGeneratorFixture, KingFromE8_Black_FiveMovesAvailable)
{
    auto result = legalMoves("4k3/8/8/8/8/8/8/4K3 b - - 0 1");
    EXPECT_EQ(5u, result.size());
}

TEST_F(MoveGeneratorFixture, KingAndPawn_White_PawnBlocksOneMoveOfKingButHasDoublePushAvailable)
{
    auto result = legalMoves("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1");
    EXPECT_EQ(6u, result.size());
}

// 8 [   ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][ N ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][ K ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// the king can capture the knight on d7 but can't move to f8 since the knight covers it.
TEST_F(MoveGeneratorFixture, KingMoveGeneration_Black_KingCanCaptureOpponentKnight)
{
    auto result = legalMoves("4k3/3N4/8/8/8/8/8/4K3 b - - 0 1");

    EXPECT_TRUE(contains_exactly_target_squares(result, Square::D8, Square::D7, Square::E7, Square::F7));
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
// valid moves: e2, f2, d1
// can not capture knight on d2 since it is guarded by rook on d8, f1 is covered by the knight.
TEST_F(MoveGeneratorFixture, King_GuardedPiece_CanNotCaptureKnight)
{
    auto result = legalMoves("3rk3/8/8/8/8/8/3n4/4K3 w - - 0 1");

    EXPECT_TRUE(contains_exactly_target_squares(result, Square::E2, Square::F2, Square::D1));
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
    auto result = legalMoves("r3k3/8/8/8/8/8/8/4K3 b q - 0 1", from(Square::E8));

    EXPECT_EQ(6u, result.size());
    EXPECT_EQ(1u, countIf(result, isCastling));
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
    auto result = legalMoves("r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1", from(Square::E8));

    EXPECT_EQ(7u, result.size());
    EXPECT_EQ(2u, countIf(result, isCastling));
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
// valid king moves: d8, f8, d7, e7
// can not castle since we are in check.
TEST_F(MoveGeneratorFixture, King_InCheck_CanNotCastle)
{
    auto result = legalMoves("r3k3/8/3N4/8/8/8/8/4K3 b q - 0 1", from(Square::E8));

    EXPECT_TRUE(contains_exactly_target_squares(result, Square::D8, Square::F8, Square::D7, Square::E7));
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
// valid moves: f2, d1
TEST_F(MoveGeneratorFixture, King_DoubleCheck_OnlyValidMovesAreToMoveKing)
{
    auto result = legalMoves("3rk3/4r3/8/8/8/5n2/3n2P1/4K3 w - - 0 1");

    EXPECT_TRUE(contains_exactly_target_squares(result, Square::F2, Square::D1));
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
// valid moves: e2, f2, d1
// can not capture knight on d2 since it is guarded by rook on d8.
TEST_F(MoveGeneratorFixture, King_Checked_CanNotCaptureGuardedPiece)
{
    auto result = legalMoves("3rk3/8/8/8/8/5n2/3n4/4K3 w - - 0 1");

    EXPECT_TRUE(contains_exactly_target_squares(result, Square::E2, Square::F2, Square::D1));
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
// valid moves: e2, f2, f1
TEST_F(MoveGeneratorFixture, King_NotChecked_CanNotMoveIntoCheck)
{
    auto result = legalMoves("3rk3/8/8/8/8/8/8/4K3 w - - 0 1");

    EXPECT_TRUE(contains_exactly_target_squares(result, Square::E2, Square::F2, Square::F1));
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
// valid moves: d2, e2, f2
TEST_F(MoveGeneratorFixture, King_NotChecked_CanNotMoveIntoCheckKnightVariation)
{
    auto result = legalMoves("4k3/8/8/8/8/4n3/8/4K3 w - - 0 1");

    EXPECT_TRUE(contains_exactly_target_squares(result, Square::D2, Square::E2, Square::F2));
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
    auto result = legalMoves("4k3/5P2/8/8/8/8/8/4K3 b - - 0 1");

    // verify
    EXPECT_EQ(5u, result.size());
    EXPECT_EQ(1u, countIf(result, isCapture));
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
// the knight covers d8, which the king passes when castling queen side.
TEST_F(MoveGeneratorFixture, King_Castling_CanNotQueenSideCastleBecauseItsBlockedByKnight)
{
    auto result = legalMoves("r3k3/8/2N5/8/8/8/8/4K3 b q - 0 1", from(Square::E8));

    EXPECT_TRUE(contains_exactly_target_squares(result, Square::F8, Square::D7, Square::F7));
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
    auto result = legalMoves("r3k2r/8/8/1B6/8/8/8/4K3 b kq - 0 1");

    EXPECT_TRUE(contains_exactly_target_squares(result, Square::D8, Square::F8, Square::E7, Square::F7));
}

// both castles are available, the rook on a2 covers the king's other squares and both rooks are attacked.
TEST_F(MoveGeneratorFixture, King_Castling_MoreCastlingIssues)
{
    auto result = legalMoves("1B2k2r/1b4bq/8/8/8/8/r7/R3K2R w KQ - 2 2");

    EXPECT_EQ(23u, result.size());
    EXPECT_EQ(2u, countIf(result, isCastling));
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
    // b6 1 move, e2 blocked by e3, e3 1 move, f2 2 moves, g5 blocked by the knight, g7 4 promotions, h2 blocked by h3.
    auto result = legalMoves("4k3/6P1/1P4n1/6P1/8/4P2p/4PP1P/4K3 w - - 0 1", pieceType(PieceType::PAWN));

    EXPECT_EQ(8u, result.size());
}

TEST_F(MoveGeneratorFixture, PawnBasicMoves_WhiteAndBlack_SimpleCaptures)
{
    // c5, fxg5, cxd4, e4, exd4 and five king moves.
    auto white = legalMoves("4k3/8/8/5pp1/2Pp1P2/2P1P3/8/4K3 w - - 0 1");
    EXPECT_EQ(10u, white.size());
    EXPECT_EQ(3u, countIf(white, isCapture));

    // gxf4, dxc3, dxe3.
    auto black = legalMoves("4k3/8/8/5pp1/2Pp1P2/2P1P3/8/4K3 b - - 0 1", isCapture);
    EXPECT_EQ(3u, black.size());
}

TEST_F(MoveGeneratorFixture, PawnPromotion)
{
    auto result = legalMoves("4k3/8/8/8/8/8/p7/4K3 b - - 0 1", isPromotion);

    bool found[4]{false, false, false, false};
    EXPECT_EQ(4u, result.size());
    for (auto&& move : result) {
        EXPECT_EQ(Square::A1, move.targetSqr());
        i32 indx = move.readPromoteToPieceType() - static_cast<i32>(PieceType::KNIGHT);
        found[indx] = true;
    }
    EXPECT_TRUE(found[0]);  // knight
    EXPECT_TRUE(found[1]);  // bishop
    EXPECT_TRUE(found[2]);  // rook
    EXPECT_TRUE(found[3]);  // queen
}

TEST_F(MoveGeneratorFixture, PawnPromotionCapture)
{
    auto result = legalMoves("4k3/8/8/8/8/8/p7/1R2K3 b - - 0 1", isPromotion);

    // verify
    EXPECT_EQ(8u, result.size());

    bool found[8] = {false};
    for (auto&& move : result) {
        i32 indx = move.readPromoteToPieceType() - static_cast<i32>(PieceType::KNIGHT) + (4 * (int)move.isCapture());
        EXPECT_EQ(move.targetSqr() == Square::B1, move.isCapture());
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
    auto result = legalMoves("4k3/8/5n2/8/4N3/8/8/4K3 w - - 0 1");
    EXPECT_EQ(13u, result.size());

    auto knightMoves = legalMoves("4k3/8/5n2/8/4N3/8/8/4K3 w - - 0 1", from(Square::E4));
    EXPECT_EQ(8u, knightMoves.size());
    EXPECT_EQ(1u, countIf(knightMoves, isCapture));
}

TEST_F(MoveGeneratorFixture, KnightsInAllCorner_White_EightAvailableMoves)
{
    {  // knight on a1 has two moves, plus five king moves.
        auto result = legalMoves("4k3/8/8/8/8/8/8/N3K3 w - - 0 1");
        EXPECT_EQ(7u, result.size());
    }

    {  // knights in all corners have eight moves, plus five king moves.
        auto result = legalMoves("N3k2N/8/8/8/8/8/8/N3K2N w - - 0 1");
        EXPECT_EQ(13u, result.size());
        EXPECT_EQ(8u, countIf(result, pieceType(PieceType::KNIGHT)));
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
// a1=Q, a1=R, a1=B, a1=N, b1=Q+ b1=R+, b1=B, b1=N and five king moves.
// 8 promotions, 4 of which are captures, two of which are checks.
TEST_F(MoveGeneratorFixture, PawnPromotionCaptureCheck)
{
    auto result = legalMoves("4k3/8/8/8/8/8/p7/1R4K1 b - - 0 1");

    // verify
    EXPECT_EQ(13u, result.size());
    EXPECT_EQ(4u, countIf(result, isCapture));
    EXPECT_EQ(8u, countIf(result, isPromotion));
    EXPECT_EQ(0u, countIf(result, isCastling));
    EXPECT_EQ(0u, countIf(result, isEnPassant));

    int checks = 0;
    for (auto mv : result) {
        testContext.MakeMove(mv);
        checks += sideToMoveIsChecked() ? 1 : 0;
        testContext.UnmakeMove();
    }
    EXPECT_EQ(2, checks);
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
// the rook covers d5, d6, b7 & c7 and is guarded by the king.
TEST_F(MoveGeneratorFixture, King_Threat_CantCaptureGuardedByKing)
{
    auto result = legalMoves("4k3/3r4/2K5/8/8/8/8/8 w - - 0 1");

    EXPECT_TRUE(contains_exactly_target_squares(result, Square::B5, Square::C5, Square::B6));
}

// 8 [   ][   ][   ][   ][ k ][   ][ b ][   ]
// 7 [   ][   ][   ][   ][   ][ P ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][ K ][ R ][   ][   ]
//     A    B    C    D    E    F    G    H
// Only valid move for bishop is to capture the pawn.
TEST_F(MoveGeneratorFixture, Bishop_KingInCheck_BishopOnlyHasOneMove)
{
    auto result = legalMoves("4k1b1/5P2/8/8/8/8/8/4KR2 b - - 0 1");
    EXPECT_EQ(5u, result.size());

    auto bishopMoves = legalMoves("4k1b1/5P2/8/8/8/8/8/4KR2 b - - 0 1", from(Square::G8));
    EXPECT_TRUE(contains_exactly_target_squares(bishopMoves, Square::F7));
}

// 8 [   ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][ b ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][ K ][ R ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
TEST_F(MoveGeneratorFixture, Bishop_Pinned_NoValidMoves)
{
    auto result = legalMoves("4k3/8/8/4b3/8/8/8/3KR3 b - - 0 1", from(Square::E5));

    EXPECT_EQ(0u, result.size());
}

// the queen pins the bishop on c1 and the bishop on a5 pins the rook, only the king can move.
TEST_F(MoveGeneratorFixture, Bishop_Pinned_NotAllowedToMoveFromOnePinToAnother)
{
    auto result = legalMoves("6k1/8/8/b7/1R6/8/8/q1B1K3 w - - 0 1");

    EXPECT_EQ(5u, result.size());
    EXPECT_EQ(5u, countIf(result, from(Square::E1)));
}

// 8 [   ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][ b ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][ N ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][ K ][ R ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// found an edge case where a pinned piece would be allowed to capture
// a different piece than the one pinning it.
TEST_F(MoveGeneratorFixture, Bishop_Pinned_NotAllowedToCapture)
{
    auto result = legalMoves("4k3/8/8/4b3/8/6N1/8/3KR3 b - - 0 1", from(Square::E5));

    EXPECT_EQ(0u, result.size());
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
TEST_F(MoveGeneratorFixture, Rook_Captures_TwoRooksCanCaptureSamePiece)
{
    auto rookMoves = pieceType(PieceType::ROOK);
    auto result = legalMoves("R6r/4k3/8/8/8/8/8/4K2R w - - 0 1", [&](const PackedMove& mv) { return mv.isCapture() && rookMoves(mv); });

    EXPECT_EQ(2u, result.size());
    for (auto& mv : result) {
        EXPECT_EQ(Square::H8, mv.targetSqr());
    }
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
// the rook on b1 can only move along the pin, Rxa1, Rc1 & Rd1, the rook on d2 can't move.
TEST_F(MoveGeneratorFixture, Rook_Pinned_CantMoveRookFromOnePinToAnother)
{
    auto result = legalMoves("5k2/8/8/8/8/2b5/3R4/qR2K3 w - - 0 1");

    EXPECT_EQ(7u, result.size());
    EXPECT_EQ(0u, countIf(result, from(Square::D2)));
    EXPECT_EQ(3u, countIf(result, from(Square::B1)));
}

// 8 [   ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][ P ][ P ][ P ][   ][   ][   ]
// 1 [   ][   ][   ][ Q ][ K ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// valid moves: c1, b1, a1
TEST_F(MoveGeneratorFixture, Queen_StartingPos_BlockedByPawns)
{
    auto result = legalMoves("4k3/8/8/8/8/8/2PPP3/3QK3 w - - 0 1", from(Square::D1));

    EXPECT_TRUE(contains_exactly_target_squares(result, Square::C1, Square::B1, Square::A1));
}

// the queen on c1 can only move along the pin, Qxa1, Qb1 & Qd1, the rook is pinned by the bishop.
TEST_F(MoveGeneratorFixture, Queen_Pinned_CantMoveFromOnePinToAntoher)
{
    auto result = legalMoves("6k1/8/8/b7/1R6/8/8/q1Q1K3 w - - 0 1");

    EXPECT_EQ(8u, result.size());
    EXPECT_EQ(3u, countIf(result, from(Square::C1)));
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
// valid moves: Kf1, Kh1, Kh2
TEST_F(MoveGeneratorFixture, King_InCheck_OnlyValidMoveIsKing)
{
    auto result = legalMoves("4k3/8/1b3nbN/8/8/8/6P1/6K1 w - - 0 1");

    EXPECT_TRUE(contains_exactly_target_squares(result, Square::F1, Square::H1, Square::H2));
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
// valid moves: Kf1, Kh1, Kh2
TEST_F(MoveGeneratorFixture, King_InCheck_OnlyValidMoveIsKing_RookVarient)
{
    auto result = legalMoves("4k3/8/1b3nrN/8/8/8/6P1/6K1 w - - 0 1");

    EXPECT_TRUE(contains_exactly_target_squares(result, Square::F1, Square::H1, Square::H2));
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
// the pawn is pinned along the file it moves on, g3 & g4.
TEST_F(MoveGeneratorFixture, Pawn_Pinned_ShouldHaveTwoMoves)
{
    auto result = legalMoves("4k3/8/6r1/8/8/8/6P1/6K1 w - - 0 1", from(Square::G2));

    EXPECT_TRUE(contains_exactly_target_squares(result, Square::G3, Square::G4));
}

// the queen pins the pawn on d1 and the bishop on a5 pins the rook, only the king can move.
TEST_F(MoveGeneratorFixture, Pawn_Pinned_NotAbleToMoveFromOnePinToAnother)
{
    auto result = legalMoves("6k1/8/8/b7/1R6/8/8/q2PK3 w - - 0 1");

    EXPECT_EQ(4u, result.size());
    EXPECT_EQ(4u, countIf(result, from(Square::E1)));
}

// the queen on e3 checks the king, Rf2, Kh1 & dxe3.
TEST_F(MoveGeneratorFixture, Pawn_Pinned_ShouldNotHaveAnyMovesCheckedVariation)
{
    auto result = legalMoves("r3k2r/Pppp1ppp/1b3nbN/nPP5/BB2P3/4qN2/Pp1P2PP/R2Q1RK1 w kq - 1 2");

    EXPECT_EQ(3u, result.size());
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
TEST_F(MoveGeneratorFixture, Pawn_Pinned_ShouldNotHaveAnyMovesBlackVariation)
{
    auto result = legalMoves("4k3/3p4/8/1B6/8/8/8/6K1 b - - 0 1", from(Square::D7));

    EXPECT_EQ(0u, result.size());
}

// 8 [   ][   ][   ][   ][   ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [ K ][ P ][   ][   ][   ][   ][   ][ r ]
// 4 [   ][ R ][   ][   ][ P ][   ][   ][ k ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][   ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// the pawn on b5 is pinned along the rank, six rook moves, three king moves & e5.
TEST_F(MoveGeneratorFixture, Pawn_Pinned_FromSideOddSituation)
{
    auto result = legalMoves("8/8/8/KP5r/1R2P2k/8/8/8 w - - 0 1");

    EXPECT_EQ(10u, result.size());
    EXPECT_EQ(0u, countIf(result, from(Square::B5)));
}

TEST_F(MoveGeneratorFixture, Pawn_NotCheckingKing)
{
    io::fen_parser::deserialize("r4b2/1p4p1/p5k1/2p5/6pK/4Pq2/P1n2P1P/3R3R w - - 6 34", testContext.editChessboard());

    EXPECT_FALSE(sideToMoveIsChecked());
}

TEST_F(MoveGeneratorFixture, Pawn_White_CheckingKing)
{
    io::fen_parser::deserialize("8/8/6k1/7P/7K/8/8/8 b - - 0 1", testContext.editChessboard());

    EXPECT_TRUE(BlackMoveGen(testContext.readChessPosition(), testParams).isChecked());
    EXPECT_FALSE(WhiteMoveGen(testContext.readChessPosition(), testParams).isChecked());
}

TEST_F(MoveGeneratorFixture, Pawn_Black_CheckingKing)
{
    io::fen_parser::deserialize("8/8/6k1/6p1/7K/8/8/8 w - - 0 1", testContext.editChessboard());

    EXPECT_TRUE(WhiteMoveGen(testContext.readChessPosition(), testParams).isChecked());
    EXPECT_FALSE(BlackMoveGen(testContext.readChessPosition(), testParams).isChecked());
}

// 8 [   ][   ][   ][   ][   ][   ][   ][   ]
// 7 [   ][   ][ p ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][ p ][   ][   ][   ][   ]
// 5 [ K ][ P ][   ][   ][   ][   ][   ][ r ]
// 4 [   ][ R ][   ][   ][   ][ p ][ P ][ k ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][ P ][   ][   ][   ]
// 1 [   ][   ][   ][   ][   ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// sequence of moves: g4 fxg3 is illegal because it puts king in check.
TEST_F(MoveGeneratorFixture, PinnedPawn_Black_CanNotCaptureEnPassant)
{
    auto result = legalMoves("8/2p5/3p4/KP5r/1R3pPk/8/4P3/8 b - g3 0 1");
    EXPECT_EQ(Square::G3, testContext.readChessPosition().enPassant().readSquare());

    EXPECT_EQ(17u, result.size());
    for (auto mv : result) {
        if (mv.targetSqr() == Square::G3 && mv.sourceSqr() == Square::F4) {
            FAIL() << "There should be no moves to G3 from Black in this setup.";
        }
    }

    auto pawnMoves = collectMoves<Set::BLACK>(pieceType(PieceType::PAWN));
    EXPECT_TRUE(contains_exactly_target_squares(pawnMoves, Square::C6, Square::C5, Square::D5, Square::F3));
    EXPECT_EQ(0u, countIf(pawnMoves, isCapture));
}

// 8 [   ][   ][   ][   ][   ][   ][   ][   ]
// 7 [   ][   ][ p ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][ p ][   ][   ][   ][   ]
// 5 [ K ][ P ][   ][   ][   ][   ][   ][ r ]
// 4 [   ][ R ][   ][   ][ P ][ p ][   ][ k ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][   ]
// 1 [   ][   ][   ][   ][   ][   ][   ][   ]
//     A    B    C    D    E    F    G    H
// e4 fxe3 is illegal, it takes both pawns off the rank and exposes the king to the rook.
TEST_F(MoveGeneratorFixture, Pawn_SelfDiscoveryCheck_EnPassantCaptureIllegal)
{
    auto result = legalMoves("8/2p5/3p4/KP5r/1R2Pp1k/8/8/8 b - e3 0 1", pieceType(PieceType::PAWN));
    EXPECT_EQ(Square::E3, testContext.readChessPosition().enPassant().readSquare());

    EXPECT_TRUE(contains_exactly_target_squares(result, Square::C6, Square::C5, Square::D5, Square::F3));
}

TEST_F(MoveGeneratorFixture, PinnedPawn_Black_SimilarAsAboveButMorePieces)
{
    auto result = legalMoves("8/2p5/3p4/KP5r/1R2Pp1k/8/6P1/8 b - e3 0 1");
    EXPECT_EQ(Square::E3, testContext.readChessPosition().enPassant().readSquare());

    EXPECT_EQ(16u, result.size());
    for (auto mv : result) {
        if (mv.targetSqr() == Square::E3 && mv.sourceSqr() == Square::F4) {
            FAIL() << "There should be no moves to E3 from Black in this setup.";
        }
    }

    auto pawnMoves = collectMoves<Set::BLACK>(pieceType(PieceType::PAWN));
    EXPECT_EQ(4u, pawnMoves.size());
    EXPECT_EQ(0u, countIf(pawnMoves, isCapture));
}

TEST_F(MoveGeneratorFixture, PinnedPawn_White_CanNotCaptureEnPassantSinceItWouldPutUsInCheck)
{
    auto result = legalMoves("8/8/3p4/KPp4r/1R2Pp1k/8/6P1/8 w - c6 0 2");
    EXPECT_EQ(Square::C6, testContext.readChessPosition().enPassant().readSquare());

    EXPECT_EQ(13u, result.size());
    for (auto mv : result) {
        if (mv.targetSqr() == Square::C6) {
            FAIL() << "There should be no moves to C6 from White in this setup.";
        }
    }

    auto pawnMoves = collectMoves<Set::WHITE>(pieceType(PieceType::PAWN));
    EXPECT_EQ(4u, pawnMoves.size());
    EXPECT_EQ(0u, countIf(pawnMoves, isCapture));
}

TEST_F(MoveGeneratorFixture, Pawn_NotPinned_CanCaptureEnPassantWhileKingIsOnEPRank)
{
    auto result = legalMoves("8/8/8/K7/4Pp1k/8/6P1/8 b - e3 0 2", pieceType(PieceType::PAWN));
    EXPECT_EQ(Square::E3, testContext.readChessPosition().enPassant().readSquare());

    EXPECT_TRUE(contains_exactly_target_squares(result, Square::F3, Square::E3));
    EXPECT_EQ(1u, countIf(result, isEnPassant));
}

TEST_F(MoveGeneratorFixture, Pawn_NotPinned_CanCaptureEnPassantWhileKingIsOnEPRankMorePawnsVariation)
{
    auto result = legalMoves("8/8/8/K7/1R2PpPk/8/8/8 b - g3 0 2", pieceType(PieceType::PAWN));
    EXPECT_EQ(Square::G3, testContext.readChessPosition().enPassant().readSquare());

    EXPECT_TRUE(contains_exactly_target_squares(result, Square::F3, Square::G3));
    EXPECT_EQ(1u, countIf(result, isEnPassant));
}

TEST_F(MoveGeneratorFixture, Knight_Move_NothingSpecial)
{
    auto result = legalMoves("8/8/4n3/8/k7/8/8/5K2 b - - 0 1", from(Square::E6));

    EXPECT_EQ(8u, result.size());
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
 *     A    B    C    D    E    F    G    H
 * the king can't step back along the rank, a4 is still covered by the rook. */
TEST_F(MoveGeneratorFixture, KingCheckedByRook)
{
    auto result = legalMoves("8/8/8/8/1k5R/8/8/5K2 b - - 0 1");

    EXPECT_TRUE(contains_exactly_target_squares(result, Square::A3, Square::B3, Square::C3, Square::A5, Square::B5, Square::C5));
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
The pawn on e4 checks the king and dxe3 would capture it, but the pawn is pinned by the white rook. */
TEST_F(MoveGeneratorFixture, Pawn_DoubleMoveCheck_EnPassantCaptureNotAvailableBecauseOfPin)
{
    auto result = legalMoves("8/8/8/3k4/3pP3/8/8/1K1R2B1 b - e3 0 1");

    EXPECT_TRUE(sideToMoveIsChecked());
    EXPECT_EQ(7u, result.size());
    EXPECT_EQ(0u, countIf(result, from(Square::D4))) << "There shouldn't be any pawn moves amongst the available moves";
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
    // c6, c5, exd5, g5, b3, bxc3 & hxg2, d7 is pinned by the bishop.
    auto result = legalMoves("r3k2r/p1ppqpb1/bn2pnp1/1B1PN3/1p2P3/2N2Q1p/PPPB1PPP/R3K2R b KQkq - 1 1", pieceType(PieceType::PAWN));

    EXPECT_EQ(7u, result.size());
    EXPECT_EQ(0u, countIf(result, from(Square::D7)));
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
    auto result = legalMoves("8/8/1k6/2b5/3P4/2p2K2/8/8 b - - 2 2", from(Square::C3));

    EXPECT_TRUE(contains_exactly_target_squares(result, Square::C2));
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
    auto result = legalMoves("8/8/8/3k4/3n4/1B6/8/1K1R4 b - - 0 1", from(Square::D4));

    EXPECT_EQ(0u, result.size());
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
 * fen:  8/K6r/3p4/1Pp5/1R3p1k/8/4P1P1/8 w - c6 4 3
 * only king moves get out of the rook's check, bxc6 e.p. doesn't.  */
TEST_F(MoveGeneratorFixture, KingInCheck_White_SpecialCasedPawnMovementsNotAvailable)
{
    auto result = legalMoves("8/K6r/3p4/1Pp5/1R3p1k/8/4P1P1/8 w - c6 4 3");

    EXPECT_TRUE(contains_exactly_target_squares(result, Square::A8, Square::B8, Square::A6, Square::B6));
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
 * fen: 8/1k6/8/2Pp4/2K5/8/8/8 w - d6 0 1
 * cxd6 e.p. captures the checking pawn. */
TEST_F(MoveGeneratorFixture, PawnDoubleMoveCheck_White_EnPassantCaptureAvailable)
{
    auto result = legalMoves("8/1k6/8/2Pp4/2K5/8/8/8 w - d6 0 1");

    EXPECT_TRUE(sideToMoveIsChecked());
    EXPECT_EQ(8u, result.size());
    EXPECT_EQ(1u, countIf(result, isEnPassant));
}

/**
 * 8 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 7 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 6 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 5 [   ][   ][   ][ k ][   ][   ][   ][   ]
 * 4 [   ][   ][   ][ p ][ P ][   ][   ][   ]
 * 3 [   ][   ][   ][   ][ep ][   ][   ][   ]
 * 2 [   ][   ][   ][   ][   ][   ][   ][   ]
 * 1 [   ][ K ][   ][   ][ R ][   ][ B ][   ]
 *     A    B    C    D    E    F    G    H
 * 8/8/8/3k4/3pP3/8/8/1K2R1B1 b - e3 0 1
 * the pawn on e4 is guarded by the rook, dxe3 e.p. captures it. */
TEST_F(MoveGeneratorFixture, PawnDoubleMoveCheck_Black_EnPassantCaptureAvailableForPawn)
{
    auto result = legalMoves("8/8/8/3k4/3pP3/8/8/1K2R1B1 b - e3 0 1");

    EXPECT_TRUE(sideToMoveIsChecked());
    EXPECT_EQ(7u, result.size());
    EXPECT_EQ(1u, countIf(result, isEnPassant));
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
    auto result = legalMoves("3rk3/8/3Q4/8/5q2/8/3K4/8 w - - 3 3");

    // verify
    EXPECT_TRUE(WhiteMoveGen(testContext.readChessPosition(), testParams).isChecked());
    EXPECT_FALSE(BlackMoveGen(testContext.readChessPosition(), testParams).isChecked());
    EXPECT_EQ(6u, result.size());
    EXPECT_EQ(0u, countIf(result, from(Square::D6)));
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
    auto result = legalMoves("8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1");

    EXPECT_FALSE(WhiteMoveGen(testContext.readChessPosition(), testParams).isChecked());
    EXPECT_FALSE(BlackMoveGen(testContext.readChessPosition(), testParams).isChecked());

    // verify
    EXPECT_EQ(15u, result.size());
    EXPECT_EQ(1u, countIf(result, isEnPassant));
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
3r1k2/8/3N4/5q2/8/3K4/8/8 w - - 0 1 */
TEST_F(MoveGeneratorFixture, Knight_Pinned_CanNotCaptureCheckingPiece)
{
    auto result = legalMoves("3r1k2/8/3N4/5q2/8/3K4/8/8 w - - 0 1");

    // verify
    EXPECT_TRUE(WhiteMoveGen(testContext.readChessPosition(), testParams).isChecked());
    EXPECT_FALSE(BlackMoveGen(testContext.readChessPosition(), testParams).isChecked());
    EXPECT_EQ(6u, result.size());
    EXPECT_EQ(0u, countIf(result, from(Square::D6))) << "There shouldn't be any knight moves amongst the available moves";
}

TEST_F(MoveGeneratorFixture, Knight_Pinned_CanNotMoveFromOnePinToAnother)
{
    auto result = legalMoves("6k1/8/8/b7/1R6/8/8/q2NK3 w - - 0 1");

    EXPECT_EQ(4u, result.size());
    EXPECT_EQ(4u, countIf(result, from(Square::E1)));
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
5k2/8/3N4/5q2/8/3K4/8/8 w - - 0 1
the knight can capture the queen or block on e4. */
TEST_F(MoveGeneratorFixture, Knight_CaptureCheckingQueen_ShouldOnlyHaveOneMoveAvailable)
{
    auto result = legalMoves("5k2/8/3N4/5q2/8/3K4/8/8 w - - 0 1");

    // verify
    EXPECT_TRUE(WhiteMoveGen(testContext.readChessPosition(), testParams).isChecked());
    EXPECT_FALSE(BlackMoveGen(testContext.readChessPosition(), testParams).isChecked());
    EXPECT_EQ(8u, result.size());

    auto knightMoves = legalMoves("5k2/8/3N4/5q2/8/3K4/8/8 w - - 0 1", from(Square::D6));
    EXPECT_TRUE(contains_exactly_target_squares(knightMoves, Square::F5, Square::E4))
        << "There should only exist two knight moves amongst the available moves";
}

// 8 [ R ][   ][   ][   ][ k ][   ][   ][ r ]
// 7 [   ][ b ][   ][   ][   ][   ][ b ][ q ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [   ][   ][   ][   ][   ][   ][   ][ B ]
// 1 [   ][   ][   ][   ][ K ][   ][   ][ R ]
//     A    B    C    D    E    F    G    H
// R3k2r/1b4bq/8/8/8/8/7B/4K2R b Kk - 0 1
// after Rxa8+ the bishop on b7 has two moves, Bc8 & Bxa8.
TEST_F(MoveGeneratorFixture, Bishop_KingInCheck_BlockingOrCapturingCheckingPiece)
{
    auto result = legalMoves("R3k2r/1b4bq/8/8/8/8/7B/4K2R b Kk - 0 1", from(Square::B7));

    EXPECT_EQ(WHITEROOK, testContext.readChessPosition().pieceAt(Square::A8));
    EXPECT_TRUE(contains_exactly_target_squares(result, Square::C8, Square::A8));
}

// 8 [   ][   ][   ][   ][ k ][   ][   ][   ]
// 7 [   ][   ][   ][   ][   ][   ][   ][   ]
// 6 [   ][   ][   ][   ][ p ][   ][   ][   ]
// 5 [   ][   ][   ][ P ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][ p ]
// 2 [   ][   ][   ][   ][ b ][   ][ P ][   ]
// 1 [   ][   ][   ][   ][   ][ K ][   ][   ]
//     A    B    C    D    E    F    G    H
// 4k3/8/4p3/3P4/8/7p/4b1P1/5K2 w - - 0 2
TEST_F(MoveGeneratorFixture, Pawn_PawnCapturesDuringCheck_IdentifiedABugWherePawnsWereAllowedToCaptureDespiteBeingInCheck)
{
    auto result = legalMoves("4k3/8/4p3/3P4/8/7p/4b1P1/5K2 w - - 0 2", pieceType(PieceType::PAWN));

    // just make sure setup is correct
    PositionReader pos = testContext.readChessPosition();
    EXPECT_EQ(WHITEPAWN, pos.pieceAt(Square::D5));
    EXPECT_EQ(BLACKPAWN, pos.pieceAt(Square::E6));
    EXPECT_EQ(BLACKBISHOP, pos.pieceAt(Square::E2));
    EXPECT_EQ(WHITEKING, pos.pieceAt(Square::F1));

    // verify
    EXPECT_TRUE(result.empty());
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
    auto result = legalMoves("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");

    EXPECT_EQ(48u, result.size());
    EXPECT_EQ(8u, countIf(result, isCapture));
    EXPECT_EQ(2u, countIf(result, isCastling));
}

}  // namespace ElephantTest
