#pragma once

#include "commands/logic/command_registry.hpp"
#include "commands/command_api.hpp"
#include "elephant_cli_config.h"
#include "elephant_gambit_config.h"
#include "printer/printer.hpp"


class AboutCommand: public CommandNoArgs {
public:
    static constexpr std::string_view description() { return "Outputs info about this chess engine."; }
    static constexpr int priority() { return 100; }
    static constexpr std::string_view name() { return "about"; }

    // Executes the command with the given arguments.
    bool execute() override
    {
        prnt::out << " Elephant Gambit Open Source Chess Engine 2021-2025";
        prnt::out << " versions:\n   cli:    " << ELEPHANT_CLI_VERSION_STR << "-" << ELEPHANT_CLI_VERSION_PRERELEASE
                    << ELEPHANT_CLI_VERSION_SUFFIX << "\n   engine: " << ELEPHANT_GAMBIT_VERSION_STR << "-"
                    << ELEPHANT_GAMBIT_VERSION_PRERELEASE << ELEPHANT_GAMBIT_VERSION_SUFFIX;
        prnt::out << " Source: https://github.com/looak/elephant";
        prnt::out << " Author: Alexander Loodin Ek";
        prnt::out << " Contact: alexander.loodin.ek(at)gmail.com";
        return true;
    }

    // Outputs help information for the command.
    void help() override { prnt::out << prnt::inject_line_divider(AboutCommand::name(), AboutCommand::description()); }

};  // class AboutCommand

// Register the command in the command registry.
REG_COMMAND(AboutCommand::name(), AboutCommand);

class ExitCommand : public CommandNoArgs {
    public:
    static constexpr std::string_view description() { return "Shuts down the CLI & Engine."; }
    static constexpr int priority() { return 200; }
    static constexpr std::string_view name() { return "exit"; }
    // Executes the command with the given arguments.
    bool execute() override
    {
        prnt::out << "Exiting Elephant Gambit CLI...";
        std::exit(0);
        return true;
    }
    // Outputs help information for the command.
    void help() override { prnt::out << prnt::inject_line_divider(ExitCommand::name(), ExitCommand::description()); }

};  // class ExitCommand

// Register the command in the command registry.
REG_COMMAND(ExitCommand::name(), ExitCommand);