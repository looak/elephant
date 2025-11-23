#pragma once
#include "commands/command_api.hpp"
#include "commands/logic/command_registry.hpp"

#include <core/game_context.hpp>
#include <io/fen_parser.hpp>
#include <search/search.hpp>
#include <util/bench_positions.hpp>
#include <util/clock.hpp>

struct EpdTestCase {
    std::string id;
    std::string fen;
    std::string bestMoveSan;
};

struct BenchArgs {
    std::string epdFilePath;
    u8 depth = 12;
    u8 threads = 1;
    u64 timePerTest = 5000; // milliseconds

    std::vector<EpdTestCase> testCases;
};

class BenchCommand : public Command<BenchArgs, true> {
public:
	static constexpr std::string_view description() { return "Runs bench on predefined positions."; }
	static constexpr int priority() { return 50; }
	static constexpr std::string_view name() { return "bench"; }

    // Parses command-line arguments into BenchArgs.
    std::optional<BenchArgs> parse(const std::vector<std::string>& args) override {
        BenchArgs parsedArgs;
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i] == "--epd" && i + 1 < args.size()) {
                parsedArgs.epdFilePath = args[i + 1];
                ++i; // Skip next argument as it's the value
            }
            else if (args[i] == "--depth" && i + 1 < args.size()) {
                parsedArgs.depth = static_cast<u8>(std::stoi(args[i + 1]));
                ++i;
            }
            else if (args[i] == "--threads" && i + 1 < args.size()) {
                parsedArgs.threads = static_cast<u8>(std::stoi(args[i + 1]));
                ++i;
            }
            else if (args[i] == "--time" && i + 1 < args.size()) {
                parsedArgs.timePerTest = static_cast<u8>(std::stoi(args[i + 1]));
                ++i;
            }
            else {
                // Unknown argument
                prnt::err << " Unknown argument: " << args[i] << std::endl;
                return std::nullopt;
            }
        }

        if (!parsedArgs.epdFilePath.empty()) {
            parsedArgs.testCases = loadEpdFile(parsedArgs.epdFilePath);
            if (parsedArgs.testCases.empty()) {
                prnt::err << " No valid test cases found in EPD file: " << parsedArgs.epdFilePath << std::endl;
                return std::nullopt;
            }
        }

        return parsedArgs;
    }

	// Executes the command with the given arguments.
	bool execute(const BenchArgs& args) override {		
        spdlog::info("Starting benchmark...");

        Clock timer;
        timer.Start();
        u64 nodes = 0;

        if (args.testCases.size() > 0) {
        // this bench is evaluating if the engine can solve some tricky positions
            for (const auto& testCase : args.testCases) {
                GameContext context;
                io::fen_parser::deserialize(testCase.fen.c_str(), context.editChessboard());

                Search search(context);
                SearchParameters params;
                TimeManager tm(params, context.readToPlay());
                
                params.SearchDepth = args.depth;
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
        else {
        // this bench is all about nps.
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
    }
        return true;
	}

	// Outputs help information for the command.
	void help(bool) override
	{
		prnt::out << prnt::inject_line_divider(BenchCommand::name(), BenchCommand::description());    
	}

private:
    std::vector<EpdTestCase> loadEpdFile(const std::string& filePath) {
    std::vector<EpdTestCase> cases;
    std::ifstream file(filePath);
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        EpdTestCase tc;
        size_t bmPos = line.find(" bm ");
        size_t idPos = line.find(" id ");

        if (bmPos == std::string::npos || idPos == std::string::npos) {
            continue; // Not a valid test line
        }

        tc.fen = line.substr(0, bmPos);
        
        size_t bmEndPos = line.find(';', bmPos);
        tc.bestMoveSan = line.substr(bmPos + 4, bmEndPos - (bmPos + 4));

        size_t idEndPos = line.find(';', idPos);
        tc.id = line.substr(idPos + 4, idEndPos - (idPos + 4));

        cases.push_back(tc);
    }
    return cases;
}

};  // class BenchCommand

// Register the command in the command registry.
REG_COMMAND(BenchCommand::name(), BenchCommand);