// ElephantGambit.cpp : Defines the entry point for the application.
//

#include "elephant_cli.h"
#include "game_context.h"
#include "commands.h"
#include "log.h"

#include <iostream>
#include <sstream>
#include <string>
#include <list>

Application::Application()
{
	MESSAGE() << " Elephant Chess Engine 2021-2023";
	MESSAGE() << " Version: " << "0\n";
}

void extractArgsFromCommand(const std::string& buffer, std::list<std::string>& tokens)
{
	std::istringstream ssargs(buffer);
	std::string token;
	while (std::getline(ssargs, token, ' '))
	{
		tokens.push_back(token);
	}
}

void Application::Run()
{
	GameContext context;

	while (1)
	{
		std::cout << "\n\n > ";
		std::string buffer = "";
		std::getline(std::cin, buffer);
		std::list<std::string> tokens;
		extractArgsFromCommand(buffer, tokens);

		auto&& command = CliCommands::options.find(tokens.front());
		if(tokens.size() > 0 && command != CliCommands::options.end())
		{
			auto token = tokens.front();
			tokens.pop_front();
			
			command->second.first(tokens, context);
		}
		else if (tokens.size() == 1)
		{
			// attempt to make a move with token
			auto moveCommand = CliCommands::options.find("move");
			moveCommand->second.first(tokens, context);
		}
		else
		{
			std::string invalidInput = tokens.size() > 0 ? tokens.front() : "Not a Value!";
			std::cout << " Invalid command: " << invalidInput << ", help for all commands!" <<std::endl;
		}
	}
}
