#include "commands_print.h"
#include "commands_utils.h"
#include <iostream>
#include "chessboard.h"

namespace CliPrintCommands
{
bool Board(const Chessboard& board, const std::string& input)
{
    auto boardItr = board.begin();
        
    byte prevRank = -1;
    do 
    {
        if (prevRank != boardItr.rank())
        {
            std::cout << "\n > " << (int)(boardItr.rank() + 1) << "  ";
        }
        
        std::cout << '[' << (*boardItr).getPiece().toString() << ']';
        prevRank = boardItr.rank();
        ++boardItr;

    } while (boardItr != board.end());
    std::cout << "\n >\n >     A  B  C  D  E  F  G  H\n";

    return true;
}

void BoardHelp(int option, const std::string& command)
{

}

bool HelpCommand(const Chessboard&, const std::string& input)
{
    std::cout << " > Elephant Gambit CLI print Commands:" << std::endl;
    for (PrintCommandsMap::iterator iter = options.begin(); iter != options.end(); ++iter)
    {
        options.at(iter->first).second(0, iter->first);
        std::cout << std::endl;
    }

    return true;
}

void HelpCommandHelp(int, const std::string& command)
{
    std::string helpText("Outputs this help message");
    std::cout << AddLineDivider(command, helpText);
}
} // namespace CliPrintCommands