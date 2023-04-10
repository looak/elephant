#include "commands_print.h"
#include "commands_utils.h"
#include "game_context.h"
#include "chessboard.h"
#include <iostream>
#include <sstream>
#include <array>

namespace CliPrintCommands
{
bool Board(const GameContext& context, const std::string& input)
{
	const auto& board = context.readChessboard();
    auto boardItr = board.begin();
    std::array<std::stringstream, 8> ranks;
        
    byte prevRank = -1;
    do 
    {
        if (prevRank != boardItr.rank())
        {
            ranks[boardItr.rank()] << "\n " << (int)(boardItr.rank() + 1) << "  ";
        }
        
        ranks[boardItr.rank()] << '[' << (*boardItr).readPiece().toString() << ']';
        prevRank = boardItr.rank();
        ++boardItr;

    } while (boardItr != board.end());

    auto rankItr = ranks.rbegin();
    while (rankItr != ranks.rend())
    {        
        std::cout << (*rankItr).str();
        rankItr++;
    }

    auto ep = board.readEnPassant();
    std::cout << "\n\n     A  B  C  D  E  F  G  H\n";
    std::cout << " move: " << context.readMoveCount() << "\tturn: " << (context.readToPlay() == Set::WHITE ? "White\n" : "Black\n");
    std::cout << " castling: " << board.readCastlingStateInfo().toString() << "\ten passant: " << (ep.isValid() ? ep.toString() : "-") << "\n\n";

    return true;
}

void BoardHelp(int option, const std::string& command)
{
    std::string helpText("Print the current board (default)");
    std::cout << AddLineDivider(command, helpText);
}

bool HelpCommand(const GameContext&, const std::string& input)
{
    std::cout << " Elephant Gambit CLI print Commands:" << std::endl;
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