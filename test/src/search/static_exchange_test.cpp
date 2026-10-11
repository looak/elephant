#include <gtest/gtest.h>

#include <core/game_context.hpp>
#include <io/fen_parser.hpp>
#include <search/static_exchange.hpp>

namespace ElephantTest {
/**
 * @file static_exchange_test.cpp
 * @brief Fixture testing static exchange evaluation, values are see::value: P 100, N & B 300, R 500, Q 900.
 * Naming convention: <TestedFunction>_<Condition>_<ExpectedBehavior>
 * @author Alexander Loodin Ek *
 */
////////////////////////////////////////////////////////////////
class StaticExchangeFixture : public ::testing::Test {
public:
    void setPosition(const std::string& fen) {
        io::fen_parser::deserialize(fen.c_str(), m_context.editChessboard());
    }

    bool ge(PackedMove move, i32 threshold) {
        return see::ge(m_context.readChessPosition().material(), move, threshold);
    }

    static PackedMove capture(Square from, Square to) {
        PackedMove move(from, to);
        move.setCapture(true);
        return move;
    }

    GameContext m_context;
};
////////////////////////////////////////////////////////////////

TEST_F(StaticExchangeFixture, ge_UndefendedPawn_WinsAPawn)
{
    setPosition("4k3/8/8/3p4/4P3/8/8/4K3 w - - 0 1");
    PackedMove move = capture(Square::E4, Square::D5);

    EXPECT_TRUE(ge(move, 0));
    EXPECT_TRUE(ge(move, 100));
    EXPECT_FALSE(ge(move, 101));
}

TEST_F(StaticExchangeFixture, ge_QueenTakesPawnDefendedByPawn_LosesTheQueen)
{
    setPosition("4k3/8/2p5/3p4/8/8/3Q4/4K3 w - - 0 1");
    PackedMove move = capture(Square::D2, Square::D5);

    EXPECT_FALSE(ge(move, 0));
    EXPECT_TRUE(ge(move, -800));
    EXPECT_FALSE(ge(move, -799));
}

TEST_F(StaticExchangeFixture, ge_RookTakesPawnDefendedByKnight_XrayRookOnlyRecoversTheKnight)
{
    // RxP, NxR, RxN: 100 - 500 + 300.
    setPosition("4k3/8/5n2/3p4/8/8/3R4/3RK3 w - - 0 1");
    PackedMove move = capture(Square::D2, Square::D5);

    EXPECT_FALSE(ge(move, 0));
    EXPECT_TRUE(ge(move, -100));
    EXPECT_FALSE(ge(move, -99));
}

TEST_F(StaticExchangeFixture, ge_DoubledRooksAgainstOneDefender_WinThePawn)
{
    // RxP, RxR, RxR: black is better off not recapturing, white keeps the pawn.
    setPosition("3rk3/8/8/3p4/8/8/3R4/3RK3 w - - 0 1");
    PackedMove move = capture(Square::D2, Square::D5);

    EXPECT_TRUE(ge(move, 100));
    EXPECT_FALSE(ge(move, 101));
}

TEST_F(StaticExchangeFixture, ge_EnPassant_WinsAPawn)
{
    setPosition("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");
    PackedMove move(Square::E5, Square::D6);
    move.setEnPassant(true);

    EXPECT_TRUE(ge(move, 100));
    EXPECT_FALSE(ge(move, 101));
}

TEST_F(StaticExchangeFixture, ge_UndefendedQueenPromotion_GainsQueenMinusPawn)
{
    setPosition("4k3/P7/8/8/8/8/8/4K3 w - - 0 1");
    PackedMove move(Square::A7, Square::A8);
    move.setPromoteTo(queenId);

    EXPECT_TRUE(ge(move, 800));
    EXPECT_FALSE(ge(move, 801));
}

TEST_F(StaticExchangeFixture, ge_KingIsOnlyDefender_RecapturesTheQueen)
{
    setPosition("8/8/4k3/3p4/8/8/8/3QK3 w - - 0 1");
    PackedMove move = capture(Square::D1, Square::D5);

    EXPECT_FALSE(ge(move, 0));
    EXPECT_TRUE(ge(move, -800));
}

TEST_F(StaticExchangeFixture, ge_KingCantRecaptureADefendedSquare_QueenWinsThePawn)
{
    // the rook behind the queen defends d5, the king may not take.
    setPosition("8/8/4k3/3p4/8/8/3Q4/3RK3 w - - 0 1");
    PackedMove move = capture(Square::D2, Square::D5);

    EXPECT_TRUE(ge(move, 100));
    EXPECT_FALSE(ge(move, 101));
}

TEST_F(StaticExchangeFixture, ge_BlackCaptures_SeesFromBlacksSide)
{
    // black pawn takes a knight defended by a pawn, PxN, PxP: 300 - 100.
    setPosition("4k3/8/8/4p3/3N4/2P5/8/4K3 b - - 0 1");
    PackedMove move = capture(Square::E5, Square::D4);

    EXPECT_TRUE(ge(move, 200));
    EXPECT_FALSE(ge(move, 201));
}

TEST_F(StaticExchangeFixture, ge_QuietMoveToAttackedSquare_LosesThePiece)
{
    // Nf3-e5 walks into the pawn on d6.
    setPosition("4k3/8/3p4/8/8/5N2/8/4K3 w - - 0 1");
    PackedMove move(Square::F3, Square::E5);

    EXPECT_FALSE(ge(move, 0));
    EXPECT_TRUE(ge(move, -300));
}

}  // namespace ElephantTest
