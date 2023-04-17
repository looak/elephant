#include "commands_uci.h"
#include "game_context.h"
#include "commands_utils.h"

/**
* Send back what options this engine supports. */
void UCIOptions()
{
	//std::cout << "options\n";
}

void UCI::UCIEnable(GameContext& context)
{
	bool uciEnabled = true;
	while (uciEnabled)
	{
		UCIOptions();
		std::cout << "uciok\n";

		std::string buffer = "";
		std::getline(std::cin, buffer);
		std::list<std::string> tokens;
		extractArgsFromCommand(buffer, tokens);

		if (tokens.size() == 0)
			continue;

		auto&& command = UCI::options.find(tokens.front());
		if (tokens.size() > 0 && command != UCI::options.end())
		{
			auto token = tokens.front();
			tokens.pop_front();

			command->second(tokens, context);
			if (token == "quit")
				return;
		}
	}
}

void UCI::DebugCommand(std::list<std::string>& args, GameContext& context)
{

}
void UCI::IsReadyCommand(std::list<std::string>& args, GameContext& context)
{

}
void UCI::SetOptionCommand(std::list<std::string>& args, GameContext& context)
{

}
void UCI::RegisterCommand(std::list<std::string>& args, GameContext& context)
{

}
void UCI::NewGameCommand(std::list<std::string>& args, GameContext& context)
{

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
