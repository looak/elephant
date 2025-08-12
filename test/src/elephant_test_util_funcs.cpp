#include <algorithm>
#include <array>
#include <sstream>
#include "chessboard.h"
#include "elephant_test_utils.h"
#include "game_context.h"
#include "log.h"
#include <move/move.hpp>
#include <position/position.hpp>

namespace ElephantTest {

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

void PrintPosition(PositionReader position) {
    auto posItr = position.begin();
    std::array<std::stringstream, 8> ranks;

    byte prevRank = -1;
    do {
        if (prevRank != posItr.rank()) {
            ranks[posItr.rank()] << (int)(posItr.rank() + 1) << "  ";
        }

        ranks[posItr.rank()] << '[' << posItr.get().toString() << ']';
        prevRank = posItr.rank();
        ++posItr;

    } while (posItr != position.end());

    auto rankItr = ranks.rbegin();
    while (rankItr != ranks.rend()) {
        LOG_INFO() << (*rankItr).str().c_str();
        rankItr++;
    }

    LOG_INFO() << "    A  B  C  D  E  F  G  H";
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