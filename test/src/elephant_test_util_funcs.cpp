#include "elephant_test_utils.h"
#include "chessboard.h"
#include <array>
#include <sstream>
#include "log.h"

namespace ElephantTest
{

bool PrintBoard(const Chessboard& board)
{
    auto boardItr = board.begin();
    std::array<std::stringstream, 8> ranks;
        
    byte prevRank = -1;
    do 
    {
        if (prevRank != boardItr.rank())
        {
            ranks[boardItr.rank()] << (int)(boardItr.rank() + 1) << "  ";
        }
        
        ranks[boardItr.rank()] << '[' << (*boardItr).readPiece().toString() << ']';
        prevRank = boardItr.rank();
        ++boardItr;

    } while (boardItr != board.end());

    auto rankItr = ranks.rbegin();
    while (rankItr != ranks.rend())
    {        
        LOG_INFO() << (*rankItr).str().c_str();
        rankItr++;
    }

    LOG_INFO() << "    A  B  C  D  E  F  G  H";

    return true;
}

}; // namespace ElephantTest