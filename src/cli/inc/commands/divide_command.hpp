#pragma once

#include "commands/command_api.hpp"
#include "commands/logic/command_registry.hpp"
#include <core/game_context.hpp>
#include <search/perft_search.hpp>


struct DivideCommandArgs
{
    int depth = 0;
};

class DivideCommand : public Command<DivideCommandArgs, true> {
public:
	static constexpr std::string_view description() { return "Divides the current position to a given depth."; }
	static constexpr int priority() { return 50; }
	static constexpr std::string_view name() { return "divide"; }

    std::optional<DivideCommandArgs> parse(const std::vector<std::string>& args) override
    {
        DivideCommandArgs parsedArgs{};

        if (args.size() != 1) {
            spdlog::error("'divide' command requires a depth argument.");
            return std::nullopt;
        }
        try {
            parsedArgs.depth = std::stoi(args[0]);
        }
        catch (const std::exception&) {
            spdlog::error("Invalid depth argument '{}'. Must be an integer.", args[0]);
            return std::nullopt;
        }
        return parsedArgs;
    }

    bool execute(const DivideCommandArgs& args) override
    {        
        prnt::out << " Divide command executed with depth: " << args.depth << std::endl;
        PerftSearch perftSearch(*m_context);
        auto results = std::vector<DivideResult>{}; //perftSearch.Divide(m_context->readToPlay(), args.depth);

        std::sort(results.begin(), results.end(), [](const DivideResult& a, const DivideResult& b) {
            return a.Move.source() < b.Move.source();
        });

        u64 totalNodes = 0;
        u64 accumNodes = 0;
        for (const auto& result : results) {
            u32 prettyCount = result.Result.Nodes;
            if (result.Result.Nodes == 0)
                prettyCount = 1;

            prnt::out << " " << result.Move.toString() << ": " << prettyCount << std::endl;
            totalNodes += prettyCount;
            accumNodes += result.Result.AccNodes;
        }
        prnt::out << " Nodes: " << totalNodes << std::endl;
        prnt::out << " Total nodes: " << accumNodes << std::endl;
        return true;
    }

    void help(bool extended) override
    {    
        if (extended) {
            prnt::out << "\nUsage: " << DivideCommand::name() << " <depth>" << std::endl << std::endl;
            prnt::out << "Divides the current position to the given depth and outputs the result for each move." << std::endl;
            prnt::out << "Maximum depth is limited to single digit, i.e. 9. Tool is most useful around 3-5 ply." << std::endl;            
            return;
        }
        prnt::out << prnt::inject_line_divider(DivideCommand::name(), DivideCommand::description()) << std::endl;
    }
};  // class DivideCommand

// Register the command in the command registry.
REG_COMMAND(DivideCommand::name(), DivideCommand);