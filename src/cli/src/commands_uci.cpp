#include "commands_uci.h"
#include "game_context.h"
#include "commands_utils.h"

/**
* Send back what options this engine supports. */
void UCIOptions()
{
	for (auto&& option : UCI::options)
	{
		std::cout << "option name " << option.first << " " << option.second.second << "\n";
	}
}

void UCI::UCIEnable(GameContext& context)
{
	UCIOptions();
	bool uciEnabled = true;
	while (uciEnabled)
	{
		std::cout << "uciok\n";

		std::string buffer = "";
		std::getline(std::cin, buffer);
		std::list<std::string> tokens;
		extractArgsFromCommand(buffer, tokens);

		LOG_INFO() << "From GUI: " << buffer;

		if (tokens.size() == 0)
			continue;

		auto&& command = UCI::commands.find(tokens.front());
		if (tokens.size() > 0 && command != UCI::commands.end())
		{
			auto token = tokens.front();
			tokens.pop_front();

			command->second(tokens, context);
			if (token == "quit")
				uciEnabled = false;
		}
	}
}

void UCI::DebugCommand(std::list<std::string>& args, GameContext& context)
{

}
void UCI::IsReadyCommand(std::list<std::string>&, GameContext&)
{
	LOG_INFO() << "readyok";
	std::cout << "readyok\n";
}

void UCI::SetOptionCommand(std::list<std::string>& args, GameContext& context)
{

}
void UCI::RegisterCommand(std::list<std::string>& args, GameContext& context)
{

}
void UCI::NewGameCommand(std::list<std::string>& args, GameContext& context)
{
	context.NewGame();
}
void UCI::PositionCommand(std::list<std::string>& args, GameContext& context)
{
	
}
void UCI::GoCommand(std::list<std::string>& args, GameContext& context)
{

}
void UCI::StopCommand(std::list<std::string>& args, GameContext& context)
{

}
void UCI::PonderHitCommand(std::list<std::string>& args, GameContext& context)
{

}
void UCI::QuitCommand(std::list<std::string>& args, GameContext& context)
{
	std::cout << "bye bye\n";
}

void UCI::DebugOutputOption(std::list<std::string>& args, GameContext& context)
{
	for (auto&& arg : args)
	{
		LOG_INFO() << "DebugOutputOption: " << arg;
	}	
}
