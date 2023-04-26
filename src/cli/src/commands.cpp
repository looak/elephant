#include "chessboard.h"
#include "commands.h"
#include "commands_print.h"
#include "commands_uci.h"
#include "commands_utils.h"
#include "evaluator.h"
#include "fen_parser.h"
#include "game_context.h"
#include "move.h"
#include "move_generator.h"
#include <vector>

namespace CliCommands
{

bool FenCommand(std::list<std::string>& tokens, GameContext& context)
{
    // rebuild string - maybe we should change how this works
    std::string fen("");
    for (auto&& str : tokens)
        fen += str + " ";

    // if fen is empty we want to serialize current context.
    if (fen.empty())
    {
        std::string output;
		bool result = FENParser::serialize(context, output);
		std::cout << " " << (result ? output : "Serializing failed!") << "\n";
        return result;
    }

    context.Reset();
    bool ret = FENParser::deserialize(fen.c_str(), context);
    if (!ret)
        std::cout << " Invalid FEN: " << fen << "\n";

    return ret;
}

void FenHelpCommand(const std::string& command)
{
    std::ostringstream ssCommand;
    ssCommand << "fen <string> or fen";

    std::string helpText("Sets the board to the given FEN string or outputs the FEN string for current board.");
    std::cout << AddLineDivider(ssCommand.str(), helpText);
}

void HelpHelpCommand(const std::string& command)
{
    std::ostringstream ssCommand;
    ssCommand << command << ":<command> or help";

    std::string helpText("Outputs this help message or more detailed message about command");

    std::cout << AddLineDivider(ssCommand.str(), helpText);
}

bool HelpCommand(std::list<std::string>& tokens, GameContext& context)
{
    if (tokens.empty() == false)
    {
        std::string token = tokens.front();
        if (options.find(token) != options.end())
        {
            options.at(token).second(token);
            std::cout << std::endl;
        }
        else
        {
            std::string invalidInput = token.length() > 0 ? token : "Not a Value!";
            std::cout << " Invalid command: " << invalidInput << ", help for all commands!" << std::endl;
        }
    }
    else
    {
        std::cout << " Elephant Gambit CLI Commands:" << std::endl;
        for (CommandsMap::iterator iter = options.begin(); iter != options.end(); ++iter)
        {
            iter->second.second(iter->first);
            std::cout << std::endl;
        }
    }

    return true;
}

bool PrintCommand(std::list<std::string>& tokens, GameContext& context)
{  
    if (tokens.size() == 0)
    {
        CliPrintCommands::options.at("board").first(context, "");
    }
    else if (CliPrintCommands::options.find(tokens.front()) == CliPrintCommands::options.end())
    {
        std::string invalidInput = tokens.size() > 0 ? tokens.front() : "Not a Value!";
        std::cout << " Invalid command: " << invalidInput << ", help for all commands!" << std::endl;
    }
    else
    {
        CliPrintCommands::options.at(tokens.front()).first(context, tokens.back());
    }

    return true;
}

void PrintHelpCommand(const std::string& command)
{    
    std::ostringstream ssCommand;
    ssCommand << command << ":<command> or print";
    std::string helpText("Default prints board or Prints a command.");
    std::cout << AddLineDivider(ssCommand.str(), helpText);
}

bool ExitCommand(std::list<std::string>&, GameContext&)
{
    std::exit(0);
    return true;
}

void ExitHelpCommand(const std::string& command)
{
    std::string helpText("Shutsdown Cli & Engine");
    std::cout << AddLineDivider(command, helpText);
}

bool DivideDepthCommand(std::list<std::string>& tokens, GameContext& context)
{
    if (tokens.empty() == false)
    {
        std::string token = tokens.front();
        int depth = std::stoi(token);
        
        // validate depth
		if (depth < 1 || depth > 10)
		{
			std::cout << " Invalid depth: " << depth << ", must be between 1 and 10!" << std::endl;
			return false;
		}

		MoveGenerator generator;
        
        auto moves = generator.GeneratePossibleMoves(context);
        int total = 0;
		for (auto&& move : moves)
		{
            std::cout << " " << move.SourceSquare.toString() << move.TargetSquare.toString() << ": ";
            context.MakeMove(move);
            int result = generator.Perft(context, depth -1);
            total += result;
			std::cout << result << std::endl;
            context.UnmakeMove(move);
		}
        // total += moves.size();
        
        std::cout << "\n Moves: " << moves.size() << "\n";
        std::cout << " Total: " << total << "\n";
    }
    else
    {
        DivideDepthHelpCommand("divide");
    }

    return true;
}

void DivideDepthHelpCommand(const std::string& command)
{
    std::ostringstream ssCommand;
    ssCommand << command << " <depth>";
	std::string helpText("Divide given board by depth.");
    std::cout << AddLineDivider(ssCommand.str(), helpText);
}

bool MoveCommand(std::list<std::string>& tokens, GameContext& context)
{
	if (tokens.empty() == false)
	{
		std::string token = tokens.front();
		Move move = context.readChessboard().DeserializeMoveFromPGN(token, context.readToPlay() == Set::WHITE);
		
        if (move.isInvalid())
        {
            std::cout << " Invalid move: " << token << std::endl;
            return false;
        }

		if (!context.PlayMove(move))
		{
			std::cout << " Invalid move: " << token << std::endl;
            return false;
		}
	}

	return true;
}

void MoveHelpCommand(const std::string& command)
{
    std::ostringstream ssCommand;
    ssCommand << command << " <move>";
    std::string helpText("Makes a move on the current board.");
    std::cout << AddLineDivider(ssCommand.str(), helpText);

}

bool EvaluateCommand(std::list<std::string>& tokens, GameContext& context)
{
    Evaluator evaluator;
    u64 value = 0;// evaluator.Evaluate(context.readChessboard());
    std::cout << " Evaluation: " << value << std::endl;
    return true;
}

void EvaluateHelpCommand(const std::string& command)
{

}

bool EvaluateBestMoveCommand(std::list<std::string>& tokens, GameContext& context)
{
    Move mv = context.CalculateBestMove();
    std::cout << " Best Move: " << mv.toString() << std::endl;
    return true;
}

void EvaluateBestMoveHelpCommand(const std::string& command)
{
    std::ostringstream ssCommand;
    ssCommand << "bestmove";
    std::string helpText("Returns engines suggestion for best move.");
    std::cout << AddLineDivider(ssCommand.str(), helpText);
}

bool UCIEnableCommand(std::list<std::string>& tokens, GameContext& context)
{
    std::cout << "UCI mode enabled\n";    
    UCICommands::UCIEnable();
    std::cout << "Normal mode enabled\n";
    return true;
}

void UCIEnableHelpCommand(const std::string& command)
{
    std::ostringstream ssCommand;
    ssCommand << command;
    std::string helpText("Puts engine into UCI mode.");
    std::cout << AddLineDivider(ssCommand.str(), helpText);
}

bool NewGameCommand(std::list<std::string>& tokens, GameContext& context)
{
    context.NewGame();
    return true;
}

void NewGameHelpCommand(const std::string&)
{
    std::ostringstream ssCommand;
    ssCommand << "newgame";
    std::string helpText("Starts a new game.");
    std::cout << AddLineDivider(ssCommand.str(), helpText);
}

bool AvailableMovesCommand(std::list<std::string>& tokens, GameContext& context)
{
    std::cout << " Available Moves: \n";
    MoveGenerator generator;
    auto moves = generator.GeneratePossibleMoves(context);
    for (auto&& move : moves)
    {
        std::cout << context.readChessboard().SerializeMoveToPGN(move) << " ";
    }
    std::cout << std::endl;
    return true;
}

void AvailableMovesHelpCommand(const std::string& command)
{
    std::ostringstream ssCommand;
    ssCommand << "show";
    std::string helpText("Prints all available moves.");
    std::cout << AddLineDivider(ssCommand.str(), helpText);
}


} // CliCommands
