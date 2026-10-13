#include <gtest/gtest.h>

#include <io/fen_parser.hpp>
#include <core/uci.hpp>
#include <core/game_context.hpp>
#include <search/search.hpp>
#include <system/time_manager.hpp>

#include <algorithm>
#include <chrono>
#include <thread>

namespace ElephantTest {
/**
 * @file uci_test.cpp
 * @brief Fixture testing Universal Chess Interface of engine
 * https://www.wbec-ridderkerk.nl/html/UCIProtocol.html
 * Naming convention: <UCICommand>_<ExpectedBehavior>
 * https://osherove.com/blog/2005/4/3/naming-standards-for-unit-tests.html
 * @author Alexander Loodin Ek *
 */
 ////////////////////////////////////////////////////////////////
class UciFixture : public ::testing::Test {
public:
    virtual void SetUp() {};
    virtual void TearDown() {};

    /**
     * This is the exact function that exists in commands_util which is the first
     * step in the CLI which splits any commands into a list of tokens.
     * @brief Extracts arguments from command line
     * @param buffer command line
     * @param tokens list of arguments  */
    void extractArgsFromCommand(const std::string& buffer, std::list<std::string>& tokens)
    {
        std::istringstream ssargs(buffer);
        std::string token;
        while (std::getline(ssargs, token, ' ')) {
            tokens.push_back(token);
        }
    }

