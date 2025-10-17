#pragma once
#include <iostream>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <list>


[[maybe_unused]] static void extractArgsFromCommand(const std::string& buffer, std::list<std::string>& tokens)
{
	std::istringstream ssargs(buffer);
	std::string token;
	while (std::getline(ssargs, token, ' '))
	{
		tokens.push_back(token);
	}
}