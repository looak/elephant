#include <gtest/gtest.h>

#include "game_context.h"
#include "chessboard.h"
#include "elephant_test_utils.h"
#include "fen_parser.h"

namespace ElephantTest
{
////////////////////////////////////////////////////////////////

/**
 * @file checkmate_test.cpp
 * @brief Fixture for Check, Checkmates and Stalemates tests. Endgame tests.
 * Naming convention: <TestedFunctionality>_<TestedColor>_<ExpectedResult>
 * @author Alexander Loodin Ek * 
 */
 
class CheckmateFixture : public ::testing::Test
{
public:
    virtual void SetUp()
    { 
        SetupDefaultStartingPosition(m_defaultStartingPosition);
    };
    virtual void TearDown() {};

    
    Chessboard m_emptyChessboard; // by default a board should start empty.
    Chessboard m_defaultStartingPosition;
    GameContext m_context;

private:    

};

////////////////////////////////////////////////////////////////

// 8 [ ][ ][ ][q][k][ ][ ][ ]
// 7 [ ][ ][ ][ ][ ][ ][ ][ ]
// 6 [ ][ ][ ][ ][ ][ ][ ][ ]
// 5 [ ][ ][ ][ ][ ][ ][ ][ ]
// 4 [ ][ ][ ][ ][ ][ ][ ][ ]
// 3 [ ][ ][ ][ ][ ][ ][ ][ ]
// 2 [ ][ ][ ][ ][ ][P][P][P]
// 1 [ ][ ][ ][R][ ][ ][K][ ]
//    A  B  C  D  E  F  G  H
// 3qk3/8/8/8/8/8/5PPP/3R2K1 b - - 0 1
// Black to move, checkmate in 1 move.
TEST_F(CheckmateFixture, BackrankCheckmate_Black_Checkmate)
{
    std::string fen("r3k2r/p1pNqpb1/bn2pnp1/3P4/1p2P3/2N2Q1p/PPPBBPPP/R3K2R b KQkq - 0 1");        
	FENParser::deserialize(fen.c_str(), m_context);

    EXPECT_FALSE(m_context.readChessboard().isCheckmated(Set::WHITE));
    EXPECT_FALSE(m_context.readChessboard().isCheckmated(Set::BLACK));
}

} // namespace ElephantTest