#pragma once

#include "commands/command_api.hpp"
#include "commands/logic/command_registry.hpp"

#include <search/perft_search.hpp>
#include <system/clock.hpp>
#include <algorithm>

struct PerftCommandArgs
{
    int depth = 0;
};

class PerftCommand : public Command<PerftCommandArgs, true> {
public:
    static constexpr std::string_view description() { return "Run perft on the current position to a given depth."; }
    static constexpr int priority() { return 50; }
    static constexpr std::string_view name() { return "perft"; }

    std::optional<PerftCommandArgs> parse(const std::vector<std::string>& args) override
    {
        if (args.size() != 1) {
            prnt::err << " Error: 'perft' command requires a depth argument.";
            return std::nullopt;
        }

        PerftCommandArgs parsedArgs{};
        try {
            parsedArgs.depth = std::stoi(args[0]);
        }
        catch (const std::exception&) {
            prnt::err << " Error: Invalid depth argument '" << args[0] << "'. Must be an integer.";
            return std::nullopt;
        }

        if (parsedArgs.depth <= 0) {
            prnt::err << " Error: Depth must be greater than 0.";
            return std::nullopt;
        }

        return parsedArgs;
    }

    bool execute(const PerftCommandArgs& args) override
    {
        const auto material = m_context->readChessPosition().material();
        if (material.whiteKing().count() == 0 || material.blackKing().count() == 0) {
            prnt::err << " Error: Position is not initialized. Use 'new' or 'fen' before running perft.";
            return false;
        }

        Clock timer;
        timer.Start();

        PerftSearch perftSearch(*m_context);
        auto result = perftSearch.Run(args.depth);

        timer.Stop();
        const u64 elapsedMs = static_cast<u64>(std::max<i64>(timer.getElapsedTime(), 1));
        const u64 nps = result.Nodes * 1000ULL / elapsedMs;

        prnt::out << " Perft depth " << args.depth << std::endl;
        prnt::out << " Nodes: " << result.Nodes << std::endl;
        prnt::out << " Captures: " << result.Captures << std::endl;
        prnt::out << " Castles: " << result.Castles << std::endl;
        prnt::out << " En passant: " << result.EnPassants << std::endl;
        prnt::out << " Promotions: " << result.Promotions << std::endl;
        prnt::out << " Time: " << elapsedMs << " ms" << std::endl;
        prnt::out << " NPS: " << nps << std::endl;
        return true;
    }

    void help(bool extended) override
    {
        if (extended) {
            prnt::out << "\nUsage: " << PerftCommand::name() << " <depth>" << std::endl << std::endl;
            prnt::out << "Run perft on the current position to the given depth and print aggregate counters." << std::endl;
            prnt::out << "Inputs:" << std::endl;
            prnt::out << "  <depth>  Required. Positive integer search depth.";
            return;
        }
        prnt::out << prnt::inject_line_divider(PerftCommand::name(), PerftCommand::description());
    }
};

REG_COMMAND(PerftCommand::name(), PerftCommand);
