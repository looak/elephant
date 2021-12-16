// ElephantGambit.cpp : Defines the entry point for the application.
//

#include "elephant_cli.h"
#include <iostream>
#include <string>

using namespace std;

int main()
{
	std::cout << " Elephant Chess Engine 2021" << std::endl
		<< " Version: " << 0;

	while (1)
	{
		std::cout << std::endl << " > ";
		std::string buffer = "";
		std::getline(std::cin, buffer);

		std::cout << buffer;
	}

	return 0;
}
