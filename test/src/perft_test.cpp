#include <gtest/gtest.h>
#include "elephant_test_utils.h"

#include "clock.hpp"
#include "fen_parser.h"
#include "game_context.h"
#include "move_generator.hpp"
#include "search.h"

#include <future>
#include <thread>

namespace ElephantTest {
////////////////////////////////////////////////////////////////
class PerftFixture : public ::testing::Test {
public:
    virtual void SetUp(){

    };
    virtual void TearDown(){};

    // MoveCount PerftCountMoves(
    //     GameContext& context, int depth, MoveCount& count, MoveCount::Predicate predicate = [](const Move&) { return true; })
    // {
    //     if (depth == 0) {
    //         return MoveCount();
    //     }

    //     MoveGenerator generator(context);
    //     generator.forEachMove([&](const PackedMove& mv) {
    //         context.MakeMove(mv);
    //         count += PerftCountMoves(context, depth - 1, count, predicate);
    //         context.UnmakeMove();
    //     });

    //     // auto moves = m_search.GeneratePossibleMoves(context);
    //     // count += CountMoves(moves, predicate);

    //     // if (depth > 1) {
    //     //     for (auto mv : moves) {
    //     //         Move cpy(mv);
    //     //         FATAL_ASSERT(context.MakeMove(cpy));
    //     //         FATAL_ASSERT(cpy.Piece.isValid());
    //     //         PerftCountMoves(context, depth - 1, count, predicate);
    //     //         context.UnmakeMove(cpy);
    //     //     }
    //     // }

    //     return count;
    // }

    // size_t concurrentMovesAtDepth(GameContext context, int depth)
    // {
    //     if (depth == 0) {
    //         return 0;
    //     }
    //     else if (depth == 1) {
    //         auto moves = m_search.GeneratePossibleMoves(context);
    //         return moves.size();
    //     }
    //     else {
    //         size_t count = 0;
    //         auto moves = m_search.GeneratePossibleMoves(context);
    //         for (auto mv : moves) {
    //             context.MakeMove(mv);
    //             count += CountMovesAtDepth(context, depth - 1);
    //             context.UnmakeMove(mv);
    //         }

    //         return count;
    //     }
    // }

    // size_t CountMovesAtDepth(GameContext& context, int depth)
    // {
    //     if (depth == 0) {
    //         return 0;
    //     }
    //     else if (depth == 1) {
    //         auto moves = m_search.GeneratePossibleMoves(context);
    //         return moves.size();
    //     }
    //     else {
    //         size_t count = 0;
    //         auto moves = m_search.GeneratePossibleMoves(context);
    //         for (auto mv : moves) {
    //             context.MakeMove(mv);
    //             u32 newDepth = depth - 1;
    //             count += concurrentMovesAtDepth(context, newDepth);

    //             context.UnmakeMove(mv);
    //         }

    //         return count;
    //     }
    // }

    // size_t CountMovesAtDepthConcurrent(GameContext& context, int depth)
    // {
    //     if (depth == 0) {
    //         return 0;
    //     }
    //     else if (depth == 1) {
    //         auto moves = m_search.GeneratePossibleMoves(context);
    //         return moves.size();
    //     }
    //     else {
    //         std::vector<std::future<size_t>> futures;

    //         size_t count = 0;
    //         auto moves = m_search.GeneratePossibleMoves(context);
    //         for (auto mv : moves) {
    //             context.MakeMove(mv);
    //             GameContext contextCopy(context);
    //             u32 newDepth = depth - 1;
    //             auto future = std::async(std::launch::async, &PerftFixture::concurrentMovesAtDepth, this, context, newDepth);

    //             futures.push_back(std::move(future));
    //             context.UnmakeMove(mv);
    //         }

    //         for (auto& future : futures) {
    //             count += future.get();
    //         }

    //         return count;
    //     }
    // }

    // template<bool concurrent = false>
    // void Catching_TestFunction(const std::string& fen, unsigned int expectedValue, int atDepth)
    // {
    //     Clock clock;
    //     clock.Start();
    //     FENParser::deserialize(fen.c_str(), m_context);
    //     size_t result = 0;
    //     if constexpr (concurrent)
    //         result = CountMovesAtDepthConcurrent(m_context, atDepth);
    //     else
    //         result = CountMovesAtDepth(m_context, atDepth);

