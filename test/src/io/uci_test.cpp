#include <gtest/gtest.h>

#include <io/fen_parser.hpp>
#include <core/uci.hpp>

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

TEST_F(UciFixture, DISABLED_position_fen_perft)
{
    // setup
    m_uci.Enable();
    std::string gocFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ;D1 20 ;D2 400 ;D3 8902";
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

}  // namespace ElephantTest
