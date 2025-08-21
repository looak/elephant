#include <gtest/gtest.h>
#include "elephant_test_utils.h"

#include "clock.hpp"
#include <io/fen_parser.hpp>
#include <core/game_context.hpp>
#include <move/generation/move_generator.hpp>
#include "search.hpp"

#include <future>
#include <source_location>
#include <thread>

namespace ElephantTest {
////////////////////////////////////////////////////////////////
class PerftFixture : public ::testing::Test {
public:
    virtual void SetUp() {

    };
    virtual void TearDown() {};

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
        EXPECT_EQ(1474 + 34, result.Nodes);
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
        // EXPECT_EQ(91, result.Castles);
        EXPECT_EQ(3, result.Checks);
        // EXPECT_EQ(0, result.Checkmates);
    }

    {  // depth 3
        PerftResult result = m_search.PerftDivide(m_context, 3);
        EXPECT_EQ(97862, result.Nodes);
        // EXPECT_EQ(359 + 17102, result.Captures);
        // EXPECT_EQ(1 + 45, result.EnPassants);
        // EXPECT_EQ(0, result.Promotions);
        // EXPECT_EQ(91 + 3162, result.Castles);
        // EXPECT_EQ(3 + 993, result.Checks);
        // EXPECT_EQ(0, result.Checkmates);
    }

