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

	MoveCount CountMoves(GameContext& context, int depth, MoveCount& count, bool divide = false, MoveCount::Predicate predicate = [](const Move& mv) { return true; })
	{		
		if (depth == 0)
		{
			return MoveCount();
		}
        
		auto moves = m_moveGenerator.GeneratePossibleMoves(context);        
		count += m_moveGenerator.CountMoves(moves, predicate);
        
        if (depth > 1)
        {
            for (auto mv : moves)
            {
                FATAL_ASSERT(context.MakeMove(mv));
                CountMoves(context, depth - 1, count, divide, predicate);
                context.UnmakeMove(mv);
            }
        }
        
        return count;
	}

    GameContext m_context;
    MoveGenerator m_moveGenerator;
};
//////////////////////////////////////////////////////////////
/*
depth	nodes	    totalnodes
1	    20	        20
2	    400	        420
3	    8902	    9322
4	    197281	    206603
5	    4865609	    5072212
6	    119060324	124132536
7	    3195901860	3320034396
*/
TEST_F(PerftFixture, Position_Start)
{
    // setup
    char inputFen[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    FENParser::deserialize(inputFen, m_context);

    PrintBoard(m_context.readChessboard());
    // do
    auto moves = m_moveGenerator.GeneratePossibleMoves(m_context);

    // verify    
    {
        MoveCount count;
        CountMoves(m_context, 1, count);
        EXPECT_EQ(20, count.Moves);
    }

    {
        MoveCount count;
        CountMoves(m_context, 2, count);
        EXPECT_EQ(420, count.Moves);
    }

    {
        MoveCount count;
        CountMoves(m_context, 3, count);
        EXPECT_EQ(9322, count.Moves);
    }
 /*   breaking
    {
        MoveCount count;
        CountMoves(m_context, 4, count);
        EXPECT_EQ(206603, count.Moves);
    }

    {
        MoveCount count;
        CountMoves(m_context, 5, count);
        EXPECT_EQ(5072212, count.Moves);
    }*/
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

r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1

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

    auto& board = m_context.editChessboard();
    auto otherMoves = board.GetAvailableMoves(e1, WHITEKING, 0, false, 0);

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

//3	    97862	    17102	    45	    3162	    0	        993     	1

//TEST_F(PerftFixture, Position_Two_Depth3)
//{
//    // setup
//    char inputFen[] = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
//    FENParser::deserialize(inputFen, m_context);
//    
//    // do & verify
//    MoveCount count;
//    CountMoves(m_context, 3, count);
//
//    // verify
//	EXPECT_EQ(2039 + 48 + 97862, count.Moves);
//	EXPECT_EQ(351 + 8 + 17102, count.Captures);
//	EXPECT_EQ(1 + 0 + 45, count.EnPassants);
//	EXPECT_EQ(0 + 0 + 0, count.Promotions);
//	EXPECT_EQ(91 + 2 + 3162, count.Castles);
//	EXPECT_EQ(3 + 0 + 993, count.Checks);
//	EXPECT_EQ(0 + 0 + 1, count.Checkmates);
//}

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
TEST_F(PerftFixture, Position_Three_Depth4)
{
    // setup
    std::string inputFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    FENParser::deserialize(inputFen.c_str(), m_context);

    // do & verify
    MoveCount count;
    CountMoves(m_context, 4, count);

    // verify
	EXPECT_EQ(191 + 14 + 2812 + 43238, count.Moves);
	EXPECT_EQ(14 + 1 + 209 + 3348, count.Captures);
	EXPECT_EQ(10 + 2 + 267 + 1680, count.Checks);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(2 + 123, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0 + 17, count.Checkmates);
}

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

/*
* per depth
Depth	Nodes		Captures	E.p.	Castles		Promotions	Checks		Checkmates
1		6			0			0		0			0			0			0
2		264			87			0		6			48			10			0
3		9467		1021		4		0			120			38			22
4		422333		131393		0		7795		60032		15492		5
5		15833292	2046173		6512	0			329464		200568		50562
6		706045033	210369132	212		10882006	81102984	26973664	81076

* added up
1		6			0			0		0			0			0			0
2		270			87			0		6			48			10			0
3		9737		1108		4		6			168			48			22
*/
TEST_F(PerftFixture, Position_Four)
{
    // setup    
	std::string inputFen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    FENParser::deserialize(inputFen.c_str(), m_context);
    PrintBoard(m_context.readChessboard());

    // do
    auto moves = m_moveGenerator.GeneratePossibleMoves(m_context);

    // verify
    auto count = m_moveGenerator.CountMoves(moves);
    auto orgMoves = m_moveGenerator.OrganizeMoves(moves);

    EXPECT_EQ(6, count.Moves);
    EXPECT_EQ(0, count.Captures);
    EXPECT_EQ(0, count.EnPassants);
    EXPECT_EQ(0, count.Promotions);
    EXPECT_EQ(0, count.Castles);
    EXPECT_EQ(0, count.Checks);
    EXPECT_EQ(0, count.Checkmates);
}

TEST_F(PerftFixture, Position_Four_Depth3)
{
    // setup    
    std::string inputFen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    FENParser::deserialize(inputFen.c_str(), m_context);
    
    // do & verify
    MoveCount count;
    CountMoves(m_context, 3, count);

	EXPECT_EQ(9737, count.Moves);
	EXPECT_EQ(1108, count.Captures);
	EXPECT_EQ(4, count.EnPassants);
	EXPECT_EQ(6, count.Castles);
	EXPECT_EQ(168, count.Promotions);
	EXPECT_EQ(48, count.Checks);
	EXPECT_EQ(22, count.Checkmates);
}

////////////////////////////////////////////////////////////////


} // namespace ElephantTest