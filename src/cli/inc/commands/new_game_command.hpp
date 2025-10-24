#pragma once

#include "commands/logic/command_registry.hpp"
#include "commands/command_api.hpp"

#include <core/game_context.hpp>
#include <printer/printer.hpp>

class NewGameCommand : public CommandNoArgs<true> {
public:
    static constexpr std::string_view description() { return "Resets chessboard into default starting position."; }
    static constexpr int priority() { return 1; }
    static constexpr std::string_view name() { return "new"; }

    // Executes the command with the given arguments.
    bool execute() override
    {
        m_context->Reset();
        m_context->NewGame();
        return true;
    }

    // Outputs help information for the command.
    void help(bool extended) override 
    { 
        if (extended) {
            prnt::out << "\nUsage: " << NewGameCommand::name() << "\n";
            prnt::out << "Resets the chessboard to the standard starting position for a new game.";
            prnt::out << "This command clears the current game state and prepares the engine for a fresh game.";
            prnt::out << "No additional arguments are required or accepted.";
            return;
        }
        prnt::out << prnt::inject_line_divider(NewGameCommand::name(), NewGameCommand::description()); 
    }

};  // class NewGameCommand

// Register the command in the command registry.
REG_COMMAND(NewGameCommand::name(), NewGameCommand);