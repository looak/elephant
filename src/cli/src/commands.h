#pragma once

#include <functional>
#include <map>
#include <string>
#include <list>

class GameContext;

namespace CliCommands
{

typedef std::function<bool(std::list<std::string>&, GameContext&)> CommandFunction;
typedef std::function<void(const std::string&)> HelpCommandFunction;
typedef std::map<std::string, std::pair<CommandFunction, HelpCommandFunction>> CommandsMap;

bool FenCommand(std::list<std::string>& tokens, GameContext& context);
void FenHelpCommand(const std::string& command);

bool HelpCommand(std::list<std::string>& tokens, GameContext& context);
void HelpHelpCommand(const std::string& command);

bool ExitCommand(std::list<std::string>& tokens, GameContext& context);
void ExitHelpCommand(const std::string& command);

bool PrintCommand(std::list<std::string>& tokens, GameContext& context);
void PrintHelpCommand(const std::string& command);

//static CommandsMap aliases = {
//    {"h", { HelpCommand, HelpHelpCommand } },
//    //{"m", { MoveCommand, MoveHelpCommand } },
//    {"x", { ExitCommand, ExitHelpCommand } },
//};

static CommandsMap options = {
    { "fen", { FenCommand, FenHelpCommand } },
    // {"clear", { ClearCommand, ClearHelpCommand } },
    { "help", { HelpCommand, HelpHelpCommand } },
    { "print", { PrintCommand, PrintHelpCommand } },
    // {"move", { MoveCommand, MoveHelpCommand } },
    // {"divide", {DivideDepthCommand, DivideDepthCommandHelp } },
    // {"show", { AvailableMovesCommand, AvailableMovesHelpCommand } },
    { "exit", { ExitCommand, ExitHelpCommand } }
    // {"about", { AboutCommand, AboutHelpCommand } }

};

}; // namespace CliCommands