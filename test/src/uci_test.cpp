#include <gtest/gtest.h>
#include "elephant_test_utils.h"

#include "fen_parser.h"
#include "uci.hpp"

namespace ElephantTest
{
/**
 * @file uci_test.cpp
 * @brief Fixture testing Universal Chess Interface of engine
 * https://www.wbec-ridderkerk.nl/html/UCIProtocol.html
 * Naming convention: <UCICommand>_<ExpectedBehavior>
 * https://osherove.com/blog/2005/4/3/naming-standards-for-unit-tests.html
 * @author Alexander Loodin Ek * 
 */
////////////////////////////////////////////////////////////////
class UciFixture : public ::testing::Test
{
public:
    virtual void SetUp()
    {
    };
    virtual void TearDown()
    {
    };

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
        while (std::getline(ssargs, token, ' '))
        {
            tokens.push_back(token);
        }
    }

    UCI m_uci;
    
};
////////////////////////////////////////////////////////////////
/**
 * @class ScopedRedirect
 * @brief Redirects given ostream to another buffer which we later can verify */
class ScopedRedirect
{
public:
	ScopedRedirect(std::ostream& redirected, std::ostream& target) :
		m_originalStream(redirected),
		m_originalBuffer(redirected.rdbuf())
	{        
		m_originalStream.rdbuf(target.rdbuf());
	}
	~ScopedRedirect()
	{
		m_originalStream.rdbuf(m_originalBuffer);
	}

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
    bool result;
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
    EXPECT_EQ(0, m_uci.readGameContext().readMoveHistory().size());

    const auto& board = m_uci.readGameContext().readChessboard();
    EXPECT_EQ(WHITEKING, board.readTile(e1).readPiece());
    EXPECT_EQ(BLACKKING, board.readTile(e8).readPiece());
    EXPECT_EQ(WHITEQUEEN, board.readTile(d1).readPiece());
    EXPECT_EQ(BLACKQUEEN, board.readTile(d8).readPiece());

    EXPECT_TRUE(board.readCastlingStateInfo().hasAll());

    std::string outputFen;
    FENParser::serialize(m_uci.readGameContext(), outputFen);
    EXPECT_STREQ(c_startPositionFen.c_str(), outputFen.c_str());
}

TEST_F(UciFixture, position_fen_InitializesGameToGivenFen)
{
    // setup
    m_uci.Enable();
    std::string gocFen = "r3rnk1/pb3pp1/3pp2p/1q4BQ/1P1P4/4N1R1/P4PPP/4R1K1 b - - 18 1";
    GameContext expected;
    FENParser::deserialize(gocFen.c_str(), expected);

    // do
    std::string commandLine = "position fen " + gocFen;
    std::list<std::string> args;
    extractArgsFromCommand(commandLine, args);
    args.pop_front(); // pop position
    bool result = m_uci.Position(args);

    // verify
    EXPECT_TRUE(result);
    EXPECT_EQ(Set::BLACK, m_uci.readGameContext().readToPlay());
    EXPECT_EQ(0, m_uci.readGameContext().readMoveHistory().size());

    const auto& board = m_uci.readGameContext().readChessboard();
    EXPECT_EQ(WHITEKING, board.readTile(g1).readPiece());
    EXPECT_EQ(BLACKKING, board.readTile(g8).readPiece());

    EXPECT_FALSE(board.readCastlingStateInfo().hasAny());

    std::string outputFen;
    FENParser::serialize(m_uci.readGameContext(), outputFen);
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
    args.pop_front(); // pop go
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

}; // namespace ElephantTest