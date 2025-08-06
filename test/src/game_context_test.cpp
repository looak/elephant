#include <gtest/gtest.h>
#include "game_context.h"
#include <move/move.hpp>
#include "elephant_test_utils.h"
#include <position/hash_zorbist.hpp>


namespace ElephantTest
{
////////////////////////////////////////////////////////////////
class GameContextFixture : public ::testing::Test
{
public:


    GameContext m_context;
};

////////////////////////////////////////////////////////////////

// 8 [ r ][   ][   ][   ][   ][ r ][ k ][   ]
// 7 [   ][ b ][   ][ n ][ q ][ p ][ p ][   ]
// 6 [   ][   ][   ][ p ][   ][   ][   ][ p ]
// 5 [   ][   ][ p ][   ][ P ][   ][   ][   ]
// 4 [   ][ p ][   ][   ][ n ][   ][   ][   ]
// 3 [   ][ B ][   ][   ][   ][ N ][   ][ P ]
// 2 [ P ][ P ][   ][   ][   ][ P ][ P ][   ]
// 1 [ R ][ N ][   ][ Q ][ R ][   ][ K ][   ]
//     A    B    C    D    E    F    G    H
// https://en.wikipedia.org/wiki/Fischer%E2%80%93Spassky_(1992_match)#Game_29:_Fischer%E2%80%93Spassky,_%C2%BD%E2%80%93%C2%BD_(Ruy_Lopez_Breyer)

void BuildPositionAfterMove18(Chessboard& board)
{
    auto K = WHITEKING;
    auto Q = WHITEQUEEN;
    auto B = WHITEBISHOP;
    auto N = WHITEKNIGHT;
    auto R = WHITEROOK;
    auto P = WHITEPAWN;

    auto k = BLACKKING;
    auto q = BLACKQUEEN;
    auto b = BLACKBISHOP;
    auto n = BLACKKNIGHT;
    auto r = BLACKROOK;
    auto p = BLACKPAWN;

    board.PlacePiece(R, a1);
    board.PlacePiece(N, b1);
    board.PlacePiece(Q, d1);
    board.PlacePiece(R, e1);
    board.PlacePiece(K, g1);    

    board.PlacePiece(P, a2);
    board.PlacePiece(P, b2);    
    board.PlacePiece(P, f2);
    board.PlacePiece(P, g2);

    board.PlacePiece(B, b3);
    board.PlacePiece(N, f3);
    board.PlacePiece(P, h3);

    board.PlacePiece(p, b4);
    board.PlacePiece(n, e4);

    board.PlacePiece(p, c5);
    board.PlacePiece(P, e5);

    board.PlacePiece(p, d6);
    board.PlacePiece(p, h6);

    board.PlacePiece(b, b7);
    board.PlacePiece(n, d7);
    board.PlacePiece(q, e7);
    board.PlacePiece(p, f7);
    board.PlacePiece(p, g7);

    board.PlacePiece(r, a8);
    board.PlacePiece(r, f8);
    board.PlacePiece(k, g8);

    board.setCastlingState(0);
}

// need to rethink this test, between each move I want to disambiguate the move.
// TEST_F(GameContextFixture, GameReplay_FischerSpasky_ReturnMatch_AfterMove18)
// {
//     const std::string movePgn = "1. e4 e5 2. Nf3 Nc6 3. Bb5 a6 4. Ba4 Nf6 5. O-O Be7 6. Re1 b5 7. Bb3 d6 8. c3 O-O 9. h3 Nb8 10. d4 Nbd7 "
//         "11. c4 c6 12. cxb5 axb5 13. Nc3 Bb7 14. Bg5 b4 15. Nb1 h6 16. Bh4 c5 17. dxe5 Nxe4 18. Bxe7 Qxe7"; /*19. exd6 Qf6 20. Nbd2 Nxd6 "
//         "21. Nc4 Nxc4 22. Bxc4 Nb6 23. Ne5 Rae8 24. Bxf7+ Rxf7 25. Nxf7 Rxe1+ 26. Qxe1 Kxf7 27. Qe3 Qg5 28. Qxg5 hxg5 29. b3 Ke6 30. a3 Kd6 "
//         "31. axb4 cxb4 32. Ra5 Nd5 33. f3 Bc8 34. Kf2 Bf5 35. Ra7 g6 36. Ra6+ Kc5 37. Ke1 Nf4 38. g3 Nxh3 39. Kd2 Kb5 40. Rd6 Kc5 "
//         "41. Ra6 Nf2 42. g4 Bd3 43. Re6";*/

//     std::vector<Move> moves;
//     Move::ParsePGN(movePgn, moves);

//     SetupDefaultStartingPosition(m_context.editChessboard());
//     u64 hashOne = ZorbistHash::Instance().HashBoard(m_context.readChessboard());
//     EXPECT_EQ(m_context.readChessboard().readHash(), hashOne);

//     m_context.editMoveCount() = 1;

//     m_context.PlayMoves(moves[0]);

//     //PrintBoard(m_context.readChessboard());
//     Chessboard board;
//     BuildPositionAfterMove18(board);
//     //PrintBoard(board);

//     hashOne = ZorbistHash::Instance().HashBoard(m_context.readChessboard());
//     u64 hashTwo = ZorbistHash::Instance().HashBoard(board);
    
//     EXPECT_EQ(hashOne, hashTwo);
//     EXPECT_EQ(m_context.readChessboard().readHash(), hashOne);
//     EXPECT_EQ(board.readHash(), hashTwo);
//     EXPECT_EQ(m_context.readChessboard().readHash(), board.readHash());
// }
}