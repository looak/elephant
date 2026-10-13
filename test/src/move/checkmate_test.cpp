#include <gtest/gtest.h>

#include <string_view>
#include <vector>

#include <core/game_context.hpp>
#include <io/fen_parser.hpp>
#include <move/move.hpp>
#include <move_gen_ab.hpp>

namespace ElephantTest
{
////////////////////////////////////////////////////////////////

/**
 * @file checkmate_test.cpp
 * @brief Fixture for Check, Checkmates and Stalemates tests. Endgame tests.
 * A side with no legal moves is checkmated when its king is in check and stalemated when it isn't, the same test the
 * search does when its move generator runs dry.
 * Naming convention: <TestedFunctionality>_<TestedColor>_<ExpectedResult>
 * @author Alexander Loodin Ek *
 */

class CheckmateFixture : public ::testing::TestWithParam<Generator>
{
public:
    enum class Outcome { Playing, Checkmate, Stalemate };

    void setup(std::string_view fen)
    {
        m_context.Reset();
        ASSERT_TRUE(io::fen_parser::deserialize(fen, m_context.editChessboard())) << fen;
    }

    // the outcome for set if it were to move in the current position.
    Outcome outcome(Set set)
    {
        if (set == Set::WHITE)
            return outcome<Set::WHITE>();
        return outcome<Set::BLACK>();
    }

    template<Set set>
    Outcome outcome()
    {
        bool anyMove = false;
        forEachLegalMove<set>(GetParam(), m_context.readChessPosition(), [&](PackedMove) { anyMove = true; });
        if (anyMove)
            return Outcome::Playing;
        return isChecked<set>(GetParam(), m_context.readChessPosition()) ? Outcome::Checkmate : Outcome::Stalemate;
    }

    std::vector<PackedMove> legalMoves()
    {
        std::vector<PackedMove> moves;
        auto add = [&](PackedMove mv) { moves.push_back(mv); };
        if (m_context.readToPlay() == Set::WHITE)
            forEachLegalMove<Set::WHITE>(GetParam(), m_context.readChessPosition(), add);
        else
            forEachLegalMove<Set::BLACK>(GetParam(), m_context.readChessPosition(), add);
        return moves;
    }

    bool sideToMoveIsChecked()
    {
        if (m_context.readToPlay() == Set::WHITE)
            return isChecked<Set::WHITE>(GetParam(), m_context.readChessPosition());
        return isChecked<Set::BLACK>(GetParam(), m_context.readChessPosition());
    }

    GameContext m_context;
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
// Black to move, checkmate in 1 move, Qxd1#.
TEST_P(CheckmateFixture, BackRankCheckmate_Black_Checkmate)
{
    setup("3qk3/8/8/8/8/8/5PPP/3R2K1 b - - 0 1");

    EXPECT_EQ(Outcome::Playing, outcome(Set::WHITE));
    EXPECT_EQ(Outcome::Playing, outcome(Set::BLACK));

    PackedMove Qxd1 = PackedMove::NullMove();
    for (PackedMove mv : legalMoves()) {
        if (mv.sourceSqr() == Square::D8 && mv.targetSqr() == Square::D1)
            Qxd1 = mv;
    }
    ASSERT_FALSE(Qxd1.isNull());
    EXPECT_TRUE(Qxd1.isCapture());
    m_context.MakeMove(Qxd1);

    EXPECT_EQ(Set::WHITE, m_context.readToPlay());
    EXPECT_EQ(Outcome::Checkmate, outcome(Set::WHITE));
    EXPECT_EQ(Outcome::Playing, outcome(Set::BLACK));
    EXPECT_TRUE(legalMoves().empty());
}

// 8 [k][ ][ ][ ][ ][ ][ ][ ]
// 7 [ ][ ][Q][ ][ ][ ][ ][ ]
// 6 [ ][K][ ][ ][ ][ ][ ][ ]
// 5 [ ][ ][ ][ ][ ][ ][ ][ ]
// 4 [ ][ ][ ][ ][ ][ ][ ][ ]
// 3 [ ][ ][ ][ ][ ][ ][ ][ ]
// 2 [ ][ ][ ][ ][ ][ ][ ][ ]
// 1 [ ][ ][ ][ ][ ][ ][ ][ ]
//    A  B  C  D  E  F  G  H
// k7/2Q5/1K6/8/8/8/8/8 b - - 0 1
// Black to move but they're in stalemate.
TEST_P(CheckmateFixture, BlackToMoveInStalemate_Black_Stalemate)
{
    setup("k7/2Q5/1K6/8/8/8/8/8 b - - 0 1");

    EXPECT_FALSE(sideToMoveIsChecked());
    EXPECT_TRUE(legalMoves().empty());
    EXPECT_EQ(Outcome::Stalemate, outcome(Set::BLACK));
    EXPECT_EQ(Outcome::Playing, outcome(Set::WHITE));
}

// r1bqkbnr/ppp2Qpp/2np4/4p3/2B1P3/8/PPPP1PPP/RNB1K1NR b KQkq - 0 4
// Scholar's mate, the queen on f7 is guarded by the bishop.
TEST_P(CheckmateFixture, ScholarsMate_Black_Checkmate)
{
    setup("r1bqkbnr/ppp2Qpp/2np4/4p3/2B1P3/8/PPPP1PPP/RNB1K1NR b KQkq - 0 4");

    EXPECT_EQ(Outcome::Checkmate, outcome(Set::BLACK));
    EXPECT_EQ(Outcome::Playing, outcome(Set::WHITE));
}

// r1bqkbnr/ppp2ppp/2np4/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 2 4
// the queen on h5 has 13 moves, 3 of them captures, Qxe5+ & Qxf7# give check and Qxf7 mates.
TEST_P(CheckmateFixture, ScholarsMateQueenMovesIntoMate_White_OneCheckmate)
{
    setup("r1bqkbnr/ppp2ppp/2np4/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 2 4");

    int queenMoves = 0;
    int captures = 0;
    int checks = 0;
    int checkmates = 0;
    for (PackedMove mv : legalMoves()) {
        if (mv.sourceSqr() != Square::H5)
            continue;

        queenMoves++;
        captures += mv.isCapture() ? 1 : 0;

        m_context.MakeMove(mv);
        checks += sideToMoveIsChecked() ? 1 : 0;
        if (outcome(Set::BLACK) == Outcome::Checkmate) {
            checkmates++;
            EXPECT_EQ(Square::F7, mv.targetSqr());
        }
        EXPECT_TRUE(m_context.UnmakeMove());
    }

    EXPECT_EQ(13, queenMoves);
    EXPECT_EQ(3, captures);
    EXPECT_EQ(2, checks);
    EXPECT_EQ(1, checkmates);
}

ELEPHANT_MOVE_GEN_AB(CheckmateFixture);

} // namespace ElephantTest
