#include "commands/command_api.hpp"
#include "commands/logic/command_processor.hpp"
#include "commands/logic/command_registry.hpp"
#include "elephant_cli.hpp"
#include "printer/printer.hpp"

#include <iostream>


bool NormalModeProcessor::processInput(AppContext& context, const std::string& line)
{
    // Use a string stream to easily split the line into words
    std::istringstream iss(line);
    std::string command_name;
    iss >> command_name;

    if (command_name == "exit" || command_name == "quit") {
        return false;  // Signal to exit the application
    }

    // Special command to switch modes
    if (command_name == "uci") {        
        context.setState(std::make_unique<UciModeProcessor>());
        return true;
    }

    // Find and execute the command.
    auto command = CommandRegistry::instance().createCommand(command_name);

    // Handle unrecognized commands gracefully
    if (command == nullptr) {
        prnt::out << "Error: Unknown command '" << command_name << "'";
        return true;
    }

    // Collect the arguments for the command
    std::vector<std::string> args;
    std::string arg;
    while (iss >> arg) {
        args.push_back(arg);
    }

    // The command is created, used, and then destroyed here.
    command->setContext(&m_gameContext);
    command->run(args);

    return true;
}

UciModeProcessor::UciModeProcessor()
{
    system("cls");  // Clear console on Windows)
    prnt::out << "UCI mode enabled. Type 'exit' or 'quit' to leave UCI mode.";
}

bool UciModeProcessor::processInput(AppContext& context, const std::string& line)
{
    std::istringstream iss(line);
    std::string command_name;
    iss >> command_name;

    if (command_name == "exit" || command_name == "quit") {
        return false;  // Signal to exit the application
    }

    return true;
}
