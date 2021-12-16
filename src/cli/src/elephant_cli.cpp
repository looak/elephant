// ElephantGambit.cpp : Defines the entry point for the application.
//

#include "elephant_cli.h"

#include <iostream>
#include <string>

using namespace std;


Application::Application()
{
	std::cout << " Elephant Chess Engine 2021" << std::endl
		<< " Version: " << 0;
}

void Application::Run()
{
	while (1)
	{
		std::cout << std::endl << " > ";
		std::string buffer = "";
		std::getline(std::cin, buffer);

		std::cout << buffer;
	}
}