    {  // depth 4
        PerftResult result = m_search.PerftDivide(m_context, 4);
        EXPECT_EQ(4085603, result.Nodes);
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

struct PerftCaseArgs {
    bool enabled;
    std::string testId;
    std::string fen;
    u32 expectedNodeCount;
    u8 searchDepth;

    // Auto-fill testId from the caller when not provided
    PerftCaseArgs(
        bool enabled_,
        std::string fen_,
        u32 expectedNodes_,
        u8 depth_,
        const std::source_location& loc = std::source_location::current())
        : testId(currentCallerName(loc)),
          fen(std::move(fen_)),
          expectedNodeCount(expectedNodes_),
          searchDepth(depth_),
          enabled(enabled_) {}

    // Explicit testId provided
    PerftCaseArgs(
        bool enabled_,
        std::string testId_,
        std::string fen_,
        u32 expectedNodes_,
        u8 depth_)
        : testId(std::move(testId_)),
          fen(std::move(fen_)),
          expectedNodeCount(expectedNodes_),
          searchDepth(depth_),
          enabled(enabled_) {}

private:
    static std::string currentCallerName(const std::source_location& loc) {
        if (auto* info = ::testing::UnitTest::GetInstance()->current_test_info()) {
            return std::string(info->test_suite_name()) + "." + info->name();
        }
        return std::string(loc.function_name());
    }
};

PerftResult ExecutePerftCase(const std::string& fen, int atDepth)
{
    // setup
    Chessboard board;
    fen_parser::deserialize(fen.c_str(), board);

    // do
    Search search;
    return search.PerftDivide(context, atDepth);
}

PerftResult ExecutePerftTestCase(PerftCaseArgs perftCase)
{
    Clock caseClock;
    caseClock.Start();

    LOG_INFO() << "Running test:     " << perftCase.testId;
    caseClock.Start();
    auto result = ExecutePerftCase(perftCase.fen, perftCase.searchDepth);
    caseClock.Stop();    
    result.NPS = caseClock.calcNodesPerSecond(result.Nodes);
    LOG_INFO() << " [ RESULTS ] Nodes: - - - - - - - " << result.Nodes << " nodes";
    LOG_INFO() << " [ RESULTS ] Nodes per second: - - " << result.NPS << " nps";
    LOG_INFO() << " [ RESULTS ] Elapsed time: - - - - " << caseClock.getElapsedTime() << " ms";
    LOG_INFO() << "---------------------------------";

    EXPECT_EQ(perftCase.expectedNodeCount, result.Nodes);    
    return result;
}

TEST_F(PerftFixture, EstablishedReferencePositions)
{
    std::vector<PerftCaseArgs> perftTestCases = {
        { "illegal enpassant", "3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1", 1134888, 6 },
        { "illegal enpassant", "8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1", 1015133, 6 },
        { "en passant capture, checks opponent", "8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1", 1440467, 6 },
        { "short castling", "5k2/8/8/8/8/8/8/4K2R w K - 0 1", 661072, 6 },
        { "long castling", "3k4/8/8/8/8/8/8/R3K3 w Q - 0 1", 803711, 6 },
        { "castling rights", "r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1", 1274206, 4 },
        { "castling prevented", "r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1", 1720476, 4 },
        { "promotion out of check", "2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1", 3821001, 6 },
        { "discovered check", "8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1", 1004658, 5 },
        { "promote to give check", "4k3/1P6/8/8/8/8/K7/8 w - - 0 1", 217342, 6 },
        { "under promote to give check", "8/P1k5/K7/8/8/8/8/8 w - - 0 1", 92683, 6 },
        { "self stalemate", "K1k5/8/P7/8/8/8/8/8 w - - 0 1", 2217, 6 },
        { "stalemate and checkmate", "8/k1P5/8/1K6/8/8/8/8 w - - 0 1", 567584, 7 },
        { "stalemate and checkmate", "8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1", 23527, 4 },
        { "two hundred million nodes", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 193690690, 5 },
        { "two hundred million nodes", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 178633661, 7 },
        //{ "seven hundred million nodes", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 706045033, 6 },
        { "bishop vs rook endgame", "1k6/1b6/8/8/7R/8/8/4K2R b K - 0 1", 1063513, 5 },
    };

    Clock clock;    
    clock.Start();
    u64 totalNodes = 0;
    u64 totalNps = 0;
    for (auto& perftCase : perftTestCases) {
        auto result = ExecutePerftTestCase(perftCase);
        totalNodes += result.Nodes;
        totalNps += result.NPS;
    }

    clock.Stop();
    u64 nps = clock.calcNodesPerSecond(totalNodes);
    i64 elapsedTime = clock.getElapsedTime();
    LOG_INFO() << " [ AGGREGATE RESULTS ]";
    LOG_INFO() << " [ RESULTS ] Total nodes:  - - - - - - - " << totalNodes << " nodes";
    LOG_INFO() << " [ RESULTS ] Total elapsed time: - - - - " << elapsedTime << " ms";
    LOG_INFO() << " [ RESULTS ] Total nodes per second: - - " << nps << " nps";
    LOG_INFO() << " [ RESULTS ] Average nodes per second: - " << totalNps / perftTestCases.size() << " nps";
    LOG_INFO() << "---------------------------------";
}

TEST_F(PerftFixture, DISABLED_Catching_IllegalEnPassant)
{     ExecutePerftTestCase({"3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1", 1134888, 6});
}
 TEST_F(PerftFixture, DISABLED_Catching_IllegalEnPassantTwo) { ExecutePerftTestCase({"8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1", 1015133, 6}); }

TEST_F(PerftFixture, DISABLED_Catching_EnPassantCapture_ChecksOpponent)
{     ExecutePerftTestCase({"8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1", 1440467, 6});
}
 TEST_F(PerftFixture, DISABLED_Catching_ShortCastlingCheck) { ExecutePerftTestCase({"5k2/8/8/8/8/8/8/4K2R w K - 0 1", 661072, 6}); }
 TEST_F(PerftFixture, DISABLED_Catching_LongCastlingGivesCheck) { ExecutePerftTestCase({"3k4/8/8/8/8/8/8/R3K3 w Q - 0 1", 803711, 6}); }

TEST_F(PerftFixture, DISABLED_Catching_CastlingRights)
{     ExecutePerftTestCase({"r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1", 1274206, 4});
}

TEST_F(PerftFixture, DISABLED_Catching_CastlingPrevented)
{     ExecutePerftTestCase({"r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1", 1720476, 4});
}
 TEST_F(PerftFixture, DISABLED_Catching_PromoteOutOfCheck) { ExecutePerftTestCase({"2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1", 3821001, 6}); }
 TEST_F(PerftFixture, DISABLED_Catching_DiscoveredCheck) { ExecutePerftTestCase({"8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1", 1004658, 5}); }
 TEST_F(PerftFixture, DISABLED_Catching_PromoteToGiveCheck) { ExecutePerftTestCase({"4k3/1P6/8/8/8/8/K7/8 w - - 0 1", 217342, 6}); }
 TEST_F(PerftFixture, DISABLED_Catching_UnderPromoteToGiveCheck) { ExecutePerftTestCase({"8/P1k5/K7/8/8/8/8/8 w - - 0 1", 92683, 6}); }
 TEST_F(PerftFixture, DISABLED_Catching_SelfStalemate) { ExecutePerftTestCase({"K1k5/8/P7/8/8/8/8/8 w - - 0 1", 2217, 6}); }
 TEST_F(PerftFixture, DISABLED_Catching_StalemateAndCheckmate) { ExecutePerftTestCase({"8/k1P5/8/1K6/8/8/8/8 w - - 0 1", 567584, 7}); }

TEST_F(PerftFixture, DISABLED_Catching_StalemateAndCheckmateTwo)
{     ExecutePerftTestCase({"8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1", 23527, 4});
}

/* This test takes a long time to run, so it is disabled by default
https://www.chessprogramming.net/perfect-perft/
*/
TEST_F(PerftFixture, DISABLED_Catching_TwoHundrarMillionNodes_Twice)
{     
    ExecutePerftTestCase({"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 193690690, 5});     
    ExecutePerftTestCase({"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 178633661, 7});
}

TEST_F(PerftFixture, DISABLED_Catching_SevenHundradMillionNodes)
{     ExecutePerftTestCase({"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 706045033, 6});
}

TEST_F(PerftFixture, DISABLED_Catching_BishopVsTwoRookEndgame)
{     ExecutePerftTestCase({"1k6/1b6/8/8/7R/8/8/4K2R b K - 0 1", 1063513, 5});
}

////////////////////////////////////////////////////////////////

}  // namespace ElephantTest