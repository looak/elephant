// #include <gtest/gtest.h>

// #include <core/chessboard.hpp>
// #include "elephant_test_utils.h"
// #include <io/fen_parser.hpp>
// #include <core/game_context.hpp>
// #include <move/move.hpp>

// namespace ElephantTest
// {
// ////////////////////////////////////////////////////////////////

// /**
//  * @file checkmate_test.cpp
//  * @brief Fixture for Check, Checkmates and Stalemates tests. Endgame tests.
//  * Naming convention: <TestedFunctionality>_<TestedColor>_<ExpectedResult>
//  * @author Alexander Loodin Ek *
//  */

// class CheckmateFixture : public ::testing::Test
// {
// public:
//     virtual void SetUp()
//     {
//         SetupDefaultStartingPosition(m_defaultStartingPosition);
//     };
//     virtual void TearDown() {};

//     Chessboard m_emptyChessboard; // by default a board should start empty.
//     Chessboard m_defaultStartingPosition;
//     GameContext m_context;

// private:

// };

// ////////////////////////////////////////////////////////////////

// // 8 [ ][ ][ ][q][k][ ][ ][ ]
// // 7 [ ][ ][ ][ ][ ][ ][ ][ ]
// // 6 [ ][ ][ ][ ][ ][ ][ ][ ]
// // 5 [ ][ ][ ][ ][ ][ ][ ][ ]
// // 4 [ ][ ][ ][ ][ ][ ][ ][ ]
// // 3 [ ][ ][ ][ ][ ][ ][ ][ ]
// // 2 [ ][ ][ ][ ][ ][P][P][P]
// // 1 [ ][ ][ ][R][ ][ ][K][ ]
// //    A  B  C  D  E  F  G  H
// // 3qk3/8/8/8/8/8/5PPP/3R2K1 b - - 0 1
// // Black to move, checkmate in 1 move.
// TEST_F(CheckmateFixture, BackRankCheckmate_Black_Checkmate)
// {
//     std::string fen("3qk3/8/8/8/8/8/5PPP/3R2K1 b - - 0 1");
// 	FENParser::deserialize(fen.c_str(), m_context);

//     Chessboard& boardEditor = m_context.editChessboard();

//     EXPECT_FALSE(m_context.readChessboard().isCheckmated(Set::WHITE));
//     EXPECT_FALSE(m_context.readChessboard().isCheckmated(Set::BLACK));

//     Move move(d8, d1);
//     EXPECT_TRUE(boardEditor.MakeMove(move));

//     EXPECT_TRUE(move.isCheckmate());

//     EXPECT_TRUE(m_context.readChessboard().isCheckmated(Set::WHITE));
//     EXPECT_FALSE(m_context.readChessboard().isCheckmated(Set::BLACK));
//     EXPECT_FALSE(m_context.readChessboard().isStalemated(Set::WHITE));
//     EXPECT_FALSE(m_context.readChessboard().isStalemated(Set::BLACK));
// }

// // 8 [k][ ][ ][ ][ ][ ][ ][ ]
// // 7 [ ][ ][Q][ ][ ][ ][ ][ ]
// // 6 [ ][K][ ][ ][ ][ ][ ][ ]
// // 5 [ ][ ][ ][ ][ ][ ][ ][ ]
// // 4 [ ][ ][ ][ ][ ][ ][ ][ ]
// // 3 [ ][ ][ ][ ][ ][ ][ ][ ]
// // 2 [ ][ ][ ][ ][ ][ ][ ][ ]
// // 1 [ ][ ][ ][ ][ ][ ][ ][ ]
// //    A  B  C  D  E  F  G  H
// // k7/2Q5/1K6/8/8/8/8/8 b - - 0 1
// // Black to move but they're in stalemate.

// TEST_F(CheckmateFixture, BlackToMoveInStalemate_Black_Stalemate)
// {
//     /**
//     * @brief Test case to verify the correct detection of a stalemate situation for the black side when it is black's turn to
//     move.
//     * The test uses a specific chess position provided in FEN notation and asserts the expected outcomes for both sides
//     * in terms of checkmate and stalemate status.
//     */
//     std::string fen("k7/2Q5/1K6/8/8/8/8/8 b - - 0 1");
//     FENParser::deserialize(fen.c_str(), m_context);

//     EXPECT_FALSE(m_context.readChessboard().isCheckmated(Set::WHITE));
//     EXPECT_FALSE(m_context.readChessboard().isCheckmated(Set::BLACK));
//     EXPECT_FALSE(m_context.readChessboard().isStalemated(Set::WHITE));
//     EXPECT_TRUE(m_context.readChessboard().isStalemated(Set::BLACK));
// }

// // TEST_F(CheckmateFixture, )

// } // namespace ElephantTest
