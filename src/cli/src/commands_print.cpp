#include "commands_print.h"
#include <io/printer.hpp>
#include <array>
#include <iostream>
#include <sstream>
#include <core/chessboard.hpp>
#include "commands_utils.h"
#include <core/game_context.hpp>

namespace CliPrintCommands {
bool
Board(const GameContext& context, const std::string&)
{
    std::stringstream output;
    printer::board(output, context.readChessboard());
    std::cout << output.str();
    return true;
}

void
BoardHelp(int, const std::string& command)
{
    std::string helpText("Print the current board (default)");
    std::cout << AddLineDivider(command, helpText);
}

bool
HelpCommand(const GameContext&, const std::string&)
{
    std::cout << " Elephant Gambit CLI print Commands:" << std::endl;
    for (PrintCommandsMap::iterator iter = options.begin(); iter != options.end(); ++iter) {
        options.at(iter->first).second(0, iter->first);
        std::cout << std::endl;
    }

    return true;
}

void
HelpCommandHelp(int, const std::string& command)
{
    std::string helpText("Outputs this help message");
    std::cout << AddLineDivider(command, helpText);
}

bool
PGNCommand(const GameContext&, const std::string&)
{
    // const auto& movehistory = context.readMoveHistory();
    // std::ostringstream outputPgn;

    // for (auto&& entry : movehistory) {
    //     bool odd = entry.PlyCount & 1;
    //     if (odd)
    //         outputPgn << entry.MoveCount << ". ";

    //     outputPgn << entry.SAN;
    //     if (odd)
    //         outputPgn << " ";
    //     else
    //         outputPgn << "\n";
    // }

    // std::cout << outputPgn.str() << std::endl;

    return true;
}
void
PGNHelpCommand(int, const std::string& command)
{
    std::string helpText("Print the current game in PGN format");
    std::cout << AddLineDivider(command, helpText);
}

}  // namespace CliPrintCommands