    //     EXPECT_EQ(expectedValue, result);
    //     i64 elapsedTime = clock.getElapsedTime();
    //     LOG_INFO() << "Elapsed time: " << elapsedTime << " ms";

    //     // convert to seconds
    //     float et = elapsedTime / 1000.f;
    //     i64 nps = (i64)(result / et);
    //     LOG_INFO() << "Nodes per second: " << nps << " nps";
    // }

    GameContext m_context;
    Search m_search;
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

    // verify
    {
        PerftResult result = m_search.Perft(m_context, 1);
        EXPECT_EQ(20, result.Nodes);
    }

    {
        PerftResult result = m_search.Perft(m_context, 2);
        EXPECT_EQ(420, result.Nodes);
    }

    {
        // PerftResult result = m_search.Perft(m_context, 3);
        // EXPECT_EQ(9322, result.Nodes);
    }

    // {
    //     MoveCount count;
    //     PerftCountMoves(m_context, 4, count);
    //     EXPECT_EQ(206603, count.Moves);
    // }

    // {
    //     MoveCount count;
    //     PerftCountMoves(m_context, 5, count);
    //     EXPECT_EQ(5072212, count.Moves);
    // }
}
////////////////////////////////////////////////////////////////
/**
* 8 [ r ][   ][   ][   ][ k ][   ][   ][ r ]
* 7 [   ][   ][   ][   ][   ][   ][   ][   ]
* 6 [   ][   ][   ][   ][   ][   ][   ][   ]
* 5 [   ][   ][   ][   ][ B ][   ][   ][   ]
* 4 [   ][   ][   ][ b ][ b ][   ][   ][   ]
* 3 [   ][   ][   ][   ][   ][   ][   ][   ]
* 2 [   ][   ][   ][   ][   ][   ][   ][   ]
* 1 [ R ][   ][   ][   ][ K ][   ][   ][ R ]
*     A    B    C    D    E    F    G    H

r3k2r/8/8/4B3/3bb3/8/8/R3K2R w KQkq - 0 1    */

TEST_F(PerftFixture, BishopsAndRooks_Castling)
{
    // setup
    char inputFen[] = "r3k2r/8/8/4B3/3bb3/8/8/R3K2R w KQkq - 0 1";
    FENParser::deserialize(inputFen, m_context);
    PrintBoard(m_context.readChessboard());

    // verify
    {  // depth one
        PerftResult result = m_search.Perft(m_context, 1);
        EXPECT_EQ(34, result.Nodes);
        EXPECT_EQ(4, result.Captures);
        EXPECT_EQ(0, result.EnPassants);
        EXPECT_EQ(0, result.Promotions);
        EXPECT_EQ(1, result.Castles);
        EXPECT_EQ(2, result.Checks);
        // EXPECT_EQ(0, result.Checkmates);
    }

    {
        PerftResult result = m_search.Perft(m_context, 2);
        EXPECT_EQ(1474, result.Nodes);
    }
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
4	    4085603	    757163	    1929	128013	    15172       25523	    43
5       193690690	35043416	73365	4993637	    8392	    3309887	    30171 */
TEST_F(PerftFixture, Position_Two)
{
    // setup
    char inputFen[] = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    FENParser::deserialize(inputFen, m_context);
    PrintBoard(m_context.readChessboard());

    // verify
    {  // depth one
        PerftResult result = m_search.Perft(m_context, 1);
        EXPECT_EQ(48, result.Nodes);
        EXPECT_EQ(8, result.Captures);
        EXPECT_EQ(0, result.EnPassants);
        EXPECT_EQ(0, result.Promotions);
        EXPECT_EQ(2, result.Castles);
        EXPECT_EQ(0, result.Checks);
        // EXPECT_EQ(0, result.Checkmates);
    }

    {  // depth 2
        PerftResult result = m_search.Perft(m_context, 2);
        EXPECT_EQ(2087, result.Nodes);
        EXPECT_EQ(359, result.Captures);
        EXPECT_EQ(1, result.EnPassants);
        EXPECT_EQ(0, result.Promotions);
        EXPECT_EQ(91, result.Castles);
        EXPECT_EQ(3, result.Checks);
        // EXPECT_EQ(0, result.Checkmates);
    }
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

Depth	Nodes		Captures	E.p.	Castles		Promotions Checks Checkmates
1		14			1			0		0           0
2			0 2		191			14
0		0			0				10			0 3
2812		209			2		0			0
267			0 4		43238		3348		123		0
0				1680		17 5		674624		52051		1165
0			0				52950		0 6		11030083
940350		33325	0			7552			452473		2733 7
178633661	14519036	294874	0			140024 12797406	87
*/
// TEST_F(PerftFixture, Position_Three_Depth5)
// {
//     // setup
//     std::string inputFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
//     FENParser::deserialize(inputFen.c_str(), m_context);

//     // do & verify
//     MoveCount count;
//     PerftCountMoves(m_context, 5, count);

//     // verify
//     EXPECT_EQ(191 + 14 + 2812 + 43238 + 674624, count.Moves);
//     EXPECT_EQ(14 + 1 + 209 + 3348 + 52051, count.Captures);
//     EXPECT_EQ(0, count.Castles);
//     EXPECT_EQ(2 + 123 + 1165, count.EnPassants);
//     EXPECT_EQ(0, count.Promotions);

//     // EXPECT_EQ(10 + 2 + 267 + 1680 + 52950, count.Checks);
//     // EXPECT_EQ(0 + 17 + 0, count.Checkmates);
// }

// TEST_F(PerftFixture, DISABLED_Position_Three)
// {
//     // setup
//     std::string inputFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
//     FENParser::deserialize(inputFen.c_str(), m_context);
//     PrintBoard(m_context.readChessboard());

//     // do
//     auto moves = m_search.GeneratePossibleMoves(m_context);

//     // verify
//     auto count = CountMoves(moves);
//     auto orgMoves = m_search.OrganizeMoves(moves);

//     EXPECT_EQ(14, count.Moves);
//     EXPECT_EQ(1, count.Captures);
//     EXPECT_EQ(0, count.EnPassants);
//     EXPECT_EQ(0, count.Promotions);
//     EXPECT_EQ(0, count.Castles);
//     EXPECT_EQ(2, count.Checks);
//     // EXPECT_EQ(0, count.Checkmates);

//     MoveCount::Predicate pawnPredicate = [](const Move& mv) {
//         static ChessPiece P = WHITEPAWN;
//         if (mv.Piece == P)
//             return true;

//         return false;
//     };

//     count = CountMoves(moves, pawnPredicate);

//     EXPECT_EQ(4, count.Moves);
//     EXPECT_EQ(0, count.Captures);
//     EXPECT_EQ(0, count.EnPassants);
//     EXPECT_EQ(0, count.Promotions);
//     EXPECT_EQ(0, count.Castles);
//     EXPECT_EQ(1, count.Checks);
//     // EXPECT_EQ(0, count.Checkmates);
// }

// ////////////////////////////////////////////////////////////////

// /*
// * per depth
// Depth	Nodes		Captures	E.p.	Castles		Promotions Checks
// Checkmates
// 1		6			0			0		0
// 0			0			0 2		264			87
// 0		6			48			10			0 3
// 9467		1021		4		0			120 38			22
// 4		422333		131393		0		7795 60032		15492
// 5 5		15833292	2046173		6512	0			329464
// 200568		50562
// 6		706045033	210369132	212		10882006	81102984
// 26973664	81076

// * added up
// 1		6			0			0		0
// 0			0			0 2		270			87
// 0		6			48			10			0 3
// 9737		1108		4		6			168 48			22
// */
// TEST_F(PerftFixture, DISABLED_Position_Four)
// {
//     // setup
//     std::string inputFen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
//     FENParser::deserialize(inputFen.c_str(), m_context);
//     PrintBoard(m_context.readChessboard());

//     // do
//     auto moves = m_search.GeneratePossibleMoves(m_context);

//     // verify
//     auto count = CountMoves(moves);
//     auto orgMoves = m_search.OrganizeMoves(moves);

//     EXPECT_EQ(6, count.Moves);
//     EXPECT_EQ(0, count.Captures);
//     EXPECT_EQ(0, count.EnPassants);
//     EXPECT_EQ(0, count.Promotions);
//     EXPECT_EQ(0, count.Castles);
//     EXPECT_EQ(0, count.Checks);
//     // EXPECT_EQ(0, count.Checkmates);
// }

// TEST_F(PerftFixture, Position_Four_Depth3)
// {
//     // setup
//     std::string inputFen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
//     FENParser::deserialize(inputFen.c_str(), m_context);

//     // do & verify
//     MoveCount count;
//     PerftCountMoves(m_context, 3, count);

//     EXPECT_EQ(9737, count.Moves);
//     EXPECT_EQ(1108, count.Captures);
//     EXPECT_EQ(4, count.EnPassants);
//     EXPECT_EQ(6, count.Castles);
//     EXPECT_EQ(168, count.Promotions);
//     // EXPECT_EQ(48, count.Checks);
//     // EXPECT_EQ(22, count.Checkmates);
// }

// TEST_F(PerftFixture, Catching_IllegalEnPassant) { Catching_TestFunction("3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1", 1134888, 6); }

// TEST_F(PerftFixture, Catching_IllegalEnPassantTwo) { Catching_TestFunction("8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1", 1015133, 6);
// }

// TEST_F(PerftFixture, Catching_EnPassantCapture_ChecksOpponent)
// {
//     Catching_TestFunction("8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1", 1440467, 6);
// }

// TEST_F(PerftFixture, Catching_ShortCastlingCheck) { Catching_TestFunction("5k2/8/8/8/8/8/8/4K2R w K - 0 1", 661072, 6); }

// TEST_F(PerftFixture, Catching_LongCastlingGivesCheck) { Catching_TestFunction("3k4/8/8/8/8/8/8/R3K3 w Q - 0 1", 803711, 6); }

// TEST_F(PerftFixture, Catching_CastlingRights)
// {
//     Catching_TestFunction("r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1", 1274206, 4);
// }

// TEST_F(PerftFixture, Catching_CastlingPrevented)
// {
//     Catching_TestFunction("r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1", 1720476, 4);
// }

// TEST_F(PerftFixture, Catching_PromoteOutOfCheck) { Catching_TestFunction("2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1", 3821001, 6); }

// TEST_F(PerftFixture, Catching_DiscoveredCheck) { Catching_TestFunction("8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1", 1004658, 5); }

// TEST_F(PerftFixture, Catching_PromoteToGiveCheck) { Catching_TestFunction("4k3/1P6/8/8/8/8/K7/8 w - - 0 1", 217342, 6); }

// TEST_F(PerftFixture, Catching_UnderPromoteToGiveCheck) { Catching_TestFunction("8/P1k5/K7/8/8/8/8/8 w - - 0 1", 92683, 6); }

// TEST_F(PerftFixture, Catching_SelfStalemate) { Catching_TestFunction("K1k5/8/P7/8/8/8/8/8 w - - 0 1", 2217, 6); }

// TEST_F(PerftFixture, Catching_StalemateAndCheckmate) { Catching_TestFunction("8/k1P5/8/1K6/8/8/8/8 w - - 0 1", 567584, 7); }

// TEST_F(PerftFixture, Catching_StalemateAndCheckmateTwo)
// {
//     Catching_TestFunction("8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1", 23527, 4);
// }

// /* This test takes a long time to run, so it is disabled by default
// https://www.chessprogramming.net/perfect-perft/
// */
// TEST_F(PerftFixture, Catching_TwoHundrarMillionNodes_Twice)
// {
//     Catching_TestFunction("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 193690690, 5);
//     Catching_TestFunction("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 178633661, 7);
// }

// TEST_F(PerftFixture, DISABLED_Catching_SevenHundradMillionNodes)
// {
//     Catching_TestFunction("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 706045033, 6);
// }

// TEST_F(PerftFixture, Catching_BishopVsTwoRookEndgame)
// {
//     Catching_TestFunction("1k6/1b6/8/8/7R/8/8/4K2R b K - 0 1", 1063513, 5);
// }

////////////////////////////////////////////////////////////////

}  // namespace ElephantTest