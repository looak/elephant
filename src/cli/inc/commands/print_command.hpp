#pragma once

#include "commands/command_api.hpp"
#include "commands/logic/command_registry.hpp"
#include "printer/printer.hpp"

#include <io/printer.hpp>

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

    bool execute(const PrintCommandArgs&) override
    {        
        io::printer::position(prnt::out, readPosition());
        return true;
    }

    std::optional<PrintCommandArgs> parse(const std::vector<std::string>& args) override
    {
        PrintCommandArgs parsedArgs{};
        for (const auto& arg : args) {
            if (arg == "--pretty") {
                parsedArgs.pretty = true;
            }
            else if (arg == "--flipped") {
                parsedArgs.flipped = true;
            }
            else if (arg == "--pgn") {
                parsedArgs.pgn = true;
            }
            else {
                prnt::err << "Error: Unknown argument '" << arg << "'";
                return std::nullopt;
            }
        }
        return parsedArgs;
    }

    void help(bool extended) override
    {
        if (extended) {
            prnt::out << "\nUsage: " << PrintCommand::name() << " [--pretty] [--flipped] [--pgn]" << std::endl << std::endl;
            prnt::out << "Prints the current game state to the console.";
            prnt::out << "Options:";
            prnt::out << "  --pretty    Print the board in a human-friendly format.";
            prnt::out << "  --flipped   Print the board from Black's perspective.";
            prnt::out << "  --pgn       Print the game in PGN format.";
            return;
        }
        prnt::out << prnt::inject_line_divider(PrintCommand::name(), PrintCommand::description());

    }
};

REG_COMMAND(PrintCommand::name(), PrintCommand);
