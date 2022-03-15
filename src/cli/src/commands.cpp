#include "commands.h"
#include "commands_print.h"
#include "commands_utils.h"
#include "game_context.h"
#include "fen_parser.h"

#include <vector>

namespace CliCommands
{

bool FenCommand(std::list<std::string>& tokens, GameContext& context)
{
    // rebuild string - maybe we should change how this works
    std::string fen("");
    for (auto&& str : tokens)
        fen += str + " ";

    bool ret = FENParser::deserialize(fen.c_str(), context);
    if (!ret)
        std::cout << " > Invalid FEN: " << fen;

    return ret;
}

void FenHelpCommand(const std::string& command)
{
    std::string helpText("Writes given FEN to current Game Context.");
    std::cout << AddLineDivider(command, helpText);
}

void HelpHelpCommand(const std::string& command)
{
    std::ostringstream ssCommand;
    ssCommand << command << ":<command> or help";

    std::string helpText("Outputs this help message or more detailed message about command");

    std::cout << AddLineDivider(ssCommand.str(), helpText);
}

bool HelpCommand(std::list<std::string>& tokens, GameContext& context)
{
    if (tokens.empty() == false)
    {
        std::string token = tokens.front();
        if (options.find(token) != options.end())
        {
            options.at(token).second(token);
            std::cout << std::endl;
        }
        else
        {
            std::string invalidInput = token.length() > 0 ? token : "Not a Value!";
            std::cout << " > Invalid command: " << invalidInput << ", help for all commands!" << std::endl;
        }
    }
    else
    {
        std::cout << " > Elephant Gambit CLI Commands:" << std::endl;
        for (CommandsMap::iterator iter = options.begin(); iter != options.end(); ++iter)
        {
            iter->second.second(iter->first);
            std::cout << std::endl;
        }
    }

    return true;
}

bool PrintCommand(std::list<std::string>& tokens, GameContext& context)
{  
    if (tokens.size() == 0)
    {
        CliPrintCommands::options.at("board").first(context.readChessboard(), "");
    }
    else if (CliPrintCommands::options.find(tokens.front()) == CliPrintCommands::options.end())
    {
        std::string invalidInput = tokens.size() > 0 ? tokens.front() : "Not a Value!";
        std::cout << " > Invalid command: " << invalidInput << ", help for all commands!" << std::endl;
    }
    else
    {
        CliPrintCommands::options.at(tokens.front()).first(context.readChessboard(), tokens.back());
    }

    return true;
}

void PrintHelpCommand(const std::string& command)
{    
    std::ostringstream ssCommand;
    ssCommand << command << ":<command> or print";
    std::string helpText("Default prints board or Prints a command.");
    std::cout << AddLineDivider(ssCommand.str(), helpText);
}

bool ExitCommand(std::list<std::string>&, GameContext&)
{
    std::exit(0);
    return true;
}

void ExitHelpCommand(const std::string& command)
{
    std::string helpText("Shutsdown Cli & Engine");
    std::cout << AddLineDivider(command, helpText);
}

} // CliCommands
