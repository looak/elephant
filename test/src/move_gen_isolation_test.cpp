#include <gtest/gtest.h>
#include <position/position.hpp>
#include <position/position_accessors.hpp>

#include <move/generation/move_bulk_generator.hpp>
#include <move/generation/move_gen_isolation.hpp>

#include "bitboard_test_helpers.hpp"

namespace ElephantTest {
////////////////////////////////////////////////////////////////

/**
 * @file position_test.cpp
 * @brief Fixture for testing board position and functionality around it.
 * Naming convention as of November 2023: <Piece>_<TestedFunctionality>_<ExpectedResult>
 * @author Alexander Loodin Ek
 */
class IsolationFixture : public ::testing::Test {
public:

    KingPinThreats<Set::WHITE> safeWhiteKingPinThreats(Square sqr = Square::E1) {
        return KingPinThreats<Set::WHITE>(sqr, testingPosition);
    }

    KingPinThreats<Set::BLACK> safeBlackKingPinThreats(Square sqr = Square::E8) {
        return KingPinThreats<Set::BLACK>(sqr, testingPosition);
    }

    Position testingPosition;
};

// TODO: Add Ascii chessboard comment
TEST_F(IsolationFixture, Pawn_IsolatingPiece_PawnOnDifferentRankAndFile)
{
    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces(
        piece_constants::white_pawn, Square::D4,
        piece_constants::white_pawn, Square::C5);

    // generate moves
    BulkMoveGenerator moveGen(testingPosition);
    Bitboard movesbb = moveGen.computeBulkPawnMoves<Set::WHITE>();
    
    // expected
    Bitboard expected = BitboardResultFactory::buildBoardFromAscii({
        " . . . . . . . . ",    // 8
        " . . . . . . . . ",    // 7
        " . . . . . . . . ",    // 6
        " . . . x . . . . ",    // 5
        " . . . . . . . . ",    // 4
        " . . . . . . . . ",    // 3
        " . . . . . . . . ",    // 2 
        " . . . . . . . . ",    // 1
     //   A B C D E F G H
    });

    // do isolation
    PieceIsolator<Set::WHITE, pawnId> isolator(testingPosition, movesbb, safeWhiteKingPinThreats());
    auto [quiet, captures] = isolator.isolate(Square::D4);

    // validate
    EXPECT_EQ(expected, quiet);
    EXPECT_TRUE(captures.empty());

}

// TODO: Add Ascii chessboard comment
TEST_F(IsolationFixture, Pawn_IsolatingPiece_PawnOnSameRank)
{
    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces(
        piece_constants::white_pawn, Square::D4,
        piece_constants::white_pawn, Square::C4);

    // generate moves
    BulkMoveGenerator moveGen(testingPosition);
    Bitboard movesbb = moveGen.computeBulkPawnMoves<Set::WHITE>();

    // expected
    Bitboard expectedD5 = BitboardResultFactory::buildExpectedBoard(Square::D5);
    Bitboard expectedC4 = BitboardResultFactory::buildExpectedBoard(Square::C5);

    // do isolation
    PieceIsolator<Set::WHITE, pawnId> isolator(testingPosition, movesbb, safeWhiteKingPinThreats());
    auto [quietD4, capturesD4] = isolator.isolate(Square::D4);
    auto [quietC4, capturesC4] = isolator.isolate(Square::C4);

    // validate
    EXPECT_EQ(expectedD5, quietD4);
    EXPECT_TRUE(capturesD4.empty());

    EXPECT_EQ(expectedC4, quietC4);
    EXPECT_TRUE(capturesC4.empty());
}

// TODO: Add Ascii chessboard comment
TEST_F(IsolationFixture, Pawn_IsolatingPiece_PawnOnSameFile)
{
    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces(
        piece_constants::white_pawn, Square::D2,
        piece_constants::white_pawn, Square::D5);

    // generate moves
    BulkMoveGenerator moveGen(testingPosition);
    Bitboard movesbb = moveGen.computeBulkPawnMoves<Set::WHITE>();

    // expected
    Bitboard expectedD2 = BitboardResultFactory::buildExpectedBoard(Square::D3, Square::D4);
    Bitboard expectedD5 = BitboardResultFactory::buildExpectedBoard(Square::D6);

    // do isolation
    PieceIsolator<Set::WHITE, pawnId> isolator(testingPosition, movesbb, safeWhiteKingPinThreats());
    auto [quietD2, capturesD2] = isolator.isolate(Square::D2);
    auto [quietD5, capturesD5] = isolator.isolate(Square::D5);

    // validate
    EXPECT_EQ(expectedD2, quietD2);
    EXPECT_TRUE(capturesD2.empty());

    EXPECT_EQ(expectedD5, quietD5);
    EXPECT_TRUE(capturesD5.empty());
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ k ][ p ][ . ][ R ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(IsolationFixture, Pawn_BlackPinnedPiece_NoAvailableMovesSinceItsPinned)
{
    // black pawn is pinned by white rook on h7 and shouldn't be able to move.
    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces(
        piece_constants::black_pawn, Square::F7,
        piece_constants::black_king, Square::E7,
        piece_constants::white_rook, Square::H7);

    KingPinThreats<Set::BLACK> kpt(Square::E7, testingPosition);

    // generate moves
    BulkMoveGenerator moveGen(testingPosition);
    Bitboard movesbb = moveGen.computeBulkPawnMoves<Set::BLACK>();

    // do isolation
    PieceIsolator<Set::BLACK, pawnId> isolator(testingPosition, movesbb, kpt);

    auto [nonattacks, attacks] = isolator.isolate(Square::F7);

    // validate
    u64 expected = 0x0;
    EXPECT_EQ(expected, nonattacks.read());
    EXPECT_EQ(expected, attacks.read());
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ P ][ . ][ K ][ P ][ . ][ r ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(IsolationFixture, Pawn_WhitePinnedPiece_NoAvailableMovesSinceItsPinned)
{
    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces(
        piece_constants::white_pawn, Square::F5,
        piece_constants::white_pawn, Square::C5,
        piece_constants::white_king, Square::E5,
        piece_constants::black_rook, Square::H5);

    KingPinThreats<Set::WHITE> kpt(Square::E5, testingPosition);

    // generate moves
    BulkMoveGenerator moveGen(testingPosition);
    Bitboard movesbb = moveGen.computeBulkPawnMoves<Set::WHITE>();

    // do isolation
    PieceIsolator<Set::WHITE, pawnId> isolator(testingPosition, movesbb, kpt);

    {
        auto [quiets, captures] = isolator.isolate(Square::F5);

        // validate
        u64 expected = 0x0;
        EXPECT_EQ(expected, quiets.read());
        EXPECT_EQ(expected, captures.read());
    }

    Bitboard expected = BitboardResultFactory::buildExpectedBoard(Square::C6);

    {
        auto [quiets, captures] = isolator.isolate(Square::C5);

        // validate        
        EXPECT_EQ(expected, quiets);
        EXPECT_TRUE(captures.empty());
    }
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ p ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ k ][ . ][ . ][ R ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(IsolationFixture, Pawn_KingMask_PawnCanMoveIntoBlockingCheckButNoFurther)
{
    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces(
        piece_constants::black_king, Square::E6,
        piece_constants::black_pawn, Square::F7,
        piece_constants::white_rook, Square::H6);

    KingPinThreats<Set::BLACK> kpt(toSquare(editor.material().blackKing().lsbIndex()), testingPosition);

    // expected
    Bitboard expected = BitboardResultFactory::buildExpectedBoard(Square::F6); // but not F7.

    // generate moves
    BulkMoveGenerator moveGen(testingPosition);
    Bitboard movesbb = moveGen.computeBulkPawnMoves<Set::BLACK>();

    // do isolation
    PieceIsolator<Set::BLACK, pawnId> isolator(testingPosition, movesbb, kpt);
    auto [quiet, captures] = isolator.isolate(Square::F7);

    // validate
    EXPECT_EQ(expected, quiet);
    EXPECT_EQ(Bitboard(0x0), captures);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ p ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ k ][ . ][ R ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(IsolationFixture, Pawn_KingMask_CanCaptureCheckingPiece)
{
    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces(
        piece_constants::black_king, Square::E6,
        piece_constants::black_pawn, Square::F7,
        piece_constants::white_rook, Square::G6);

    KingPinThreats<Set::BLACK> kpt(toSquare(editor.material().blackKing().lsbIndex()), testingPosition);

    // expected
    Bitboard quietExpected = BitboardResultFactory::buildExpectedBoard(Square::F6); // but not F7.
    Bitboard captureExpected = BitboardResultFactory::buildExpectedBoard(Square::G6);

    // generate moves
    BulkMoveGenerator moveGen(testingPosition);
    Bitboard movesbb = moveGen.computeBulkPawnMoves<Set::BLACK>();

    // do isolation
    PieceIsolator<Set::BLACK, pawnId> isolator(testingPosition, movesbb, kpt);
    auto [quiet, captures] = isolator.isolate(Square::F7);

    // validate
    EXPECT_EQ(quietExpected, quiet);
    EXPECT_EQ(captureExpected, captures);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ p ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ k ][ . ][ R ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(IsolationFixture, Pawn_KingMask_CanOnlyDoubleMoveToBlockCheck)
{
    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces(
        piece_constants::black_king, Square::E5,
        piece_constants::black_pawn, Square::F7,
        piece_constants::white_rook, Square::G5);

    KingPinThreats<Set::BLACK> kpt(toSquare(editor.material().blackKing().lsbIndex()), testingPosition);

    // expected
    Bitboard quietExpected = BitboardResultFactory::buildExpectedBoard(Square::F5); // but not F6;
    Bitboard captureExpected(0x0);

    // generate moves
    BulkMoveGenerator moveGen(testingPosition);
    Bitboard movesbb = moveGen.computeBulkPawnMoves<Set::BLACK>();

    // do isolation
    PieceIsolator<Set::BLACK, pawnId> isolator(testingPosition, movesbb, kpt);
    auto [quiet, captures] = isolator.isolate(Square::F7);

    // validate
    EXPECT_EQ(quietExpected, quiet);
    EXPECT_EQ(captureExpected, captures);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ b ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ P ][ . ][ . ]
// 5 [ . ][ . ][ P ][ . ][ K ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(IsolationFixture, Pawn_PinnedPieceWhite_CanCapturePinningBishop)
{
    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces(      
        piece_constants::black_bishop, Square::G7,
        piece_constants::white_king, Square::E5,
        piece_constants::white_pawn, Square::C5,
        piece_constants::white_pawn, Square::F6
    );

    KingPinThreats<Set::WHITE> kpt(Square::E5, testingPosition);

    // expected
    Bitboard quietExpected(0x0);
    Bitboard captureExpected(0x0);
    captureExpected[Square::G7] = true;

    // generate moves
    BulkMoveGenerator moveGen(testingPosition);
    Bitboard pawnMoves = moveGen.computeBulkPawnMoves<Set::WHITE>();

    // do isolation
    PieceIsolator<Set::WHITE, pawnId> isolator(testingPosition, pawnMoves, kpt);
    auto [quiet, captures] = isolator.isolate(Square::F6);

    // validate
    EXPECT_EQ(quietExpected, quiet);
    EXPECT_EQ(captureExpected, captures);

    // expected C5
    Bitboard expectedC5 = BitboardResultFactory::buildExpectedBoard(Square::C6);

    // do isolation
    auto [quietC5, capturesC5] = isolator.isolate(Square::C5);

    // validate
    EXPECT_EQ(expectedC5, quietC5);
    EXPECT_EQ(Bitboard(0x0), capturesC5);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ N ][ . ][ . ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ N ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(IsolationFixture, Knight_IsolatingPiece_TwoKnightsNotSharingSquaresButBlockingEachOther)
{
    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces(
        piece_constants::white_knight, Square::D4,
        piece_constants::white_knight, Square::E2
    );

    // expected moves
    Bitboard expected = BitboardResultFactory::buildBoardFromAscii({
        " . . . . . . . . ",    // 8
        " . . . . . . . . ",    // 7
        " . . x . x . . . ",    // 6
        " . x . . . x . . ",    // 5
        " . . . . . x . . ",    // 4
        " . x x . . x x . ",    // 3
        " . . x . . . . . ",    // 2 
        " . . x . . . x . ",    // 1
     //   A B C D E F G H
    });

    // generate moves
    BulkMoveGenerator moveGen(testingPosition);
    Bitboard knightMoves = moveGen.computeBulkKnightMoves<Set::WHITE>();

    // verify moves
    EXPECT_EQ(knightMoves, expected);

    // expected isolation
    Bitboard empty(0x0);
    expected = BitboardResultFactory::buildBoardFromAscii({
        " . . . . . . . . ",    // 8
        " . . . . . . . . ",    // 7
        " . . x . x . . . ",    // 6
        " . x . . . x . . ",    // 5
        " . . . . . . . . ",    // 4
        " . x . . . x . . ",    // 3
        " . . x . . . . . ",    // 2 
        " . . . . . . . . ",    // 1
     //   A B C D E F G H
    });

    // do isolation
    PieceIsolator<Set::WHITE, knightId> isolator(testingPosition, knightMoves, safeWhiteKingPinThreats());
    auto [quiet, captures] = isolator.isolate(Square::D4);

    // validate
    EXPECT_EQ(expected, quiet);
    EXPECT_EQ(empty, captures);

    // expected E2
    expected = BitboardResultFactory::buildBoardFromAscii({
        " . . . . . . . . ",    // 8
        " . . . . . . . . ",    // 7
        " . . . . . . . . ",    // 6
        " . . . . . . . . ",    // 5
        " . . . . . x . . ",    // 4
        " . . x . . . x . ",    // 3
        " . . . . . . . . ",    // 2 
        " . . x . . . x . ",    // 1
     //   A B C D E F G H
    });

    // do isolation
    auto [quietE2, capturesE2] = isolator.isolate(Square::E2);

    // validate
    EXPECT_EQ(expected, quietE2);
    EXPECT_EQ(empty, capturesE2);
}

// 8 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 7 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 6 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 5 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 4 [ . ][ . ][ . ][ N ][ . ][ F ][ . ][ . ]
// 3 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 2 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
// 1 [ . ][ . ][ . ][ . ][ . ][ . ][ . ][ . ]
//     A    B    C    D    E    F    G    H
TEST_F(IsolationFixture, Knight_IsolatingPiece_SharingTargetSquares)
{
    // setup
    PositionEditor editor(testingPosition);
    editor.placePieces(
        piece_constants::white_knight, Square::D4,
        piece_constants::white_knight, Square::F4
    );

    // expected moves
    Bitboard expected = BitboardResultFactory::buildBoardFromAscii({
        " . . . . . . . . ",    // 8
        " . . . . . . . . ",    // 7
        " . . x . x . x . ",    // 6
        " . x . x . x . x ",    // 5
        " . . . . . . . . ",    // 4
        " . x . x . x . x ",    // 3
        " . . x . x . x . ",    // 2 
        " . . . . . . . . ",    // 1
     //   A B C D E F G H
    });

    // generate moves
    BulkMoveGenerator moveGen(testingPosition);
    Bitboard knightMoves = moveGen.computeBulkKnightMoves<Set::WHITE>();

    // validate moves
    EXPECT_EQ(expected, knightMoves);

    // expected isolated D4
    expected = BitboardResultFactory::buildBoardFromAscii({
        " . . . . . . . . ",    // 8
        " . . . . . . . . ",    // 7
        " . . x . x . . . ",    // 6
        " . x . . . x . . ",    // 5
        " . . . . . . . . ",    // 4
        " . x . . . x . . ",    // 3
        " . . x . x . . . ",    // 2 
        " . . . . . . . . ",    // 1
     //   A B C D E F G H
    });

    Bitboard empty{};

    // do isolation
    PieceIsolator<Set::WHITE, knightId> isolator(testingPosition, knightMoves, safeWhiteKingPinThreats());
    auto [quiet, captures] = isolator.isolate(Square::D4);

    // verify
    EXPECT_EQ(expected, quiet);
    EXPECT_EQ(empty, captures);

    // expected F4
    expected = BitboardResultFactory::buildBoardFromAscii({
        " . . . . . . . . ",    // 8
        " . . . . . . . . ",    // 7
        " . . . . x . x . ",    // 6
        " . . . x . . . x ",    // 5
        " . . . . . . . . ",    // 4
        " . . . x . . . x ",    // 3
        " . . . . x . x . ",    // 2 
        " . . . . . . . . ",    // 1
     //   A B C D E F G H
    });

     // isolate F4
    auto [quietF4, capturesF4] = isolator.isolate(Square::F4);
    EXPECT_EQ(expected, quietF4);
    EXPECT_EQ(empty, capturesF4);
}


} // namespace ElephantTest