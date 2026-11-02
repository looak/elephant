#include <uci/uci_loop.hpp>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <list>
#include <sstream>
#include <string>

#include <diagnostics/logger.hpp>
#include <uci/uci.hpp>
#include <uci/uci_commands.hpp>

namespace {
std::list<std::string> tokenize(const std::string& line)
{
    std::list<std::string> tokens;
    std::istringstream stream(line);
    std::string token;
    while (std::getline(stream, token, ' '))
        tokens.push_back(token);
    return tokens;
}
} // namespace

void runUciLoop()
{
    UCI interface;
    for (const auto& option : UCICommands::options)
        std::cout << "option name " << option.first << " " << option.second << "\n";
    interface.Enable();

    std::string line;
    while (interface.Enabled() && std::getline(std::cin, line)) {
        std::list<std::string> tokens = tokenize(line);
        if (tokens.empty())
            continue;

        const std::string commandName = tokens.front();
        const auto command = UCICommands::commands.find(commandName);
        if (command == UCICommands::commands.end())
            continue;

        if (commandName == "quit" || commandName == "exit")
            std::exit(0);

        tokens.pop_front();
        try {
            command->second(tokens, interface);
        }
        catch (const std::exception& e) {
            // a malformed command must never take the engine down, report it and keep reading.
            LOG_ERROR("UCI command '{}' failed: {}", commandName, e.what());
            std::cout << "info string error " << e.what() << std::endl;
        }
    }
}
