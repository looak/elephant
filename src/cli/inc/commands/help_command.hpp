#pragma once

#include "commands/logic/command_registry.hpp"
#include "commands/command_api.hpp"

struct HelpCommandArgs
{
    std::string commandName;
    std::unique_ptr<ICommandBase> commandPtr;
};

class HelpCommand : public Command<HelpCommandArgs> {
public:
    static constexpr std::string_view description() { return "Displays list of commands. optioanl arg <cmd>"; }
    static constexpr int priority() { return 100; }
    static constexpr std::string_view name() { return "help"; }

    // Executes the command with the given arguments.
    std::optional<HelpCommandArgs> parse(const std::vector<std::string>& args) override
    {
        HelpCommandArgs parsedArgs;
        if (!args.empty()) {
            parsedArgs.commandName = args[0];
            parsedArgs.commandPtr = CommandRegistry::instance().createCommand(parsedArgs.commandName);
            
            if (parsedArgs.commandPtr == nullptr) {
                prnt::err << " Error: Unknown command '" << parsedArgs.commandName << "'";
                return std::nullopt;
            }

        }
        return parsedArgs;
    }

    bool execute(const HelpCommandArgs& args) override
    {
        //prnt::out << "Help information for: " << args << std::endl;
        if (args.commandPtr == nullptr) {
            prnt::out << " Elephant Gambit CLI Commands:";
            for (const auto& cmdName : CommandRegistry::instance().getCommands()) {
                auto command = cmdName.factory();
                if (command) {
                    command->help();
                }
            }
        }
        else {
            auto command = CommandRegistry::instance().createCommand(args.commandName);
            if (command) {
                command->help(true);
            }
            else {
                prnt::out << " Error: Unknown command '" << args.commandName << "'";
            }
        }

        return true;
    }

    // Outputs help information for the command.
    void help(bool extended) override 
    {
        if (extended) {
            prnt::out << "\nUsage: " << HelpCommand::name() << " [<command>]" << std::endl << std::endl;
            prnt::out << "Displays a list of all available commands or detailed help for a specific command.";
            prnt::out << "If no command is specified, a summary of all commands is shown.";
            prnt::out << "Options:";
            prnt::out << "  <command>   The name of the command to get detailed help for.";
            return;
        }
        prnt::out << prnt::inject_line_divider(HelpCommand::name(), HelpCommand::description()); 
    }
};

// Register the command in the command registry.
REG_COMMAND(HelpCommand::name(), HelpCommand);