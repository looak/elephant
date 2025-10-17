#include "commands_uci.h"
#include "commands_utils.h"
#include <move/move.hpp>
#include <core/uci.hpp>

/**
* Send back what options this engine supports. */
void UCIOptions()
{
    for (auto&& option : UCICommands::options)
    {
    	std::cout << "option name " << option.first << " " << option.second << "\n";
    }
}

void UCICommands::UCIEnable()
{
    UCI interface;
    UCIOptions();
    interface.Enable();    
    while (interface.Enabled())
    {
        std::string buffer = "";
        std::getline(std::cin, buffer);
        std::list<std::string> tokens;
        extractArgsFromCommand(buffer, tokens);

        LOG_INFO() << "From GUI: " << buffer;

        if (tokens.size() == 0)
            continue;

        std::string commandStr = tokens.front();
        auto&& command = UCICommands::commands.find(commandStr);
        if (tokens.size() > 0 && command != UCICommands::commands.end())
        {
            auto token = tokens.front();
            if (token == "quit")
                std::exit(0);

            tokens.pop_front(); // remove command from arguments
            if (!command->second(tokens, interface))
            {
                LOG_ERROR() << " Something went wrong during command: " << commandStr;
                std::exit(1);
            }
        }
    }
}

bool UCICommands::DebugCommand(std::list<std::string>&, UCI&)
{
    LOG_ERROR() << "Not implemented";
    return false;
}

bool UCICommands::IsReadyCommand(std::list<std::string>&, UCI& interface)
{
    return interface.IsReady();
}

bool UCICommands::SetOptionCommand(std::list<std::string>& args, UCI& interface)
{
    interface.SetOption(args);
    return true;
}

bool UCICommands::RegisterCommand(std::list<std::string>&, UCI&)
{
    LOG_ERROR() << "Not implemented";
    return false;
}

bool UCICommands::NewGameCommand(std::list<std::string>&, UCI& interface)
{
    return interface.NewGame();
}

bool UCICommands::PositionCommand(std::list<std::string>& args, UCI& interface)
{
    if (!interface.Position(args))
    {
        LOG_ERROR() << " Something went wrong during position command";
        return false;
    }

    return true;
}
bool UCICommands::GoCommand(std::list<std::string>& args, UCI& interface)
{
    return interface.Go(args);
}
bool UCICommands::StopCommand(std::list<std::string>&, UCI& interface)
{
    return interface.Stop();
}
bool UCICommands::PonderHitCommand(std::list<std::string>&, UCI&)
{
    LOG_ERROR() << "Not implemented";
    return false;
}
bool UCICommands::QuitCommand(std::list<std::string>&, UCI&)
{
    std::cout << "bye bye\n";
    return true;
}