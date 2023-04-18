#pragma once

#include <map>
#include <string>
#include <functional>

class GameContext;

namespace CliPrintCommands
{

typedef std::function<bool(const GameContext&, const std::string&)> CommandFunction;
typedef std::function<void(int, const std::string&)> HelpCommandFunction;
typedef std::map<std::string, std::pair<CommandFunction, HelpCommandFunction>> PrintCommandsMap;

bool Board(const GameContext& context, const std::string& input);
void BoardHelp(int option, const std::string& command);

bool HelpCommand(const GameContext& context, const std::string& input);
void HelpCommandHelp(int option, const std::string& command);

bool PGNCommand(const GameContext& context, const std::string& input);
void PGNHelpCommand(int option, const std::string& command);

static PrintCommandsMap options = {
    { "board", { Board, BoardHelp } },
    { "pgn", { PGNCommand, PGNHelpCommand } },
    { "help", { HelpCommand, HelpCommandHelp } }
};

} // namespace CliPrintCommands