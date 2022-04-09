#include <gtest/gtest.h>
#include "game_context.h"
#include "fen_parser.h"
#include "move_generator.h"
#include "elephant_test_utils.h"

namespace ElephantTest
{
////////////////////////////////////////////////////////////////
class PerftFixture : public ::testing::Test
{
public:
    virtual void SetUp()
    {

    };
    virtual void TearDown() {};

    GameContext m_context;
    MoveGenerator m_moveGenerator;
};
////////////////////////////////////////////////////////////////

TEST_F(PerftFixture, Position_Start)
{
    // setup
    char inputFen[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    FENParser::deserialize(inputFen, m_context);

    PrintBoard(m_context.readChessboard());
    // do
    auto moves = m_moveGenerator.GeneratePossibleMoves(m_context);

    // verify    
    auto count = m_moveGenerator.CountMoves(moves);
    EXPECT_EQ(20, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);
}
////////////////////////////////////////////////////////////////

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

Position Two Expected Results
a.k.a. Kiwipete
depth	nodes			totalnodes
1		48				48
2		2,039			2,087
3		97,862			99,949
4		4,085,603		4,185,552
5		193,690,690		19,78,76,242
6		8,031,647,685	8,229,523,927
*/
/*
Depth	Nodes	    Captures	E.p.	Castles	    Promotions	Checks	    Checkmates
1	    48      	8	        0	    2	        0	        0	        0
2	    2039	    351	        1	    91      	0	        3       	0
3	    97862	    17102	    45	    3162	    0	        993     	1
4	    4085603	    757163	    1929	128013	    15172	    25523	    43
5	    193690690	35043416	73365	4993637	    8392	    3309887	    30171
*/


TEST_F(PerftFixture, Position_Two)
{
    // setup
	char inputFen[] = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    FENParser::deserialize(inputFen, m_context);
    PrintBoard(m_context.readChessboard());

    // do
    auto moves = m_moveGenerator.GeneratePossibleMoves(m_context);

    // verify
    auto count = m_moveGenerator.CountMoves(moves);

    EXPECT_EQ(48, count.Moves);
    EXPECT_EQ(8, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(2, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);

    MoveCount::Predicate pawnPredicate = [](const Move& mv) 
    {
        static ChessPiece P = WHITEPAWN;
        if (mv.Piece == P)
            return true;
        
        return false;
    };

    count = m_moveGenerator.CountMoves(moves, pawnPredicate);

    EXPECT_EQ(8, count.Moves);
    EXPECT_EQ(2, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);

}

////////////////////////////////////////////////////////////////

/*
        8  [ ][ ][ ][ ][ ][ ][ ][ ]
        7  [ ][ ][p][ ][ ][ ][ ][ ]
        6  [ ][ ][ ][p][ ][ ][ ][ ]
        5  [K][P][ ][ ][ ][ ][ ][r]
        4  [ ][R][ ][ ][ ][p][ ][k]
        3  [ ][ ][ ][ ][ ][ ][ ][ ]
        2  [ ][ ][ ][ ][P][ ][P][ ]
        1  [ ][ ][ ][ ][ ][ ][ ][ ]
            A  B  C  D  E  F  G  H

Depth	Nodes		Captures	E.p.	Castles		Promotions		Checks		Checkmates
1		14			1			0		0			0				2			0
2		191			14			0		0			0				10			0
3		2812		209			2		0			0				267			0
4		43238		3348		123		0			0				1680		17
5		674624		52051		1165	0			0				52950		0
6		11030083	940350		33325	0			7552			452473		2733
7		178633661	14519036	294874	0			140024			12797406	87
*/
TEST_F(PerftFixture, Position_Three)
{
    // setup
	std::string inputFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    FENParser::deserialize(inputFen.c_str(), m_context);
    PrintBoard(m_context.readChessboard());

    // do
    auto moves = m_moveGenerator.GeneratePossibleMoves(m_context);

    // verify
    auto count = m_moveGenerator.CountMoves(moves);
    auto orgMoves = m_moveGenerator.OrganizeMoves(moves);

    EXPECT_EQ(14, count.Moves);
    EXPECT_EQ(1, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(2, count.Checks);
    EXPECT_EQ(0, count.Checkmates);

    MoveCount::Predicate pawnPredicate = [](const Move& mv) 
    {
        static ChessPiece P = WHITEPAWN;
        if (mv.Piece == P)
            return true;
        
        return false;
    };

    count = m_moveGenerator.CountMoves(moves, pawnPredicate);

    EXPECT_EQ(4, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(1, count.Checks);
    EXPECT_EQ(0, count.Checkmates);
}

////////////////////////////////////////////////////////////////

} // namespace ElephantTest