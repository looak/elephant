#pragma once
#include <vector>
#include "defines.hpp"

class Chessboard;
struct Notation;
struct KingMask;
struct Move;

namespace ElephantTest {
void PrintBoard(const Chessboard& board);

// 8 [ r ][ n ][ b ][ q ][ k ][ b ][ n ][ r ]
// 7 [ p ][ p ][ p ][ p ][ p ][ p ][ p ][ p ]
// 6 [   ][   ][   ][   ][   ][   ][   ][   ]
// 5 [   ][   ][   ][   ][   ][   ][   ][   ]
// 4 [   ][   ][   ][   ][   ][   ][   ][   ]
// 3 [   ][   ][   ][   ][   ][   ][   ][   ]
// 2 [ P ][ P ][ P ][ P ][ P ][ P ][ P ][ P ]
// 1 [ R ][ N ][ B ][ Q ][ K ][ B ][ N ][ R ]
//     A    B    C    D    E    F    G    H
// fen: rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
void SetupDefaultStartingPosition(Chessboard& board);

bool NotationCompare(Notation lhs, Notation rhs);

bool VerifyListsContainSameNotations(std::vector<Notation> listOne, std::vector<Notation> listTwo);

u64 CombineKingMask(KingMask mask);

struct MoveCount {
    typedef std::function<bool(const Move&)> Predicate;
    void operator+=(const MoveCount& rhs)
    {
        this->Captures += rhs.Captures;
        this->Promotions += rhs.Promotions;
        this->EnPassants += rhs.EnPassants;
        this->Castles += rhs.Castles;
        this->Checks += rhs.Checks;
        this->Checkmates += rhs.Checkmates;
        this->Moves += rhs.Moves;
    }
    u32 Captures = 0;
    u32 Promotions = 0;
    u32 EnPassants = 0;
    u32 Castles = 0;
    u32 Checks = 0;
    u32 Checkmates = 0;
    u32 Moves = 0;
};

MoveCount CountMoves(
    const std::vector<Move>& moves, MoveCount::Predicate predicate = [](const Move&) -> bool { return true; });


#define BLACKPAWN piece_constants::black_pawn
#define BLACKKNIGHT piece_constants::black_knight
#define BLACKBISHOP piece_constants::black_bishop
#define BLACKROOK piece_constants::black_rook
#define BLACKQUEEN piece_constants::black_queen
#define BLACKKING piece_constants::black_king

#define WHITEPAWN piece_constants::white_pawn
#define WHITEKNIGHT piece_constants::white_knight
#define WHITEBISHOP piece_constants::white_bishop
#define WHITEROOK piece_constants::white_rook
#define WHITEQUEEN piece_constants::white_queen
#define WHITEKING piece_constants::white_king

#define a1 Notation::BuildPosition('a', 1)
#define a2 Notation::BuildPosition('a', 2)
#define a3 Notation::BuildPosition('a', 3)
#define a4 Notation::BuildPosition('a', 4)
#define a5 Notation::BuildPosition('a', 5)
#define a6 Notation::BuildPosition('a', 6)
#define a7 Notation::BuildPosition('a', 7)
#define a8 Notation::BuildPosition('a', 8)

#define b1 Notation::BuildPosition('b', 1)
#define b2 Notation::BuildPosition('b', 2)
#define b3 Notation::BuildPosition('b', 3)
#define b4 Notation::BuildPosition('b', 4)
#define b5 Notation::BuildPosition('b', 5)
#define b6 Notation::BuildPosition('b', 6)
#define b7 Notation::BuildPosition('b', 7)
#define b8 Notation::BuildPosition('b', 8)

#define c1 Notation::BuildPosition('c', 1)
#define c2 Notation::BuildPosition('c', 2)
#define c3 Notation::BuildPosition('c', 3)
#define c4 Notation::BuildPosition('c', 4)
#define c5 Notation::BuildPosition('c', 5)
#define c6 Notation::BuildPosition('c', 6)
#define c7 Notation::BuildPosition('c', 7)
#define c8 Notation::BuildPosition('c', 8)

#define d1 Notation::BuildPosition('d', 1)
#define d2 Notation::BuildPosition('d', 2)
#define d3 Notation::BuildPosition('d', 3)
#define d4 Notation::BuildPosition('d', 4)
#define d5 Notation::BuildPosition('d', 5)
#define d6 Notation::BuildPosition('d', 6)
#define d7 Notation::BuildPosition('d', 7)
#define d8 Notation::BuildPosition('d', 8)

#define e1 Notation::BuildPosition('e', 1)
#define e2 Notation::BuildPosition('e', 2)
#define e3 Notation::BuildPosition('e', 3)
#define e4 Notation::BuildPosition('e', 4)
#define e5 Notation::BuildPosition('e', 5)
#define e6 Notation::BuildPosition('e', 6)
#define e7 Notation::BuildPosition('e', 7)
#define e8 Notation::BuildPosition('e', 8)

#define f1 Notation::BuildPosition('f', 1)
#define f2 Notation::BuildPosition('f', 2)
#define f3 Notation::BuildPosition('f', 3)
#define f4 Notation::BuildPosition('f', 4)
#define f5 Notation::BuildPosition('f', 5)
#define f6 Notation::BuildPosition('f', 6)
#define f7 Notation::BuildPosition('f', 7)
#define f8 Notation::BuildPosition('f', 8)

#define g1 Notation::BuildPosition('g', 1)
#define g2 Notation::BuildPosition('g', 2)
#define g3 Notation::BuildPosition('g', 3)
#define g4 Notation::BuildPosition('g', 4)
#define g5 Notation::BuildPosition('g', 5)
#define g6 Notation::BuildPosition('g', 6)
#define g7 Notation::BuildPosition('g', 7)
#define g8 Notation::BuildPosition('g', 8)

#define h1 Notation::BuildPosition('h', 1)
#define h2 Notation::BuildPosition('h', 2)
#define h3 Notation::BuildPosition('h', 3)
#define h4 Notation::BuildPosition('h', 4)
#define h5 Notation::BuildPosition('h', 5)
#define h6 Notation::BuildPosition('h', 6)
#define h7 Notation::BuildPosition('h', 7)
#define h8 Notation::BuildPosition('h', 8)

}  // namespace ElephantTest