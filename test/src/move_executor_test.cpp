#include <gtest/gtest.h>

#include <chessboard.h>
#include <game_context.h>
#include <move/move_executor.hpp>
#include <position/position.hpp>


#include "chess_positions.hpp"

namespace ElephantTest {
////////////////////////////////////////////////////////////////

/**
 * @file move_executor_test.cpp
 * @brief Testing all things move executioner, make, unmake, make(vargs), etc.
 * Naming convention as of October 2023: <TestedFunctionality>_<ExpectedResult>
 * @author Alexander Loodin Ek  */
class MoveExecutorFixture : public ::testing::Test {
public:
    Position testingPosition;
    GameState gameState;
    GameHistory gameHistory;
};
////////////////////////////////////////////////////////////////


TEST_F(MoveExecutorFixture, MakeValidMove_E2E4_UpdatesBoard) {
    
    // set up    
    chess_positions::defaultStartingPosition(testingPosition);
    MoveExecutor executor(testingPosition.edit(), gameState, gameHistory);
    PositionReader positionReader(testingPosition);

    PackedMove move(Square::E2, Square::E4);

    // do
    executor.makeMove<true>(move);

    // verify
    EXPECT_EQ(positionReader.pieceAt(Square::E4), piece_constants::white_pawn);
    EXPECT_EQ(positionReader.pieceAt(Square::E2), piece_constants::null());
}

// 8 [ r ][ n ][ b ][ q ][ k ][ b ][ n ][ r ]
// 7 [ p ][ p ][ p ][   ][ p ][ p ][ p ][ p ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][ p ][ P ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [ P ][ P ][   ][   ][ P ][ P ][ P ][ P ]
// 1 [ R ][ N ][ B ][ Q ][ K ][ B ][ N ][ R ]
//     A    B    C    D    E    F    G    H
// 1.d4 d5 2.c4 dxc4
TEST_F(MoveExecutorFixture, BuildMoveSequence_QueensGambitAccepted)
{
    std::string pgn = "1.d4 d5 2.c4 dxc4";

    std::vector<Move> moves;
    Move::ParsePGN(pgn, moves);
}

// // https://en.wikipedia.org/wiki/Portable_Game_Notation
// // 1. e4 e5 2. Nf3 Nc6 3. Bb5 a6 {This opening is called the Ruy Lopez.}
// // 4. Ba4 Nf6 5. O - O Be7
// TEST_F(MoveFixture, BuildMoveSequence_FischerSpassky)
// {
//     std::string pgn = "1. e4 e5 2. Nf3 Nc6 3. Bb5 a6 4. Ba4 Nf6";
//     std::vector<Move> moves;
//     Move::ParsePGN(pgn, moves);

//     EXPECT_EQ(moves.size(), 8);

//     {
//         const int index = 0;
//         const auto& mv = moves[index];
//         EXPECT_EQ(WHITEPAWN, mv.Piece);
//         EXPECT_EQ(e4, mv.TargetSquare);
//         EXPECT_EQ(&moves[index + 1], mv.NextMove);
//         EXPECT_EQ(nullptr, mv.PrevMove);
//     }

//     {
//         const int index = 1;
//         const auto& mv = moves[index];
//         EXPECT_EQ(BLACKPAWN, mv.Piece);
//         EXPECT_EQ(e5, mv.TargetSquare);
//         EXPECT_EQ(&moves[index + 1], mv.NextMove);
//         EXPECT_EQ(&moves[index - 1], mv.PrevMove);
//     }

//     {
//         const int index = 2;
//         const auto& mv = moves[index];
//         EXPECT_EQ(WHITEKNIGHT, mv.Piece);
//         EXPECT_EQ(f3, mv.TargetSquare);
//         EXPECT_EQ(&moves[index + 1], mv.NextMove);
//         EXPECT_EQ(&moves[index - 1], mv.PrevMove);
//     }

//     {
//         const int index = 3;
//         const auto& mv = moves[index];
//         EXPECT_EQ(BLACKKNIGHT, mv.Piece);
//         EXPECT_EQ(c6, mv.TargetSquare);
//         EXPECT_EQ(&moves[index + 1], mv.NextMove);
//         EXPECT_EQ(&moves[index - 1], mv.PrevMove);
//     }

//     {
//         const int index = 4;
//         const auto& mv = moves[index];
//         EXPECT_EQ(WHITEBISHOP, mv.Piece);
//         EXPECT_EQ(b5, mv.TargetSquare);
//         EXPECT_EQ(&moves[index + 1], mv.NextMove);
//         EXPECT_EQ(&moves[index - 1], mv.PrevMove);
//     }

//     {
//         const int index = 5;
//         const auto& mv = moves[index];
//         EXPECT_EQ(BLACKPAWN, mv.Piece);
//         EXPECT_EQ(a6, mv.TargetSquare);
//         EXPECT_EQ(&moves[index + 1], mv.NextMove);
//         EXPECT_EQ(&moves[index - 1], mv.PrevMove);
//     }

//     {
//         const int index = 6;
//         const auto& mv = moves[index];
//         EXPECT_EQ(WHITEBISHOP, mv.Piece);
//         EXPECT_EQ(a4, mv.TargetSquare);
//         EXPECT_EQ(&moves[index + 1], mv.NextMove);
//         EXPECT_EQ(&moves[index - 1], mv.PrevMove);
//     }

//     {
//         const int index = 7;
//         const auto& mv = moves[index];
//         EXPECT_EQ(BLACKKNIGHT, mv.Piece);
//         EXPECT_EQ(f6, mv.TargetSquare);
//         EXPECT_EQ(nullptr, mv.NextMove);
//         EXPECT_EQ(&moves[index - 1], mv.PrevMove);
//     }
// }
// TEST_F(MoveFixture, BuildMoveSequence_FischerSpassky_CommentAndCastling)
// {
//     std::string pgn =
//         "1. e4 { This is a comment. }e5 2. O-O-O O-O 3. Bxb5 a6{ This opening is called the Ruy "
//         "Lopez. } 4. Ba4+ Nf6 5. O-O Be7 6. de5 Bxe5+";
//     std::vector<Move> moves;
//     auto comments = Move::ParsePGN(pgn, moves);

//     EXPECT_EQ(2, comments.size());
//     EXPECT_EQ(12, moves.size());
//     EXPECT_EQ(MoveFlag::Castle, moves[2].Flags & MoveFlag::Castle);
//     EXPECT_EQ(MoveFlag::Castle, moves[3].Flags & MoveFlag::Castle);
//     EXPECT_EQ(MoveFlag::Castle, moves[8].Flags & MoveFlag::Castle);
//     EXPECT_EQ(MoveFlag::Castle, moves[3].Flags & MoveFlag::Castle);

//     EXPECT_EQ(MoveFlag::Capture, moves[4].Flags & MoveFlag::Capture);
//     EXPECT_EQ(MoveFlag::Capture, moves[10].Flags & MoveFlag::Capture);
//     EXPECT_EQ(MoveFlag::Capture, moves[11].Flags & MoveFlag::Capture);

//     EXPECT_EQ(MoveFlag::Check, moves[6].Flags & MoveFlag::Check);
//     EXPECT_EQ(MoveFlag::Check, moves[11].Flags & MoveFlag::Check);

//     EXPECT_EQ(WHITEPAWN, moves[10].Piece);
// }

// TEST_F(MoveFixture, BuildMoveSequence_DisambiguatingMoves)
// {
//     std::string pgn = "9. h3 R1a3 10. d4 Nbd7 23. Rae8 Ne5 24. Qh4e1 25. e1 e2 26. Ra1xa2+ e2";
//     std::vector<Move> moves;
//     Move::ParsePGN(pgn, moves);

//     {  // h3
//         const int index = 0;
//         const auto& mv = moves[index];
//         EXPECT_EQ(WHITEPAWN, mv.Piece);
//         EXPECT_EQ(h3, mv.TargetSquare);
//     }

//     {  // R1a3
//         const int index = 1;
//         const auto& mv = moves[index];
//         EXPECT_EQ(BLACKROOK, mv.Piece);
//         EXPECT_EQ(a3, mv.TargetSquare);
//         EXPECT_EQ(Notation(9, 0), mv.SourceSquare);
//     }

//     {  // d4
//         const int index = 2;
//         const auto& mv = moves[index];
//         EXPECT_EQ(WHITEPAWN, mv.Piece);
//         EXPECT_EQ(d4, mv.TargetSquare);
//     }

//     {  // Nbd7
//         const int index = 3;
//         const auto& mv = moves[index];
//         EXPECT_EQ(BLACKKNIGHT, mv.Piece);
//         EXPECT_EQ(d7, mv.TargetSquare);
//         EXPECT_EQ(Notation(1, 9), mv.SourceSquare);
//     }

//     {  // Rae8
//         const int index = 4;
//         const auto& mv = moves[index];
//         EXPECT_EQ(WHITEROOK, mv.Piece);
//         EXPECT_EQ(e8, mv.TargetSquare);
//         EXPECT_EQ(Notation(0, 9), mv.SourceSquare);
//     }

//     {  // Ne5
//         const int index = 5;
//         const auto& mv = moves[index];
//         EXPECT_EQ(BLACKKNIGHT, mv.Piece);
//         EXPECT_EQ(e5, mv.TargetSquare);
//     }

//     {  // Qh4e1
//         const int index = 6;
//         const auto& mv = moves[index];
//         EXPECT_EQ(WHITEQUEEN, mv.Piece);
//         EXPECT_EQ(e1, mv.TargetSquare);
//         EXPECT_EQ(h4, mv.SourceSquare);
//     }

//     {  // e1
//         const int index = 7;
//         const auto& mv = moves[index];
//         EXPECT_EQ(WHITEPAWN, mv.Piece);
//         EXPECT_EQ(e1, mv.TargetSquare);
//     }

//     {  // e2
//         const int index = 8;
//         const auto& mv = moves[index];
//         EXPECT_EQ(BLACKPAWN, mv.Piece);
//         EXPECT_EQ(e2, mv.TargetSquare);
//     }

//     {  // Ra1xa2+
//         const int index = 9;
//         const auto& mv = moves[index];
//         EXPECT_EQ(WHITEROOK, mv.Piece);
//         EXPECT_EQ(a2, mv.TargetSquare);
//         EXPECT_EQ(a1, mv.SourceSquare);

//         EXPECT_EQ(MoveFlag::Check, moves[index].Flags & MoveFlag::Check);
//         EXPECT_EQ(MoveFlag::Capture, moves[index].Flags & MoveFlag::Capture);
//     }

//     {  // e2
//         const int index = 10;
//         const auto& mv = moves[index];
//         EXPECT_EQ(BLACKPAWN, mv.Piece);
//         EXPECT_EQ(e2, mv.TargetSquare);
//     }
// }