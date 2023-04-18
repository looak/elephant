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

bool DivideDepthCommand(std::list<std::string>& tokens, GameContext& context);
void DivideDepthHelpCommand(const std::string& command);

bool MoveCommand(std::list<std::string>& tokens, GameContext& context);
void MoveHelpCommand(const std::string& command);

bool EvaluateCommand(std::list<std::string>& tokens, GameContext& context);
void EvaluateHelpCommand(const std::string& command);

bool EvaluateBestMoveCommand(std::list<std::string>& tokens, GameContext& context);
void EvaluateBestMoveHelpCommand(const std::string& command);

bool UCIEnableCommand(std::list<std::string>& tokens, GameContext& context);
void UCIEnableHelpCommand(const std::string& command);

bool NewGameCommand(std::list<std::string>& tokens, GameContext& context);
void NewGameHelpCommand(const std::string& command);

bool AvailableMovesCommand(std::list<std::string>& tokens, GameContext& context);
void AvailableMovesHelpCommand(const std::string& command);

//static CommandsMap aliases = {
//    {"h", { HelpCommand, HelpHelpCommand } },
//    //{"m", { MoveCommand, MoveHelpCommand } },
//    {"x", { ExitCommand, ExitHelpCommand } },
//};

static CommandsMap options = {
    { "fen", { FenCommand, FenHelpCommand } },
//    { "evaluate", { EvaluateCommand, EvaluateHelpCommand } },
    { "bestmove", { EvaluateBestMoveCommand, EvaluateBestMoveHelpCommand } }, // "bestmove
    { "uci", { UCIEnableCommand, UCIEnableHelpCommand } },
    { "help", { HelpCommand, HelpHelpCommand } },
    { "print", { PrintCommand, PrintHelpCommand } },
    { "move", { MoveCommand, MoveHelpCommand } },
    { "divide", {DivideDepthCommand, DivideDepthHelpCommand } },
    { "newgame", { NewGameCommand, NewGameHelpCommand } },
    { "exit", { ExitCommand, ExitHelpCommand } },
    { "show", { AvailableMovesCommand, AvailableMovesHelpCommand } }
    // {"clear", { ClearCommand, ClearHelpCommand } },
    // {"about", { AboutCommand, AboutHelpCommand } }
};

}; // namespace CliCommands