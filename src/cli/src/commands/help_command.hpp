#pragma once

#include <command_logic/command_registry.hpp>
#include <commands/command_api.hpp>

class HelpCommand : public Command<std::string, void*> {
public:
    static constexpr std::string_view description() { return "Displays list of commands. optioanl arg <cmd>"; }
    static constexpr int priority() { return 100; }
    static constexpr std::string_view name() { return "help"; }

    // Executes the command with the given arguments.
    std::string parse(const std::vector<std::string>& args) override
    {
        if (args.empty()) {
            return "";
        }
        return args[0];
    }
    void* execute(const std::string& args) override
    {
        //prnt::out << "Help information for: " << args << std::endl;
        if (args.empty()) {
            prnt::out << " Elephant Gambit CLI Commands:";
            for (const auto& cmdName : CommandRegistry::instance().getCommands()) {
                auto command = cmdName.factory();
                if (command) {
                    command->help();
                }
            }
        }
        else {
            auto command = CommandRegistry::instance().createCommand(args);
            if (command) {
                command->help();
            }
            else {
                prnt::out << " Error: Unknown command '" << args << "'";
            }
        }

        return nullptr;
    }

    // Outputs help information for the command.
    void help() override { prnt::out << prnt::inject_line_divider(HelpCommand::name(), HelpCommand::description()); }
};

// Register the command in the command registry.
REG_COMMAND(HelpCommand::name(), HelpCommand);