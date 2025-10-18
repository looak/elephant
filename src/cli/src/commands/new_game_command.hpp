#pragma once

#include <command_logic/command_registry.hpp>
#include <commands/command_api.hpp>
#include <core/game_context.hpp>
#include <printer/printer.hpp>

class NewGameCommand : public Command<bool, bool, true> {
public:
    static constexpr std::string_view description() { return "Resets chessboard into default starting position."; }
    static constexpr int priority() { return 1; }
    static constexpr std::string_view name() { return "new"; }

    // Parses the arguments from a vector of strings.
    bool parse(const std::vector<std::string>& args) override { return true; }

    // Executes the command with the given arguments.
    bool execute(const bool& args) override
    {
        if (args == false)
            prnt::err << "Invalid arguments to NewGameCommand";

        m_context->Reset();
        m_context->NewGame();
        return true;
    }

    // Outputs help information for the command.
    void help() override { prnt::out << prnt::inject_line_divider(NewGameCommand::name(), NewGameCommand::description()); }

};  // class NewGameCommand

// Register the command in the command registry.
REG_COMMAND(NewGameCommand::name(), NewGameCommand);