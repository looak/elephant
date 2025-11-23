#include "commands/command_api.hpp"
#include "commands/logic/command_processor.hpp"
#include "commands/logic/command_registry.hpp"
#include "elephant_cli.hpp"
#include "printer/printer.hpp"

#include "commands/uci_commands.hpp"

#include <iostream>
#include <thread>
#include <syncstream>


bool NormalModeProcessor::processInput(AppContext& context, const std::string& line)
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
        context.setState(std::make_unique<UciModeProcessor>());
        return true;
    }

    // Find and execute the command.
    auto command = CommandRegistry::instance().createCommand(command_name);

    // Handle unrecognized commands gracefully
    if (command == nullptr) {
        command = CommandRegistry::instance().createCommand("move");        
        args.push_back(command_name);  // treat the command as a SAN move argument
    }

    // Collect the arguments for the command    
    std::string arg;
    while (iss >> arg) {
        args.push_back(arg);
    }

    // The command is created, used, and then destroyed here.
    command->setContext(&m_gameContext);
    if (command->run(args) > 0) {
        prnt::err << "Error: Unknown command '" << command_name << "'";
    }

    return true;
}

UciModeProcessor::UciModeProcessor() 
{
    system("cls");
}

void UciModeProcessor::options()
{    
    for (auto&& option : UCICommands::options) {
        std::cout << "option name " << option.first << " " << option.second << "\n";
    } 
}

void UciModeProcessor::extractArgsFromCommand(const std::string& buffer, std::list<std::string>& tokens)
{
    std::istringstream ssargs(buffer);
    std::string token;
    while (std::getline(ssargs, token, ' ')) {
        tokens.push_back(token);
    }
}

bool UciModeProcessor::processInput(AppContext& context, const std::string& line)
{
    UCI interface;
    options();
    interface.Enable();
    UCIThreadContext uciWorker(interface, 0);

    std::jthread uciWorkerThread([&uciWorker](std::stop_token stopToken) {        
        uciWorker.process(stopToken);
    });
    
    while (interface.Enabled()) {
        std::string buffer = "";
        std::getline(std::cin, buffer);
        std::list<std::string> tokens;
        extractArgsFromCommand(buffer, tokens);        

        if (tokens.size() == 0)
            continue;

        std::string commandStr = tokens.front();
        auto&& command = UCICommands::commands.find(commandStr);
        if (tokens.size() > 0 && command != UCICommands::commands.end()) {
            auto token = tokens.front();
            if (token == "quit" || token == "exit")
                std::exit(0);
            if (token == "normal") {
                context.setState(std::make_unique<NormalModeProcessor>());
                return true;
            }       
            std::osyncstream syncOut(std::cout);
            syncOut << "queuing command: " << commandStr << "\n";
            tokens.pop_front();  // remove command from arguments
            uciWorker.queue(tokens, command->second);
            
                
        }
    }

    uciWorkerThread.request_stop();
    return true;
}

void UciModeProcessor::independentMode()
{
    UCI interface;
    options();
    interface.Enable();
    UCIThreadContext uciThread(interface, 0);

    std::jthread uciWorker([&uciThread](std::stop_token stopToken) {        
        uciThread.process(stopToken);
    });
    

    while (interface.Enabled()) {
        std::string buffer = "";
        std::getline(std::cin, buffer);
        std::list<std::string> tokens;
        extractArgsFromCommand(buffer, tokens);

        spdlog::info("From GUI: {}", buffer);

        if (tokens.size() == 0)
            continue;

        std::string commandStr = tokens.front();
        auto&& command = UCICommands::commands.find(commandStr);
        if (tokens.size() > 0 && command != UCICommands::commands.end()) {
            auto token = tokens.front();
            if (token == "quit" || token == "exit")
                std::exit(0);  

            tokens.pop_front();  // remove command from arguments
            uciThread.queue(tokens, command->second);
        }
    }
}
