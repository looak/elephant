#include "commands/uci_commands.hpp"

#include <io/printer.hpp>
#include <move/move.hpp>
#include <core/uci.hpp>

bool UCICommands::DebugCommand(std::list<std::string> args, UCI& interface)
{
    if (args.empty() == false)
        interface.Debug(args.front() == "on");
    return true;
}

bool UCICommands::IsReadyCommand(std::list<std::string>, UCI& interface)
{
    return interface.IsReady();
}

bool UCICommands::SetOptionCommand(std::list<std::string> args, UCI& interface)
{
    interface.SetOption(args);
    return true;
}

bool UCICommands::RegisterCommand(std::list<std::string>, UCI&)
{
    // the engine needs no registration, the command is accepted & ignored.
    return true;
}

bool UCICommands::NewGameCommand(std::list<std::string>, UCI& interface) {
    return interface.NewGame();
}

bool UCICommands::PositionCommand(std::list<std::string> args, UCI& interface) {
    // a malformed position is reported by UCI::Position itself.
    return interface.Position(args);
}
bool UCICommands::GoCommand(std::list<std::string> args, UCI& interface) {
    interface.AsyncGo(args);
    return true;
}
bool UCICommands::BenchCommand(std::list<std::string> args, UCI& interface) {
    return interface.Bench(args);
}
bool UCICommands::BuildInfoCommand(std::list<std::string>, UCI& interface) {
    return interface.BuildInfo();
}
bool UCICommands::StopCommand(std::list<std::string>, UCI& interface) {
    return interface.Stop();
}
bool UCICommands::PonderHitCommand(std::list<std::string>, UCI& interface) {
    return interface.PonderHit();
}

bool UCICommands::PrintCommand(std::list<std::string>, UCI& interface)
{
    io::printer::board(std::cout, interface.readGameContext().readChessboard());
    return true;
}

bool UCICommands::QuitCommand(std::list<std::string>, UCI&) {
    std::cout << "bye bye";
    return true;
}