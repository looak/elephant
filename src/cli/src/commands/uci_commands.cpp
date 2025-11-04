#include "commands/uci_commands.hpp"
#include <move/move.hpp>
#include <core/uci.hpp>

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