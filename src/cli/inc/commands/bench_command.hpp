#pragma once
#include "commands/command_api.hpp"
#include "commands/logic/command_registry.hpp"

#include <core/game_context.hpp>
#include <io/fen_parser.hpp>
#include <search/search.hpp>
#include <util/bench_positions.hpp>
#include <util/clock.hpp>

class BenchCommand : public CommandNoArgs<true> {
public:
	static constexpr std::string_view description() { return "Runs bench on predefined positions."; }
	static constexpr int priority() { return 50; }
	static constexpr std::string_view name() { return "bench"; }

	// Executes the command with the given arguments.
	bool execute() override
	{		
        LOG_INFO() << "Starting benchmark...";

        Clock timer;
        timer.Start();
        u64 nodes = 0;

        for (const auto& fen : bench::fens) {
            GameContext context;
            io::fen_parser::deserialize(fen.c_str(), context.editChessboard());

            Search search(context);
            SearchParameters params;
            TimeManager tm(params, context.readToPlay());
            
            params.SearchDepth = bench::depth;
            if (context.readToPlay() == Set::WHITE) {
                auto result = search.go<Set::WHITE>(params, tm);
                nodes += result.count;
            }
            else {
                auto result = search.go<Set::BLACK>(params, tm);
                nodes += result.count;
            }
        }

        timer.Stop();
        float elapsedSeconds = timer.getElapsedTime() / 1000.0f;

        std::cout << "info string " << elapsedSeconds << " seconds\n";
        std::cout << nodes << " nodes " << timer.calcNodesPerSecond(nodes) << " nps\n";
        return true;
	}

	// Outputs help information for the command.
	void help(bool) override
	{
		prnt::out << prnt::inject_line_divider(BenchCommand::name(), BenchCommand::description());    
	}

};  // class BenchCommand

// Register the command in the command registry.
REG_COMMAND(BenchCommand::name(), BenchCommand);