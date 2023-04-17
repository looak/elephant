// ElephantGambit.cpp : Defines the entry point for the application.
//
#include "commands.h"
#include "commands_utils.h"
#include "elephant_cli.h"
#include "elephant_cli_config.h"
#include "elephant_gambit_config.h"
#include "game_context.h"
#include "log.h"

#include <iostream>
#include <sstream>
#include <string>
#include <list>

Application::Application()
{
	MESSAGE() << " Elephant Gambit Open Source Chess Engine 2021-2023"; // EGOSCE
	MESSAGE() << " Versions: \tCLI: " << ELEPHANT_CLI_VERSION_STR << "\tEngine: " << ELEPHANT_GAMBIT_VERSION_STR;	
	MESSAGE() << " Source: https://github.com/looak/elephant";
	MESSAGE() << " Author: Alexander Loodin Ek\n";
}

void Application::Run()
{
	GameContext context;
		
	while (1)
	{
		std::cout << " > ";
		std::string buffer = "";
		std::getline(std::cin, buffer);
		std::list<std::string> tokens;
		extractArgsFromCommand(buffer, tokens);

		if (tokens.size() == 0)
			continue;

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
