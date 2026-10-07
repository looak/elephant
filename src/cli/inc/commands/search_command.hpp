#pragma once

#include "commands/command_api.hpp"
#include "commands/logic/command_registry.hpp"

#include <search/search.hpp>
#include <system/time_manager.hpp>

struct SearchCommandArgs
{
    u8 depth = 8;
    u32 moveTimeMs = 0;
};

class SearchCommand : public Command<SearchCommandArgs, true> {
public:
    static constexpr std::string_view description() { return "Search the current position and return the best move."; }
    static constexpr int priority() { return 50; }
    static constexpr std::string_view name() { return "search"; }

    std::optional<SearchCommandArgs> parse(const std::vector<std::string>& args) override
    {
        SearchCommandArgs parsedArgs{};

        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i] == "--depth" && i + 1 < args.size()) {
                try {
                    const int depth = std::stoi(args[i + 1]);
                    if (depth <= 0 || depth > 255) {
                        prnt::err << " Error: '--depth' must be in range [1, 255].";
                        return std::nullopt;
                    }
                    parsedArgs.depth = static_cast<u8>(depth);
                }
                catch (const std::exception&) {
                    prnt::err << " Error: Invalid value for '--depth': " << args[i + 1];
                    return std::nullopt;
                }
                ++i;
                continue;
            }

            if (args[i] == "--time" && i + 1 < args.size()) {
                try {
                    const int moveTimeMs = std::stoi(args[i + 1]);
                    if (moveTimeMs <= 0) {
                        prnt::err << " Error: '--time' must be greater than 0.";
                        return std::nullopt;
                    }
                    parsedArgs.moveTimeMs = static_cast<u32>(moveTimeMs);
                }
                catch (const std::exception&) {
                    prnt::err << " Error: Invalid value for '--time': " << args[i + 1];
                    return std::nullopt;
                }
                ++i;
                continue;
            }

            prnt::err << " Error: Unknown argument '" << args[i] << "'.";
            return std::nullopt;
        }

        return parsedArgs;
    }

    bool execute(const SearchCommandArgs& args) override
    {
        const auto material = m_context->readChessPosition().material();
        if (material.whiteKing().count() == 0 || material.blackKing().count() == 0) {
            prnt::err << " Error: Position is not initialized. Use 'new' or 'fen' before searching.";
            return false;
        }

        SearchParameters params;
        params.SearchDepth = args.depth;
        params.MoveTime = args.moveTimeMs;

        TimeManager timeManager(params, m_context->readToPlay());
        timeManager.applyTimeSettings(params, m_context->readToPlay());

        Search search(*m_context);
        SearchResult result;

        if (m_context->readToPlay() == Set::WHITE) {
            result = search.go<Set::WHITE>(params, timeManager);
        }
        else {
            result = search.go<Set::BLACK>(params, timeManager);
        }

        if (result.move().isNull()) {
            prnt::err << " Error: Search completed without a legal move.";
            return false;
        }

        prnt::out << " Best move: " << result.move().toString() << std::endl;
        prnt::out << " Score: " << result.score << " cp" << std::endl;
        prnt::out << " Nodes: " << result.count << std::endl;
        if (result.pvLine.length > 0) {
            prnt::out << " PV: " << result.pvLine.toString() << std::endl;
        }
        return true;
    }

    void help(bool extended) override
    {
        if (extended) {
            prnt::out << "\nUsage: " << SearchCommand::name() << " [--depth <plies>] [--time <ms>]" << std::endl << std::endl;
            prnt::out << "Search the current position and return the best move." << std::endl;
            prnt::out << "Options:" << std::endl;
            prnt::out << "  --depth <plies>  Optional. Search depth in plies (default: 8)." << std::endl;
            prnt::out << "  --time <ms>      Optional. Time limit in milliseconds.";
            return;
        }
        prnt::out << prnt::inject_line_divider(SearchCommand::name(), SearchCommand::description());
    }
};

REG_COMMAND(SearchCommand::name(), SearchCommand);
