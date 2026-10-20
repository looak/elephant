#include <gtest/gtest.h>

#include <io/fen_parser.hpp>
#include <core/uci.hpp>
#include <elephant_gambit_config.h>
#include <core/game_context.hpp>
#include <search/search.hpp>
#include <system/time_manager.hpp>
#include <search/search_heuristic_structures.hpp>

#include <algorithm>
#include <chrono>
#include <map>
#include <sstream>
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

// buildinfo is non standard, a test runner reads it to tell builds apart. Every line is "buildinfo <key> <value>",
// the version & the build settings are always there.
TEST_F(UciFixture, buildinfo_Outputs_KeyValueLines)
{
    std::stringstream testOutput;
    {
        ScopedRedirect coutRedirect(std::cout, testOutput);
        EXPECT_TRUE(m_uci.BuildInfo());
    }

    std::map<std::string, std::string> entries;
    std::string line;
    while (std::getline(testOutput, line)) {
        ASSERT_TRUE(line.starts_with("buildinfo ")) << line;
        const auto keyEnd = line.find(' ', 10);
        ASSERT_NE(keyEnd, std::string::npos) << line;
        entries[line.substr(10, keyEnd - 10)] = line.substr(keyEnd + 1);
    }

    EXPECT_EQ(ELEPHANT_GAMBIT_VERSION_FULL, entries["version"]);
    for (const char* key : { "compiler", "config", "assertions", "exceptions", "bits" })
        EXPECT_FALSE(entries[key].empty()) << key;
    if (entries.contains("commit"))
        EXPECT_EQ(40u, entries["commit"].size());
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
    EXPECT_EQ(c_startPositionFen, outputFen);
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
    EXPECT_STREQ("6Q1/2k5/8/8/8/8/8/2K5 b - - 0 1", outputFen.c_str());  // a promotion is a pawn move, the clock resets
}

