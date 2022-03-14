#pragma once

#include <map>
#include <string>
#include <functional>

#include "chessboard.h"

namespace PrintCommands
{

typedef std::function<void(const Chessboard&, const std::string)> CommandFunction;
typedef std::function<void(int, const std::string)> HelpCommandFunction;
typedef std::map<std::string, std::pair<CommandFunction, HelpCommandFunction>> PrintCommandsMap;

void Board(const Chessboard& board, const std::string input);
void BoardHelp(int option, const std::string command);

void HelpCommand(const Chessboard& board, const std::string input);
void HelpCommandHelp(int option, const std::string command);

static PrintCommandsMap CommandList = {
    {"board", { Board, BoardHelp } },
    {"help", { HelpCommand, HelpCommandHelp } }
};

} // namespace PrintCommands