    UCI m_uci;
};
////////////////////////////////////////////////////////////////
/**
 * @class ScopedRedirect
 * @brief Redirects given ostream to another buffer which we later can verify */
class ScopedRedirect {
public:
    ScopedRedirect(std::ostream& redirected, std::ostream& target) :
        m_originalStream(redirected),
        m_originalBuffer(redirected.rdbuf())
    {
        m_originalStream.rdbuf(target.rdbuf());
    }
    ~ScopedRedirect() { m_originalStream.rdbuf(m_originalBuffer); }

private:
    std::ostream& m_originalStream;
    std::streambuf* m_originalBuffer;
};
////////////////////////////////////////////////////////////////

TEST_F(UciFixture, isready_Outputs_readyok)
{
    // setup
    m_uci.Enable();

    std::stringstream testOutput;
    bool result = false;
    {
        // redirect std::cout to a buffer
        ScopedRedirect coutRedirect(std::cout, testOutput);

        // do
        result = m_uci.IsReady();
    }

    // verify
    EXPECT_STREQ("readyok\n", testOutput.str().c_str());
    EXPECT_TRUE(result);
}

TEST_F(UciFixture, Enabled_Outputs_uciok)
{
    // setup
    m_uci.Enable();

    std::stringstream testOutput;
    {
        // redirect std::cout to a buffer
        ScopedRedirect coutRedirect(std::cout, testOutput);

        // do
        m_uci.Enable();
    }

    // verify
    EXPECT_STREQ("uciok\n", testOutput.str().c_str());
}

TEST_F(UciFixture, position_startpos_InitializesGameContextToDefaultStartPos)
{
    // setup
    m_uci.Enable();

    // do
    std::list<std::string> args{ "startpos" };
    bool result = m_uci.Position(args);

    // verify
    EXPECT_TRUE(result);
    EXPECT_EQ(Set::WHITE, m_uci.readGameContext().readToPlay());
    EXPECT_EQ(1, m_uci.readGameContext().readMoveCount());
    // EXPECT_EQ(0, m_uci.readGameContext().readMoveHistory().size());

    auto board = m_uci.readGameContext().readChessPosition();
    EXPECT_EQ(WHITEKING, board.pieceAt(Square::E1));
    EXPECT_EQ(BLACKKING, board.pieceAt(Square::E8));
    EXPECT_EQ(WHITEQUEEN, board.pieceAt(Square::D1));
    EXPECT_EQ(BLACKQUEEN, board.pieceAt(Square::D8));

    EXPECT_TRUE(board.castling().hasAll());

    std::string outputFen;
    io::fen_parser::serialize(m_uci.readGameContext().readChessboard(), outputFen);
    EXPECT_STREQ(c_startPositionFen.c_str(), outputFen.c_str());
}

TEST_F(UciFixture, position_fen_InitializesGameToGivenFen)
{
    // setup
    m_uci.Enable();
    std::string gocFen = "r3rnk1/pb3pp1/3pp2p/1q4BQ/1P1P4/4N1R1/P4PPP/4R1K1 b - - 18 1";
    GameContext expected;
    io::fen_parser::deserialize(gocFen.c_str(), expected.editChessboard());

    // do
    std::string commandLine = "position fen " + gocFen;
    std::list<std::string> args;
    extractArgsFromCommand(commandLine, args);
    args.pop_front();  // pop position
    bool result = m_uci.Position(args);

    // verify
    EXPECT_TRUE(result);
    EXPECT_EQ(Set::BLACK, m_uci.readGameContext().readToPlay());
    // EXPECT_EQ(0, m_uci.readGameContext().readMoveHistory().size());

    const auto& board = m_uci.readGameContext().readChessPosition();
    EXPECT_EQ(WHITEKING, board.pieceAt(Square::G1));
    EXPECT_EQ(BLACKKING, board.pieceAt(Square::G8));

    EXPECT_FALSE(board.castling().hasAny());

    std::string outputFen;
    io::fen_parser::serialize(m_uci.readGameContext().readChessboard(), outputFen);
    EXPECT_STREQ(gocFen.c_str(), outputFen.c_str());
}

// when converting the acn to a packed move we didn't identify the pawn capture as a capture for some reason.
TEST_F(UciFixture, position_fen_moves_InitializesGameToGivenFenAndAppliesMoves_PawnCaptureHandledCorrectly)
{
    // setup
    m_uci.Enable();
    std::string gocFen = "2r5/p1p1nk1p/q4pp1/1p1pp3/1P4P1/2P5/3PPP1P/2Q1K1NR w K - 0 32";
    GameContext expected;
    io::fen_parser::deserialize(gocFen.c_str(), expected.editChessboard());

    // do
    std::string commandLine = "position fen " + gocFen + " moves d2d4 e5d4";
    std::list<std::string> args;
    extractArgsFromCommand(commandLine, args);
    args.pop_front();  // pop position
    bool result = m_uci.Position(args);

    // verify
    EXPECT_TRUE(result);
    EXPECT_EQ(Set::WHITE, m_uci.readGameContext().readToPlay());
    // EXPECT_EQ(0, m_uci.readGameContext().readMoveHistory().size());

    const auto& board = m_uci.readGameContext().readChessPosition();
    EXPECT_EQ(BLACKPAWN, board.pieceAt(Square::D4));
    EXPECT_EQ(WHITEPAWN, board.pieceAt(Square::C3));

    CastlingStateInfo expectedCastlingState;
    expectedCastlingState.grantWhiteKingSide();
    expectedCastlingState.revokeWhiteQueenSide();
    expectedCastlingState.revokeAllBlack();
    EXPECT_EQ(expectedCastlingState, board.castling());

    EXPECT_FALSE(board.enPassant());

    gocFen = "2r5/p1p1nk1p/q4pp1/1p1p4/1P1p2P1/2P5/4PP1P/2Q1K1NR w K - 0 33";
    std::string outputFen;
    io::fen_parser::serialize(m_uci.readGameContext().readChessboard(), outputFen);
    EXPECT_STREQ(gocFen.c_str(), outputFen.c_str());
}

// EPD style perft annotations after the fen, ";D1 20 ;D2 400 ;D3 8902", end the fen and are ignored.
TEST_F(UciFixture, position_fen_PerftAnnotationsAreIgnored)
{
    // setup
    m_uci.Enable();
    const std::string startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    // do
    std::string commandLine = "position fen " + startFen + " ;D1 20 ;D2 400 ;D3 8902";
    std::list<std::string> args;
    extractArgsFromCommand(commandLine, args);
    args.pop_front();  // pop position
    bool result = m_uci.Position(args);

    // verify
    EXPECT_TRUE(result);
    EXPECT_EQ(Set::WHITE, m_uci.readGameContext().readToPlay());

    const auto& board = m_uci.readGameContext().readChessPosition();
    EXPECT_EQ(WHITEKING, board.pieceAt(Square::E1));
    EXPECT_EQ(BLACKKING, board.pieceAt(Square::E8));
    EXPECT_EQ(WHITEQUEEN, board.pieceAt(Square::D1));
    EXPECT_EQ(BLACKQUEEN, board.pieceAt(Square::D8));
    EXPECT_TRUE(board.castling().hasAll());
    EXPECT_FALSE(board.enPassant());

    std::string outputFen;
    io::fen_parser::serialize(m_uci.readGameContext().readChessboard(), outputFen);
    EXPECT_STREQ(startFen.c_str(), outputFen.c_str());
}

TEST_F(UciFixture, go_depth_3_DoesASearchAndReturnsAMove)
{
    // setup
    m_uci.Enable();
    m_uci.NewGame();

    // do
    std::string commandLine = "go depth 3";
    std::list<std::string> args;
    extractArgsFromCommand(commandLine, args);
    args.pop_front();  // pop go
    bool result = false;
    std::stringstream testOutput;
    {
        // redirect std::cout to a buffer
        ScopedRedirect coutRedirect(std::cout, testOutput);

        // do
        result = m_uci.Go(args);
    }

    EXPECT_TRUE(result);
}

TEST_F(UciFixture, go_infinite_SearchesAndHoldsBestmoveUntilStop)
{
    // setup, mate in one so the search ends on its own long before stop is sent.
    m_uci.Enable();
    std::list<std::string> position;
    extractArgsFromCommand("fen 6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1", position);
    m_uci.Position(position);

    std::stringstream testOutput;
    std::string beforeStop;
    {
        ScopedRedirect coutRedirect(std::cout, testOutput);

        // do
        m_uci.AsyncGo({ "infinite" });
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        beforeStop = testOutput.str();
        m_uci.Stop();
        m_uci.SyncGo();
    }

    // verify
    EXPECT_EQ(std::string::npos, beforeStop.find("bestmove"));
    EXPECT_NE(std::string::npos, testOutput.str().find("bestmove a1a8"));
}

// The GUI sends the clock with go ponder, it mustn't limit the search, the time is the opponent's until ponderhit.
TEST_F(UciFixture, go_ponder_SearchesAndHoldsBestmoveUntilPonderhit)
{
    // setup, mate in one so the search ends on its own long before ponderhit is sent.
    m_uci.Enable();
    std::list<std::string> position;
    extractArgsFromCommand("fen 6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1", position);
    m_uci.Position(position);

    std::stringstream testOutput;
    std::string beforePonderhit;
    {
        ScopedRedirect coutRedirect(std::cout, testOutput);

        // do
        m_uci.AsyncGo({ "ponder", "wtime", "100", "btime", "100" });
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        beforePonderhit = testOutput.str();
        m_uci.PonderHit();
        m_uci.SyncGo();
    }

    // verify
    EXPECT_EQ(std::string::npos, beforePonderhit.find("bestmove"));
    EXPECT_NE(std::string::npos, testOutput.str().find("bestmove a1a8"));
}

TEST_F(UciFixture, go_ponder_StopEndsThePonderSearch)
{
    // setup
    m_uci.Enable();
    m_uci.NewGame();

    std::stringstream testOutput;
    {
        ScopedRedirect coutRedirect(std::cout, testOutput);

        // do
        m_uci.AsyncGo({ "ponder", "wtime", "60000", "btime", "60000" });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        m_uci.Stop();
        m_uci.SyncGo();
    }

    // verify
    EXPECT_NE(std::string::npos, testOutput.str().find("bestmove"));
}

TEST_F(UciFixture, setoption_MultiWordName_MoveOverheadIsAccepted)
{
    // setup
    m_uci.Enable();

    // do
    std::list<std::string> args;
    extractArgsFromCommand("setoption name Move Overhead value 100", args);
    args.pop_front();  // pop setoption
    bool result = m_uci.SetOption(args);

    // verify
    EXPECT_TRUE(result);
}

TEST_F(UciFixture, position_PromotingPawn)
{
    // setup
    m_uci.Enable();
    std::string fen = "8/2k3P1/8/8/8/8/8/2K5 w - - 0 1";

    // do
    std::string commandLine = "position fen " + fen + " moves g7g8q";
    std::list<std::string> args;
    extractArgsFromCommand(commandLine, args);
    args.pop_front();  // pop position
    bool result = m_uci.Position(args);

    // verify
    EXPECT_TRUE(result);
    EXPECT_EQ(Set::BLACK, m_uci.readGameContext().readToPlay());

    const auto& board = m_uci.readGameContext().readChessPosition();
    EXPECT_EQ(WHITEQUEEN, board.pieceAt(Square::G8));

    std::string outputFen;
    io::fen_parser::serialize(m_uci.readGameContext().readChessboard(), outputFen);
    EXPECT_STREQ("6Q1/2k5/8/8/8/8/8/2K5 b - - 1 1", outputFen.c_str());
}

namespace {
// the largest "nodes" value of the info lines, the node count of the last completed iteration.
u64 lastReportedNodes(const std::string& output)
{
    u64 nodes = 0;
    size_t pos = 0;
    while ((pos = output.find(" nodes ", pos)) != std::string::npos) {
        pos += 7;
        nodes = std::max<u64>(nodes, std::stoull(output.substr(pos)));
    }
    return nodes;
}
}  // namespace

TEST_F(UciFixture, go_nodes_StopsAtTheNodeLimitAndReturnsAMove)
{
    // setup
    m_uci.Enable();
    m_uci.NewGame();

    std::stringstream testOutput;
    bool result = false;
    {
        ScopedRedirect coutRedirect(std::cout, testOutput);

        // do
        result = m_uci.Go({ "nodes", "5000" });
    }

    // verify, completed iterations never report more than the limit.
    EXPECT_TRUE(result);
    EXPECT_NE(std::string::npos, testOutput.str().find("bestmove"));
    EXPECT_EQ(std::string::npos, testOutput.str().find("bestmove 0000"));
    EXPECT_GT(lastReportedNodes(testOutput.str()), 0u);
    EXPECT_LE(lastReportedNodes(testOutput.str()), 5000u);
}

TEST_F(UciFixture, go_nodes_SearchStopsCloseToTheLimitAndIsDeterministic)
{
    auto search = [](u64 nodeLimit) {
        GameContext context;
        context.NewGame();
        Search search(context);
        SearchParameters params;
        params.NodeLimit = nodeLimit;
        TimeManager clock(params, Set::WHITE);
        return search.go<Set::WHITE>(params, clock);
    };

    // do
    SearchResult first = search(20000);
    SearchResult second = search(20000);

    // verify, the count passes the limit by at most the nodes counted while unwinding.
    EXPECT_GE(first.count, 20000u);
    EXPECT_LE(first.count, 20000u + c_maxSearchDepth);
    EXPECT_EQ(first.count, second.count);
    EXPECT_EQ(first.move(), second.move());
    EXPECT_FALSE(first.move().isNull());
}

TEST_F(UciFixture, go_nodes_CombinesWithDepth)
{
    // setup
    m_uci.Enable();
    m_uci.NewGame();

    std::stringstream testOutput;
    {
        ScopedRedirect coutRedirect(std::cout, testOutput);

        // do, a depth that is reached long before the node limit.
        m_uci.Go({ "depth", "2", "nodes", "10000000" });
    }

    // verify
    EXPECT_NE(std::string::npos, testOutput.str().find("depth 2"));
    EXPECT_EQ(std::string::npos, testOutput.str().find("depth 3"));
    EXPECT_NE(std::string::npos, testOutput.str().find("bestmove"));
}

TEST_F(UciFixture, go_mate_1_FindsTheMatingMove)
{
    // setup, back rank mate in one.
    m_uci.Enable();
    std::list<std::string> position;
    extractArgsFromCommand("fen 6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1", position);
    m_uci.Position(position);

    std::stringstream testOutput;
    {
        ScopedRedirect coutRedirect(std::cout, testOutput);

        // do
        m_uci.Go({ "mate", "1" });
    }

    // verify
    EXPECT_NE(std::string::npos, testOutput.str().find("score mate 1"));
    EXPECT_NE(std::string::npos, testOutput.str().find("bestmove a1a8"));
}

TEST_F(UciFixture, go_mate_2_FindsTheMateInTwo)
{
    // setup, no mate in one. 1. Kg6 Kg8 2. Ra8# or 1. Kf7 Kh7 2. Rh1#.
    m_uci.Enable();
    std::list<std::string> position;
    extractArgsFromCommand("fen 7k/8/5K2/8/8/8/8/R7 w - - 0 1", position);
    m_uci.Position(position);

    std::stringstream testOutput;
    {
        ScopedRedirect coutRedirect(std::cout, testOutput);

        // do
        m_uci.Go({ "mate", "2" });
    }

    // verify
    const std::string output = testOutput.str();
    EXPECT_NE(std::string::npos, output.find("score mate 2"));
    EXPECT_TRUE(output.find("bestmove f6g6") != std::string::npos || output.find("bestmove f6f7") != std::string::npos)
        << output;
}

TEST_F(UciFixture, go_mate_WithoutAMateStillEndsWithABestmove)
{
    // setup
    m_uci.Enable();
    m_uci.NewGame();

    std::stringstream testOutput;
    {
        ScopedRedirect coutRedirect(std::cout, testOutput);

        // do, no mate in two from the start position, the search ends at depth 4.
        m_uci.Go({ "mate", "2" });
    }

    // verify
    EXPECT_NE(std::string::npos, testOutput.str().find("depth 4"));
    EXPECT_EQ(std::string::npos, testOutput.str().find("depth 5"));
    EXPECT_NE(std::string::npos, testOutput.str().find("bestmove"));
}

TEST_F(UciFixture, go_searchmoves_RestrictsTheRoot)
{
    // setup
    m_uci.Enable();
    m_uci.NewGame();

    std::stringstream testOutput;
    {
        ScopedRedirect coutRedirect(std::cout, testOutput);

        // do, the moves list ends at the next option.
        m_uci.Go({ "searchmoves", "a2a3", "depth", "3" });
    }

    // verify
    EXPECT_NE(std::string::npos, testOutput.str().find("depth 3"));
    EXPECT_EQ(std::string::npos, testOutput.str().find("depth 4"));
    EXPECT_NE(std::string::npos, testOutput.str().find("bestmove a2a3"));
}

TEST_F(UciFixture, go_searchmoves_MatingMoveLeftOutIsNotPlayed)
{
    // setup, a1a8 mates but isn't among the moves to search.
    m_uci.Enable();
    std::list<std::string> position;
    extractArgsFromCommand("fen 6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1", position);
    m_uci.Position(position);

    std::stringstream testOutput;
    {
        ScopedRedirect coutRedirect(std::cout, testOutput);

        // do
        m_uci.Go({ "depth", "3", "searchmoves", "a1a2", "g1f1" });
    }

    // verify
    const std::string output = testOutput.str();
    EXPECT_EQ(std::string::npos, output.find("bestmove a1a8"));
    EXPECT_TRUE(output.find("bestmove a1a2") != std::string::npos || output.find("bestmove g1f1") != std::string::npos)
        << output;
}

TEST_F(UciFixture, go_searchmoves_NoLegalMoveGivenSearchesAllMoves)
{
    // setup
    m_uci.Enable();
    m_uci.NewGame();

    std::stringstream testOutput;
    {
        ScopedRedirect coutRedirect(std::cout, testOutput);

        // do
        m_uci.Go({ "searchmoves", "e2e5", "depth", "2" });
    }

    // verify
    EXPECT_NE(std::string::npos, testOutput.str().find("bestmove"));
    EXPECT_EQ(std::string::npos, testOutput.str().find("bestmove 0000"));
    EXPECT_EQ(std::string::npos, testOutput.str().find("bestmove e2e5"));
}

TEST_F(UciFixture, go_InvalidValue_IsReportedAndSearchStillReturnsAMove)
{
    // setup
    m_uci.Enable();
    m_uci.NewGame();

    std::stringstream testOutput;
    bool result = false;
    {
        ScopedRedirect coutRedirect(std::cout, testOutput);

        // do
        EXPECT_NO_THROW(result = m_uci.Go({ "nodes", "abc", "depth", "2" }));
    }

    // verify
    EXPECT_TRUE(result);
    EXPECT_NE(std::string::npos, testOutput.str().find("info string error"));
    EXPECT_NE(std::string::npos, testOutput.str().find("bestmove"));
}

TEST_F(UciFixture, setoption_UnknownOption_IsReportedAndDoesNotThrow)
{
    // setup
    m_uci.Enable();

    // do
    std::list<std::string> args;
    extractArgsFromCommand("setoption name Foo value 1", args);
    args.pop_front();  // pop setoption
    bool result = true;
    std::stringstream testOutput;
    {
        ScopedRedirect coutRedirect(std::cout, testOutput);
        EXPECT_NO_THROW(result = m_uci.SetOption(args));
    }

    // verify
    EXPECT_FALSE(result);
    EXPECT_NE(std::string::npos, testOutput.str().find("info string error"));
}

TEST_F(UciFixture, setoption_MalformedOption_DoesNotThrow)
{
    // setup
    m_uci.Enable();
    bool tooFewArguments = true;
    bool notANumber = true;

    // do
    EXPECT_NO_THROW(tooFewArguments = m_uci.SetOption({ "name", "Hash" }));
    EXPECT_NO_THROW(notANumber = m_uci.SetOption({ "name", "Hash", "value", "lots" }));

    // verify
    EXPECT_FALSE(tooFewArguments);
    EXPECT_FALSE(notANumber);
}

TEST_F(UciFixture, position_Malformed_IsReportedAndDoesNotThrow)
{
    // setup
    m_uci.Enable();
    bool noArguments = true;
    bool badMove = true;
    bool unknownKeyword = true;

    // do
    EXPECT_NO_THROW(noArguments = m_uci.Position({}));
    EXPECT_NO_THROW(badMove = m_uci.Position({ "startpos", "moves", "e2e4", "zz99" }));
    EXPECT_NO_THROW(unknownKeyword = m_uci.Position({ "somewhere" }));

    // verify
    EXPECT_FALSE(noArguments);
    EXPECT_FALSE(badMove);
    EXPECT_FALSE(unknownKeyword);
}

TEST_F(UciFixture, ucinewgame_ClearsTranspositionTableAndAge)
{
    // setup
    m_uci.Enable();
    auto& tt = m_uci.editGameContext().editTranspositionTable();
    const u64 hash = 0x123456789ABCDEF0ull;
    tt.setAge(5);
    tt.store(hash, PackedMove(Square::E2, Square::E4), 42, 7, TTF_CUT_EXACT);

    PackedMove move;
    Score score = 0;
    u8 depth = 0;
    TranspositionFlag flag = TTF_NONE;
    ASSERT_TRUE(tt.probe(hash, move, score, depth, flag));

    // do
    bool result = m_uci.NewGame();

    // verify
    EXPECT_TRUE(result);
    EXPECT_FALSE(tt.probe(hash, move, score, depth, flag));
    EXPECT_EQ(0, tt.getAge());
}

TEST_F(UciFixture, position_startpos_moves_KeepsTranspositionTable)
{
    // setup
    m_uci.Enable();
    m_uci.NewGame();
    auto& tt = m_uci.editGameContext().editTranspositionTable();
    const u64 hash = 0x123456789ABCDEF0ull;
    tt.setAge(5);
    tt.store(hash, PackedMove(Square::E2, Square::E4), 42, 7, TTF_CUT_EXACT);

    // do, the GUI sends this every move of a game.
    std::string commandLine = "position startpos moves e2e4 e7e5 g1f3";
    std::list<std::string> args;
    extractArgsFromCommand(commandLine, args);
    args.pop_front();  // pop position
    bool result = m_uci.Position(args);

    // verify
    EXPECT_TRUE(result);
    PackedMove move;
    Score score = 0;
    u8 depth = 0;
    TranspositionFlag flag = TTF_NONE;
    EXPECT_TRUE(tt.probe(hash, move, score, depth, flag));
    EXPECT_EQ(42, score);
    EXPECT_EQ(7, depth);
    EXPECT_EQ(5, tt.getAge());
}

TEST_F(UciFixture, ucinewgame_AfterSearch_RootIsNoLongerInTranspositionTable)
{
    // setup, a search leaves the root position in the TT.
    m_uci.Enable();
    m_uci.NewGame();
    std::list<std::string> positionArgs{ "startpos" };
    m_uci.Position(positionArgs);
    const u64 rootHash = m_uci.readGameContext().readChessPosition().hash();

    std::list<std::string> goArgs{ "depth", "4" };
    std::stringstream testOutput;
    {
        ScopedRedirect coutRedirect(std::cout, testOutput);
        m_uci.Go(goArgs);
    }

    auto& tt = m_uci.editGameContext().editTranspositionTable();
    PackedMove move;
    Score score = 0;
    u8 depth = 0;
    TranspositionFlag flag = TTF_NONE;
    ASSERT_TRUE(tt.probe(rootHash, move, score, depth, flag));
    ASSERT_NE(0, tt.getAge());

    // do
    m_uci.NewGame();

    // verify
    EXPECT_FALSE(tt.probe(rootHash, move, score, depth, flag));
    EXPECT_EQ(0, tt.getAge());
}

}  // namespace ElephantTest