#pragma once
#include <position/position_accessors.hpp>

namespace ElephantTest {
namespace chess_positions {

/**
* 8 [ r ][ n ][ b ][ q ][ k ][ b ][ n ][ r ]
* 7 [ p ][ p ][ p ][ p ][ p ][ p ][ p ][ p ]
* 6 [   ][   ][   ][   ][   ][   ][   ][   ]
* 5 [   ][   ][   ][   ][   ][   ][   ][   ]
* 4 [   ][   ][   ][   ][   ][   ][   ][   ]
* 3 [   ][   ][   ][   ][   ][   ][   ][   ]
* 2 [ P ][ P ][ P ][ P ][ P ][ P ][ P ][ P ]
* 1 [ R ][ N ][ B ][ Q ][ K ][ B ][ N ][ R ]
*     A    B    C    D    E    F    G    H
* fen: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1   */ 
void defaultStartingPosition(PositionEditor position) {

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

    position.placePiece(R, Square::A1);
    position.placePiece(N, Square::B1);
    position.placePiece(B, Square::C1);
    position.placePiece(Q, Square::D1);
    position.placePiece(K, Square::E1);
    position.placePiece(B, Square::F1);
    position.placePiece(N, Square::G1);
    position.placePiece(R, Square::H1);

    position.placePiece(P, Square::A2);
    position.placePiece(P, Square::B2);
    position.placePiece(P, Square::C2);
    position.placePiece(P, Square::D2);
    position.placePiece(P, Square::E2);
    position.placePiece(P, Square::F2);
    position.placePiece(P, Square::G2);
    position.placePiece(P, Square::H2);

    position.placePiece(r, Square::A8);
    position.placePiece(n, Square::B8);
    position.placePiece(b, Square::C8);
    position.placePiece(q, Square::D8);
    position.placePiece(k, Square::E8);
    position.placePiece(b, Square::F8);
    position.placePiece(n, Square::G8);
    position.placePiece(r, Square::H8);

    position.placePiece(p, Square::A7);
    position.placePiece(p, Square::B7);
    position.placePiece(p, Square::C7);
    position.placePiece(p, Square::D7);
    position.placePiece(p, Square::E7);
    position.placePiece(p, Square::F7);
    position.placePiece(p, Square::G7);
    position.placePiece(p, Square::H7);

    position.castling().grantAll();
    position.enPassant().clear();
}

/**
* https://en.wikipedia.org/wiki/The_Game_of_the_Century_(chess)
* "Windmill Position from s.k. game of the century between Donald Bryen as White & Bobby
* Fischer as Black. Played at the Marshall Chess Club in New York City on October 17th 1956. At
* move 17 Fisher begins a windmill and that is where this board is.
* 8 [ r ][   ][   ][   ][ r ][ n ][ k ][   ]
* 7 [ p ][ b ][   ][   ][   ][ p ][ p ][   ]
* 6 [   ][   ][   ][ p ][ p ][   ][   ][ p ]
* 5 [   ][ q ][   ][   ][   ][   ][ B ][ Q ]
* 4 [   ][ P ][   ][ P ][   ][   ][   ][   ]
* 3 [   ][   ][   ][   ][ N ][   ][ R ][   ]
* 2 [ P ][   ][   ][   ][   ][ P ][ P ][ P ]
* 1 [ R ][   ][   ][   ][ R ][   ][ K ][   ]
*     A    B    C    D    E    F    G    H
* fen: r3rnk1/pb3pp1/3pp2p/1q4BQ/1P1P4/4N1R1/P4PPP/4R1K1 b - - 18 1    */
void windmillPosition(PositionEditor position) {
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

    position.placePiece(r, Square::A8);
    position.placePiece(r, Square::E8);
    position.placePiece(n, Square::F8);
    position.placePiece(k, Square::G8);

    position.placePiece(p, Square::A7);
    position.placePiece(b, Square::B7);
    position.placePiece(p, Square::F7);
    position.placePiece(p, Square::G7);

    position.placePiece(p, Square::D6);
    position.placePiece(p, Square::E6);
    position.placePiece(p, Square::H6);

    position.placePiece(q, Square::B5);
    position.placePiece(B, Square::G5);
    position.placePiece(Q, Square::H5);

    position.placePiece(P, Square::B4);
    position.placePiece(P, Square::D4);

    position.placePiece(N, Square::E3);
    position.placePiece(R, Square::G3);

    position.placePiece(P, Square::A2);
    position.placePiece(P, Square::F2);
    position.placePiece(P, Square::G2);
    position.placePiece(P, Square::H2);

    position.placePiece(R, Square::E1);
    position.placePiece(K, Square::G1);

}


// The so far longest game in the history of world championship chess.
// 2021 World Championship between Magnus Carlsen and Ian Nepomniachtchi.
// At this position, in game 6 Nepomniachtchi resigned.
void nepomniachtchiResignsGameSix(PositionEditor position) {
    position.placePiece(BLACKQUEEN, Square::G1);
    position.placePiece(BLACKKING, Square::D8);
    position.placePiece(WHITEPAWN, Square::E6);
    position.placePiece(WHITEPAWN, Square::F5);
    position.placePiece(WHITEKNIGHT, Square::G7);
    position.placePiece(WHITEROOK, Square::F7);
    position.placePiece(WHITEKING, Square::H4);
}


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

void move18_FischerSpassky(PositionEditor position) {
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

    position.placePiece(R, Square::A1);
    position.placePiece(N, Square::B1);
    position.placePiece(Q, Square::D1);
    position.placePiece(R, Square::E1);
    position.placePiece(K, Square::G1);

    position.placePiece(P, Square::A2);
    position.placePiece(P, Square::B2);
    position.placePiece(P, Square::F2);
    position.placePiece(P, Square::G2);

    position.placePiece(B, Square::B3);
    position.placePiece(N, Square::F3);
    position.placePiece(P, Square::H3);

    position.placePiece(p, Square::B4);
    position.placePiece(n, Square::E4);

    position.placePiece(p, Square::C5);
    position.placePiece(P, Square::E5);

    position.placePiece(p, Square::D6);
    position.placePiece(p, Square::H6);

    position.placePiece(b, Square::B7);
    position.placePiece(n, Square::D7);
    position.placePiece(q, Square::E7);
    position.placePiece(p, Square::F7);
    position.placePiece(p, Square::G7);

    position.placePiece(r, Square::A8);
    position.placePiece(r, Square::F8);
    position.placePiece(k, Square::G8);

    position.castling().clear();
}

} // namespace chess_positions
} // namespace ElephantTest
