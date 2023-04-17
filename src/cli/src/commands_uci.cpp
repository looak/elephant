#include "commands_uci.h"
#include "game_context.h"
#include "commands_utils.h"

/**
* Send back what options this engine supports. */
void UCIOptions()
{
    //std::cout << "options\n";
}

void UCIEnable(GameContext& context)
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
		if(tokens.size() > 0 && command != UCI::options.end())
		{
			auto token = tokens.front();
			tokens.pop_front();
			
			command->second(tokens, context);
		}
		else if (tokens.size() == 1)
		{
            auto token = tokens.front();
            if (token == "quit")
                return;
		}		
    }    
}

void DebugCommand(std::list<std::string>& args, GameContext& context)
{

}
void IsReadyCommand(std::list<std::string>& args, GameContext& context)
{

}
void SetOptionCommand(std::list<std::string>& args, GameContext& context)
{

}
void RegisterCommand(std::list<std::string>& args, GameContext& context)
{

}
void NewGameCommand(std::list<std::string>& args, GameContext& context)
{

}
void PositionCommand(std::list<std::string>& args, GameContext& context)
{

}
void GoCommand(std::list<std::string>& args, GameContext& context)
{

}
void StopCommand(std::list<std::string>& args, GameContext& context)
{

}
void PonderHitCommand(std::list<std::string>& args, GameContext& context)
{

}
void QuitCommand(std::list<std::string>& args, GameContext& context)
{

}
