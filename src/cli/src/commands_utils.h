#pragma once
#include <iostream>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <list>

static const std::string whitespace = " ............................... ";
static const size_t lineLength = 28;
[[maybe_unused]] static std::string AddLineDivider(std::string command, std::string helpText)
{
    size_t lengthLeft = lineLength - command.length();
    if (lengthLeft == 0 || lengthLeft > 128) // might wrap
        throw new std::range_error("AddLineDevider is too short!");

    std::stringstream output;
    output << " # " << command << ' ';
    while (lengthLeft > 0)
    {
        output << '.';
        --lengthLeft;
    }
    output << " " << helpText;
    return output.str();
}

[[maybe_unused]] static void extractArgsFromCommand(const std::string& buffer, std::list<std::string>& tokens)
{
	std::istringstream ssargs(buffer);
	std::string token;
	while (std::getline(ssargs, token, ' '))
	{
		tokens.push_back(token);
	}
}