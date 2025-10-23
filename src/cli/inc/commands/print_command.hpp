#pragma once

#include "commands/command_api.hpp"
#include "commands/logic/command_registry.hpp"

struct PrintCommandArgs
{
    bool pretty;
    bool flipped;
    bool pgn;
};

class PrintCommand : public ReadOnlyCommand<PrintCommandArgs> {
public:
    static constexpr std::string_view description() { return "Prints the current game state."; }
    static constexpr int priority() { return 200; }
    static constexpr std::string_view name() { return "print"; }    

    bool execute(const PrintCommandArgs& args) override
    {
        // Implementation of the print command
        return true;
    }

    PrintCommandArgs parse(const std::vector<std::string>& args) override
    {
        PrintCommandArgs parsedArgs;
        // Parse arguments and populate parsedArgs
        return parsedArgs;
    }

    void help() override
    {
        prnt::out << prnt::inject_line_divider(PrintCommand::name(), PrintCommand::description());

    }
};

REG_COMMAND(PrintCommand::name(), PrintCommand);
