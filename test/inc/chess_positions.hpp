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
void defaultStartingPosition(PositionEditor position);

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
void windmillPosition(PositionEditor position);

// The so far longest game in the history of world championship chess.
// 2021 World Championship between Magnus Carlsen and Ian Nepomniachtchi.
// At this position, in game 6 Nepomniachtchi resigned.
void nepomniachtchiResignsGameSix(PositionEditor position);


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

void move18_FischerSpassky(PositionEditor position);

} // namespace chess_positions
} // namespace ElephantTest
