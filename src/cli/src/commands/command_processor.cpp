#include "commands/command_api.hpp"
#include "commands/logic/command_processor.hpp"
#include "commands/logic/command_registry.hpp"
#include "elephant_cli.hpp"
#include "printer/printer.hpp"

#include "commands/uci_commands.hpp"

#include <iostream>
#include <thread>
#include <syncstream>

namespace {
bool looksLikeSanMove(const std::string& token)
{
    if (token.empty()) {
        return false;
    }

    if (token == "O-O" || token == "O-O-O" || token == "0-0" || token == "0-0-0") {
        return true;
    }

    if (token.length() < 2 || token.length() > 7) {
        return false;
    }

    const std::string_view allowed = "KQRBNabcdefgh12345678x=+#-";
    if (allowed.find(token.front()) == std::string_view::npos) {
        return false;
    }

    for (const char c : token) {
        if (allowed.find(c) == std::string_view::npos) {
            return false;
        }
    }

    return true;
}
}  // namespace


bool NormalModeProcessor::processInput(AppContext* context, const std::string& line)
{
    // Use a string stream to easily split the line into words
    std::istringstream iss(line);
    std::string command_name;
    std::vector<std::string> args;
    iss >> command_name;

    if (command_name == "exit" || command_name == "quit") {
        return false;  // Signal to exit the application
    }

    // Special command to switch modes
    if (command_name == "uci") {        
        context->setState(std::make_unique<UciModeProcessor>());
        return true;
    }

    // Find and execute the command.
    bool fallbackToMove = false;
    auto command = CommandRegistry::instance().createCommand(command_name);

    // Handle unrecognized commands gracefully
    if (command == nullptr && looksLikeSanMove(command_name)) {
        command = CommandRegistry::instance().createCommand("move");        
        args.push_back(command_name);  // treat the command as a SAN move argument
        fallbackToMove = true;
    }

    if (command == nullptr) {
        prnt::err << "Error: Unknown command '" << command_name << "'" << std::endl;
        return true;
    }

    // Collect the arguments for the command    
    std::string arg;
    while (iss >> arg) {
        args.push_back(arg);
    }

    // The command is created, used, and then destroyed here.
    command->setContext(&m_gameContext);
    if (command->run(args) > 0 && fallbackToMove) {
        prnt::err << "Error: Unknown command '" << command_name << "'" << std::endl;
    }

    return true;
}

UciModeProcessor::UciModeProcessor() 
{
    // system("cls");
}

void UciModeProcessor::options()
{    
    for (auto&& option : UCICommands::options) {
        std::cout << "option name " << option.first << " " << option.second << "\n";
    } 
}

void UciModeProcessor::tokenize(const std::string& buffer, std::list<std::string>& tokens)
{
    std::istringstream ssargs(buffer);
    std::string token;
    while (std::getline(ssargs, token, ' ')) {
        tokens.push_back(token);
    }
}

bool UciModeProcessor::processInput(AppContext* context, const std::string&) {
    UCI interface;
    options();
    interface.Enable();
    
    while (interface.Enabled()) {
        std::string buffer = "";
        std::getline(std::cin, buffer);
        std::list<std::string> tokens;
        tokenize(buffer, tokens);        

        if (tokens.size() == 0)
            continue;

        std::string commandStr = tokens.front();
        auto&& command = UCICommands::commands.find(commandStr);
    
        if (tokens.size() > 0 && command != UCICommands::commands.end()) {
            auto token = tokens.front();
    
            if (token == "quit" || token == "exit")
                std::exit(0);
    
            if (token == "normal") {
                ASSERT_MSG(context != nullptr, "AppContext is null in UciModeProcessor::processInput");
                context->setState(std::make_unique<NormalModeProcessor>());
                return true;
            }       
            
            tokens.pop_front();  // remove command from arguments
            command->second(tokens, interface);
        }
    }

    return true;
}