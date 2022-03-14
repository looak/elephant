// ElephantGambit.cpp : Defines the entry point for the application.
//

#include "elephant_cli.h"
#include "game_context.h"
#include "commands_print.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

Application::Application()
{
	std::cout << " Elephant Chess Engine 2021" << std::endl
		<< " Version: " << 0;
}

void Application::Run()
{
	GameContext context;

	while (1)
	{
		std::cout << std::endl << " > ";
		std::string buffer = "";
		std::getline(std::cin, buffer);
		std::istringstream ssargs(buffer);
		std::vector<std::string> tokens;
		std::string token;
		while (std::getline(ssargs, token, ':'))
		{
			tokens.push_back(token);
		}

		auto&& command = PrintCommands::CommandList.find(tokens.front());
		if(tokens.size() > 0 && command != PrintCommands::CommandList.end())
		{
			command->second.first(context.readChessboard(), "");
			// else
			// 	printOptions.at(tokens.front()).first(tokens.back(), state);
		}
		else
		{
			std::string invalidInput = tokens.size() > 0 ? tokens.front() : "Not a Value!";
			std::cout << " > Invalid command: " << invalidInput << ", help for all commands!" <<std::endl;
		}

		std::cout << buffer;
	}
}
