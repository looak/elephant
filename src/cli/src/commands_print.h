#pragma once

#include <map>
#include <string>
#include <functional>

class Chessboard;

namespace CliPrintCommands
{

typedef std::function<bool(const Chessboard&, const std::string&)> CommandFunction;
typedef std::function<void(int, const std::string&)> HelpCommandFunction;
typedef std::map<std::string, std::pair<CommandFunction, HelpCommandFunction>> PrintCommandsMap;

bool Board(const Chessboard& board, const std::string& input);
void BoardHelp(int option, const std::string& command);

bool HelpCommand(const Chessboard& board, const std::string& input);
void HelpCommandHelp(int option, const std::string& command);

static PrintCommandsMap options = {
    {"board", { Board, BoardHelp } },
    {"help", { HelpCommand, HelpCommandHelp } }
};

} // namespace CliPrintCommands