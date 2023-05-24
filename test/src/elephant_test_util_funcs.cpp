#include <algorithm>
#include <array>
#include <sstream>
#include "bitboard.h"
#include "chessboard.h"
#include "elephant_test_utils.h"
#include "game_context.h"
#include "log.h"
#include "move.h"

namespace ElephantTest {
//
// bool PrintBoard(const GameContext& context)
//{
//    const Chessboard& board = context.readChessboard();
//    auto boardItr = board.begin();
//    std::array<std::stringstream, 8> ranks;
//
//    byte prevRank = -1;
//    do
//    {
//        if (prevRank != boardItr.rank())
//        {
//            ranks[boardItr.rank()] << "\n > " << (int)(boardItr.rank() + 1) << "  ";
//        }
//
//        ranks[boardItr.rank()] << '[' << (*boardItr).readPiece().toString() << ']';
//        prevRank = boardItr.rank();
//        ++boardItr;
//
//    } while (boardItr != board.end());
//
//    auto rankItr = ranks.rbegin();
//    while (rankItr != ranks.rend())
//    {
//        LOG_INFO() << (*rankItr).str();
//        rankItr++;
//    }
//
//    LOG_INFO() << "\n >\n >     A  B  C  D  E  F  G  H\n";
//    LOG_INFO() << " > move: " << std::dec << (int)context.readMoveCount() << "\tply: " << (int)context.readPly() << "\n";
//    LOG_INFO() << " > hash: 0x" << std::hex << board.readHash() << "\n";
//    LOG_INFO() << " > castling state: " << PrintCastlingState(board) << "\n";
//    LOG_INFO() << " > prev move: " << Notation::toString(move.SourceSquare) << Notation::toString(move.TargetSquare) << "\n";
//
//    return true;
//}

MoveCount
CountMoves(const std::vector<Move>& moves, MoveCount::Predicate predicate)
{
    MoveCount result;

    for (auto&& mv : moves) {
        if (!predicate(mv))
            continue;

        if ((mv.Flags & MoveFlag::Capture) == MoveFlag::Capture)
            result.Captures++;
        if ((mv.Flags & MoveFlag::Promotion) == MoveFlag::Promotion)
            result.Promotions++;
        if ((mv.Flags & MoveFlag::EnPassant) == MoveFlag::EnPassant)
            result.EnPassants++;
        if ((mv.Flags & MoveFlag::Castle) == MoveFlag::Castle)
            result.Castles++;
        if ((mv.Flags & MoveFlag::Check) == MoveFlag::Check)
            result.Checks++;
        if ((mv.Flags & MoveFlag::Checkmate) == MoveFlag::Checkmate) {
            result.Checks++;
            result.Checkmates++;
        }

        result.Moves++;
    }

    return result;
}

u64
CombineKingMask(KingMask mask)
{
    u64 result = 0;
    for (int i = 0; i < 8; ++i) {
        result |= mask.threats[i];
    }
    result |= mask.knightsAndPawns;
    return result;
}

bool
PrintBoard(const Chessboard& board)
{
    auto boardItr = board.begin();
    std::array<std::stringstream, 8> ranks;

    byte prevRank = -1;
    do {
        if (prevRank != boardItr.rank()) {
            ranks[boardItr.rank()] << (int)(boardItr.rank() + 1) << "  ";
        }

        ranks[boardItr.rank()] << '[' << (*boardItr).readPiece().toString() << ']';
        prevRank = boardItr.rank();
        ++boardItr;

    } while (boardItr != board.end());

    auto rankItr = ranks.rbegin();
    while (rankItr != ranks.rend()) {
        LOG_INFO() << (*rankItr).str().c_str();
        rankItr++;
    }

    LOG_INFO() << "    A  B  C  D  E  F  G  H";

    return true;
}

void
SetupDefaultStartingPosition(Chessboard& board)
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
    board.PlacePiece(B, c1);
    board.PlacePiece(Q, d1);
    board.PlacePiece(K, e1);
    board.PlacePiece(B, f1);
    board.PlacePiece(N, g1);
    board.PlacePiece(R, h1);

    board.PlacePiece(P, a2);
    board.PlacePiece(P, b2);
    board.PlacePiece(P, c2);
    board.PlacePiece(P, d2);
    board.PlacePiece(P, e2);
    board.PlacePiece(P, f2);
    board.PlacePiece(P, g2);
    board.PlacePiece(P, h2);

    board.PlacePiece(r, a8);
    board.PlacePiece(n, b8);
    board.PlacePiece(b, c8);
    board.PlacePiece(q, d8);
    board.PlacePiece(k, e8);
    board.PlacePiece(b, f8);
    board.PlacePiece(n, g8);
    board.PlacePiece(r, h8);

    board.PlacePiece(p, a7);
    board.PlacePiece(p, b7);
    board.PlacePiece(p, c7);
    board.PlacePiece(p, d7);
    board.PlacePiece(p, e7);
    board.PlacePiece(p, f7);
    board.PlacePiece(p, g7);
    board.PlacePiece(p, h7);

    board.setCastlingState(15);
}

bool
NotationCompare(Notation lhs, Notation rhs)
{
    return lhs.index() < rhs.index();
}

bool
VerifyListsContainSameNotations(std::vector<Notation> listOne, std::vector<Notation> listTwo)
{
    std::sort(listOne.begin(), listOne.end(), NotationCompare);
    std::sort(listTwo.begin(), listTwo.end(), NotationCompare);

    if (listOne.size() != listTwo.size()) {
        return false;
    }

    for (size_t i = 0; i < listOne.size(); ++i) {
        if (listOne[i] != listTwo[i]) {
            return false;
        }
    }

    return true;
}

}  // namespace ElephantTest