namespace {
// the largest "nodes" value of the iteration lines, the node count of the last completed iteration.
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

// the final "info nodes N" line before bestmove, every node searched.
u64 finalReportedNodes(const std::string& output)
{
    const size_t pos = output.rfind("info nodes ");
    return pos == std::string::npos ? 0 : std::stoull(output.substr(pos + 11));
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

    // verify, completed iterations never report more than the limit. The final line counts every node, the limit plus
    // at most the nodes counted while unwinding, and comes right before bestmove.
    const std::string output = testOutput.str();
    EXPECT_TRUE(result);
    EXPECT_NE(std::string::npos, output.find("bestmove"));
    EXPECT_EQ(std::string::npos, output.find("bestmove 0000"));
    const std::string iterations = output.substr(0, output.rfind("info nodes "));
    EXPECT_GT(lastReportedNodes(iterations), 0u);
    EXPECT_LE(lastReportedNodes(iterations), 5000u);
    EXPECT_GE(finalReportedNodes(output), 5000u);
    EXPECT_LE(finalReportedNodes(output), 5000u + c_maxSearchDepth);
    EXPECT_LT(output.rfind("info nodes "), output.rfind("bestmove"));
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

// Only a pawn captures en passant, issue #14. A knight or king stepping onto the empty en passant square is a quiet
// move: no capture flag, the pawn that double pushed stays and the halfmove clock counts up instead of resetting.
TEST_F(UciFixture, position_moves_NonPawnOntoEnPassantSquare_IsNotACapture)
{
    struct Case {
        std::string fen;
        std::string moves;
        Square emptied;     // source square of the last move
        Square target;      // the en passant square
        ChessPiece piece;   // the piece that moved onto it
        Square pawn;        // the pawn that double pushed, still on the board
        std::string expectedFen;
    };
    const Case cases[] = {
        { "rnbqkb1r/pppppppp/8/8/6n1/8/PPPPPPPP/RNBQKBNR w KQkq - 3 3", "e2e4 g4e3", Square::G4, Square::E3, BLACKKNIGHT,
          Square::E4, "rnbqkb1r/pppppppp/8/8/4P3/4n3/PPPP1PPP/RNBQKBNR w KQkq - 1 4" },
        { "4k3/3p4/8/4K3/8/8/8/8 b - - 5 40", "d7d5 e5d6", Square::E5, Square::D6, WHITEKING,
          Square::D5, "4k3/8/3K4/3p4/8/8/8/8 b - - 1 41" },
    };

    for (const auto& c : cases) {
        UCI uci;
        uci.Enable();
        std::list<std::string> args;
        extractArgsFromCommand("position fen " + c.fen + " moves " + c.moves, args);
        args.pop_front();  // pop position
        EXPECT_TRUE(uci.Position(args)) << c.fen;

        const auto& history = uci.readGameContext().readGameHistory().moveUndoUnits;
        ASSERT_EQ(2u, history.size()) << c.fen;
        EXPECT_FALSE(history.back().move.isCapture()) << c.moves;
        EXPECT_FALSE(history.back().move.isEnPassant()) << c.moves;
        EXPECT_EQ(ChessPiece::None(), history.back().capturedPiece) << c.moves;

        const auto& board = uci.readGameContext().readChessPosition();
        EXPECT_EQ(c.piece, board.pieceAt(c.target)) << c.moves;
        EXPECT_EQ(ChessPiece::None(), board.pieceAt(c.emptied)) << c.moves;
        EXPECT_EQ(PieceType::PAWN, board.pieceAt(c.pawn).getType()) << c.moves;
        EXPECT_FALSE(board.enPassant()) << c.moves;
        EXPECT_EQ(1, uci.readGameContext().readPly()) << c.moves;

        std::string outputFen;
        io::fen_parser::serialize(uci.readGameContext().readChessboard(), outputFen);
        EXPECT_EQ(c.expectedFen, outputFen);
    }
}

// A pawn onto the en passant square is still an en passant capture, flagged as one so unmake puts the pawn back on
// the right square.
TEST_F(UciFixture, position_moves_PawnOntoEnPassantSquare_CapturesEnPassant)
{
    m_uci.Enable();
    const std::string fen = "4k3/3p4/8/4P3/8/8/8/4K3 b - - 5 40";
    std::list<std::string> args;
    extractArgsFromCommand("position fen " + fen + " moves d7d5 e5d6", args);
    args.pop_front();  // pop position
    EXPECT_TRUE(m_uci.Position(args));

    const auto& history = m_uci.readGameContext().readGameHistory().moveUndoUnits;
    ASSERT_EQ(2u, history.size());
    EXPECT_TRUE(history.back().move.isCapture());
    EXPECT_TRUE(history.back().move.isEnPassant());

    const auto& board = m_uci.readGameContext().readChessPosition();
    EXPECT_EQ(WHITEPAWN, board.pieceAt(Square::D6));
    EXPECT_EQ(ChessPiece::None(), board.pieceAt(Square::D5));
    EXPECT_EQ(ChessPiece::None(), board.pieceAt(Square::E5));
    EXPECT_EQ(0, m_uci.readGameContext().readPly());

    std::string outputFen;
    io::fen_parser::serialize(m_uci.readGameContext().readChessboard(), outputFen);
    EXPECT_EQ("4k3/8/3P4/8/8/8/8/4K3 b - - 0 41", outputFen);
}


////////////////////////////////////////////////////////////////
// From a fastchess game which reported "PV continues after threefold repetition". h2h4 used to record (and hash) an en
// passant square on h3 although no black pawn stood on g4 to capture it, so the position after h2h4 hashed differently
// from the same position reached later by f3f4 and the third occurrence went unnoticed. The en passant square is now only
// recorded when an enemy pawn stands beside the pushed pawn.
namespace repetition_game {
const std::string fen = "r1bqkb1r/1p1n1ppp/p1p1pn2/2Pp4/3P2P1/2N1BN2/PP2PP1P/R2QKB1R w KQkq - 1 9";
const std::string movesUntilH4 =
    "g4g5 f6h5 a1c1 b7b6 f1h3 b6c5 h3g4 g7g6 d4c5 f8g7 e1g1 e8g8 d1a4 c8b7 e3d2 a8b8 b2b4 d8e7 c1d1 d7e5 f3e5 g7e5 "
    "f2f4 h5f4 f1f4 h7h5 g5h6 e7h4 g1h1 e5f4 d2f4 h4g4 f4b8 f8b8 a4a3 g4h5 a3c1 e6e5 d1g1 b7c8 c1g5 h5g5 g1g5 c8f5 "
    "a2a3 d5d4 c3a4 g8h7 a4b6 h7h6 g5g3 e5e4 g3g1 d4d3 h1g2 b8d8 e2d3 e4d3 b6c4 d8d4 c4d2 a6a5 g1b1 a5b4 a3b4 h6g7 "
    "g2g3 d4g4 g3f2 g4h4 d2f3 h4c4 f3d2 c4f4 f2g3 f4g4 g3f2 g7f6 b4b5 c6b5 b1b5 f6e5 b5b7 e5d5 b7f7 g4h4 d2f3 h4f4 "
    "f7f8 d5c5 f8d8 c5c4 f2g3 f4g4 g3f2 c4c3 d8d6 g4e4 d6c6 c3b2 f2g3 e4e2 c6d6 b2c3 d6c6 c3b2 c6d6 e2c2 g3f4 c2c4 "
    "f4e3 c4e4 e3f2 b2c3 d6c6 c3b4 c6d6 e4e2 f2g3 b4c3 d6c6 c3b4 c6d6 f5e4 d6d7 b4c3 f3d4 e2e1 d4b5 c3b4 b5c7 e1e3 "
    "g3f4 e3e2 d7d4 b4b3 h2h4";
// the game continued, then the engine (white) played f4f3 f5e4 f3f4 in its PV.
const std::string movesAfterH4 = "e4f5 f4f3 f5e4 f3f4 e4f5";
}  // namespace repetition_game

// Threefold repetition of the current position, judged the way the search does: the hashes before every game move
// plus the current one.
static bool isThreefold(const GameContext& context)
{
    MoveHistory history;
    for (const auto& undoUnit : context.readGameHistory().moveUndoUnits)
        history.push(undoUnit.hash);
    const u64 hash = context.readChessPosition().hash();
    history.push(hash);
    return history.isRepetition(hash, context.readPly());
}

TEST_F(UciFixture, position_DoublePushWithoutCapturer_RepetitionDetected)
{
    m_uci.Enable();
    std::list<std::string> args;

    // after h2h4, no black pawn on g4 could capture on h3 so there's no en passant square.
    extractArgsFromCommand("fen " + repetition_game::fen + " moves " + repetition_game::movesUntilH4, args);
    ASSERT_TRUE(m_uci.Position(args));
    {
        const auto position = m_uci.readGameContext().readChessPosition();
        EXPECT_EQ(Set::BLACK, m_uci.readGameContext().readToPlay());
        EXPECT_EQ(WHITEPAWN, position.pieceAt(Square::H4));
        EXPECT_EQ(WHITEKING, position.pieceAt(Square::F4));
        EXPECT_EQ(BLACKBISHOP, position.pieceAt(Square::E4));
        EXPECT_NE(BLACKPAWN, position.pieceAt(Square::G4));
        EXPECT_FALSE(position.enPassant());
    }
    const u64 afterH4 = m_uci.readGameContext().readChessPosition().hash();
    EXPECT_FALSE(isThreefold(m_uci.readGameContext()));

    // e4f5 f4f3 f5e4 f3f4 reaches it a second time, not yet a draw.
    args.clear();
    extractArgsFromCommand("fen " + repetition_game::fen + " moves " + repetition_game::movesUntilH4 + " e4f5 f4f3 f5e4 f3f4", args);
    ASSERT_TRUE(m_uci.Position(args));
    EXPECT_EQ(afterH4, m_uci.readGameContext().readChessPosition().hash());
    EXPECT_FALSE(isThreefold(m_uci.readGameContext()));

    // the game moves then the engine's PV f4f3 f5e4 f3f4 reaches it a third time.
    args.clear();
    extractArgsFromCommand("fen " + repetition_game::fen + " moves " + repetition_game::movesUntilH4 + " " +
                           repetition_game::movesAfterH4 + " f4f3 f5e4 f3f4", args);
    ASSERT_TRUE(m_uci.Position(args));
    EXPECT_EQ(afterH4, m_uci.readGameContext().readChessPosition().hash());
    EXPECT_TRUE(isThreefold(m_uci.readGameContext()));
}

// After the game moves and f4f3 f5e4, white (worse off) can claim the draw with f3f4, the position after h2h4 for the
// third time. The search has to see it as a draw and the PV must stop there, like fastchess checks.
TEST_F(UciFixture, go_DoublePushWithoutCapturer_PVStopsAtRepetition)
{
    m_uci.Enable();
    const std::string gameMoves = repetition_game::movesUntilH4 + " " + repetition_game::movesAfterH4 + " f4f3 f5e4";
    std::list<std::string> args;
    extractArgsFromCommand("fen " + repetition_game::fen + " moves " + gameMoves, args);
    ASSERT_TRUE(m_uci.Position(args));

    std::stringstream testOutput;
    {
        ScopedRedirect coutRedirect(std::cout, testOutput);
        ASSERT_TRUE(m_uci.Go({ "depth", "10" }));
    }

    // last pv printed.
    std::string pv;
    std::string line;
    while (std::getline(testOutput, line)) {
        const size_t at = line.find(" pv ");
        if (at != std::string::npos)
            pv = line.substr(at + 4);
    }
    std::list<std::string> pvMoves;
    extractArgsFromCommand(pv, pvMoves);
    pvMoves.remove("");
    ASSERT_FALSE(pvMoves.empty()) << testOutput.str();
    EXPECT_EQ("f3f4", pvMoves.front()) << pv;
    EXPECT_NE(std::string::npos, testOutput.str().find("bestmove f3f4"));

    // replay the game & pv one move at a time and compare positions by FEN (pieces, side to move & castling, without the
    // en passant field) instead of hashes, so the check doesn't share the engine's hashing. No pv move may follow a
    // threefold repetition. Dropping the en passant field entirely is fine here, no en passant capture is possible.
    std::list<std::string> allMoves;
    extractArgsFromCommand(gameMoves, allMoves);
    const size_t gameLength = allMoves.size();
    allMoves.insert(allMoves.end(), pvMoves.begin(), pvMoves.end());

    std::vector<std::string> keys;
    std::string played;
    for (size_t ply = 0; ply <= allMoves.size(); ++ply) {
        if (ply > 0)
            played += " " + *std::next(allMoves.begin(), static_cast<std::ptrdiff_t>(ply - 1));
        args.clear();
        extractArgsFromCommand("fen " + repetition_game::fen + (played.empty() ? "" : " moves" + played), args);
        ASSERT_TRUE(m_uci.Position(args));
        std::string fen;
        io::fen_parser::serialize(m_uci.readGameContext().readChessboard(), fen);
        std::list<std::string> fields;
        extractArgsFromCommand(fen, fields);
        keys.push_back(fields.front() + " " + *std::next(fields.begin()) + " " + *std::next(fields.begin(), 2));

        if (ply <= gameLength)
            continue;
        if (std::count(keys.begin(), keys.end(), keys.back()) >= 3) {
            EXPECT_EQ(allMoves.size(), ply) << "PV continues after threefold repetition: " << pv;
            break;
        }
    }
    EXPECT_EQ(3, std::count(keys.begin(), keys.end(), keys[gameLength + 1])) << "f3f4 should be the third occurrence";
}

}  // namespace ElephantTest
