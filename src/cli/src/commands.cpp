#include "commands.h"

#include <charconv>
#include <vector>

#include "chessboard.h"
#include "commands_print.h"
#include "commands_uci.h"
#include "commands_utils.h"
#include "elephant_cli_config.h"
#include "elephant_gambit_config.h"
#include "evaluator.h"
#include "fen_parser.h"
#include "game_context.h"
#include "move.h"
#include "move_generator.hpp"
#include "search.hpp"

namespace CliCommands {

bool
FenCommand(std::list<std::string>& tokens, GameContext& context)
{
    // rebuild string - maybe we should change how this works
    std::string fen("");
    for (auto&& str : tokens)
        fen += str + " ";

    // if fen is empty we want to serialize current context.
    if (fen.empty()) {
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

void
FenHelpCommand(const std::string&)
{
    std::ostringstream ssCommand;
    ssCommand << "fen <string> or fen";

    std::string helpText("Sets the board to the given FEN string or outputs the FEN string for current board.");
    std::cout << AddLineDivider(ssCommand.str(), helpText);
}

void
HelpHelpCommand(const std::string& command)
{
    std::ostringstream ssCommand;
    ssCommand << command << ":<command> or help";

    std::string helpText("Outputs this help message or more detailed message about command");

    std::cout << AddLineDivider(ssCommand.str(), helpText);
}

bool
HelpCommand(std::list<std::string>& tokens, GameContext&)
{
    if (tokens.empty() == false) {
        std::string token = tokens.front();
        if (options.find(token) != options.end()) {
            options.at(token).second(token);
            std::cout << std::endl;
        }
        else {
            std::string invalidInput = token.length() > 0 ? token : "Not a Value!";
            std::cout << " Invalid command: " << invalidInput << ", help for all commands!" << std::endl;
        }
    }
    else {
        std::cout << " Elephant Gambit CLI Commands:" << std::endl;
        for (OrderedCommands::iterator iter = ordered.begin(); iter != ordered.end(); ++iter) {
            auto command = options.at(*iter);
            command.second(*iter);
            std::cout << std::endl;
        }
    }

    return true;
}

bool
PrintCommand(std::list<std::string>& tokens, GameContext& context)
{
    if (tokens.size() == 0) {
        CliPrintCommands::options.at("board").first(context, "");
    }
    else if (CliPrintCommands::options.find(tokens.front()) == CliPrintCommands::options.end()) {
        std::string invalidInput = tokens.size() > 0 ? tokens.front() : "Not a Value!";
        std::cout << " Invalid command: " << invalidInput << ", help for all commands!" << std::endl;
    }
    else {
        CliPrintCommands::options.at(tokens.front()).first(context, tokens.back());
    }

    return true;
}

void
PrintHelpCommand(const std::string& command)
{
    std::ostringstream ssCommand;
    ssCommand << command << ":<command> or print";
    std::string helpText("Default prints board or Prints a command.");
    std::cout << AddLineDivider(ssCommand.str(), helpText);
}

bool
ExitCommand(std::list<std::string>&, GameContext&)
{
    std::exit(0);
    return true;
}

void
ExitHelpCommand(const std::string& command)
{
    std::string helpText("Shutsdown Cli & Engine");
    std::cout << AddLineDivider(command, helpText);
}

bool
DivideDepthCommand(std::list<std::string>& tokens, GameContext& context)
{
    if (tokens.empty() == false) {
        std::string token = tokens.front();
        int depth = std::stoi(token);

        // validate depth
        if (depth < 1 || depth > 10) {
            std::cout << " Invalid depth: " << depth << ", must be between 1 and 10!" << std::endl;
            return false;
        }

        MoveGenerator movGen(context);
        movGen.generate();

        u64 total = 0;
        u16 moves = 0;

        movGen.forEachMove([&](const PrioratizedMove& pm) {
            std::cout << " " << pm.move.toString();
            if (pm.move.isPromotion()) {
                // using black here since we want to print the type in lowercase.
                ChessPiece promoted(Set::BLACK, (PieceType)pm.move.readPromoteToPieceType());
                std::cout << promoted.toString();
            }
            std::cout << ": ";
            context.MakeMove(pm.move);
            Search search;
            auto result = search.PerftDivide(context, depth - 1);
            std::cout << (result.Nodes == 0 ? 1 : result.Nodes) << std::endl;
            total += result.Nodes;
            moves++;
            context.UnmakeMove();
        });

        std::cout << "\n Moves: " << moves << "\n";
        std::cout << " Total: " << total << "\n";
    }
    else {
        DivideDepthHelpCommand("divide");
    }

    return true;
}

void
DivideDepthHelpCommand(const std::string& command)
{
    std::ostringstream ssCommand;
    ssCommand << command << " <depth>";
    std::string helpText("Divide given board by depth.");
    std::cout << AddLineDivider(ssCommand.str(), helpText);
}

bool
MoveCommand(std::list<std::string>& tokens, GameContext& context)
{
    if (tokens.empty() == false) {
        std::string token = tokens.front();
        auto move = Move::fromPGN(token, context.readToPlay() == Set::WHITE);

        if (move.isInvalid()) {
            std::cout << " Invalid move: " << token << std::endl;
            return false;
        }

        if (!context.TryMakeMove(move)) {
            std::cout << " Invalid move: " << token << std::endl;
            return false;
        }
    }

    return true;
}

void
MoveHelpCommand(const std::string& command)
{
    std::ostringstream ssCommand;
    ssCommand << command << " <move>";
    std::string helpText("Makes a move on the current board.");
    std::cout << AddLineDivider(ssCommand.str(), helpText);
}

bool
UndoCommand(std::list<std::string>&, GameContext& context)
{
    if (context.UnmakeMove()) {
        std::cout << " Undo successful!" << std::endl;
    }
    else {
        std::cout << " Undo failed!" << std::endl;
    }
    return true;
}

void
UndoHelpCommand(const std::string& command)
{
    std::ostringstream ssCommand;
    ssCommand << command;
    std::string helpText("Undos last move.");
    std::cout << AddLineDivider(ssCommand.str(), helpText);
}

bool
EvaluateCommand(std::list<std::string>&, GameContext& context)
{
    Evaluator evaluator;
    MoveGenerator moveGen(context);
    moveGen.generate();
    i32 value = evaluator.Evaluate(context.readChessboard(), moveGen);
    std::cout << " Evaluation: " << value << std::endl;
    return true;
}

void
EvaluateHelpCommand(const std::string& command)
{
    std::ostringstream ssCommand;
    ssCommand << command;
    std::string helpText("Returns engines evaluation of position.");
    std::cout << AddLineDivider(ssCommand.str(), helpText);
}

bool
EvaluateBestMoveCommand(std::list<std::string>& tokens, GameContext& context)
{
    SearchParameters searchParams;
    if (tokens.size() != 0) {
        std::string depth = tokens.front();
        std::from_chars(depth.data(), depth.data() + depth.size(), searchParams.SearchDepth);
    }

    searchParams.Infinite = true;
    SearchResult result = context.CalculateBestMove(searchParams);
    std::cout << " Best Move: " << result.move.toString() << std::endl;
    std::cout << " Score: " << result.score << std::endl;
    return true;
}

void
EvaluateBestMoveHelpCommand(const std::string&)
{
    std::ostringstream ssCommand;
    ssCommand << "bestmove";
    std::string helpText("Returns engines suggestion for best move.");
    std::cout << AddLineDivider(ssCommand.str(), helpText);
}

bool
UCIEnableCommand(std::list<std::string>&, GameContext&)
{
    std::cout << "UCI mode enabled\n";
    UCICommands::UCIEnable();
    std::cout << "Normal mode enabled\n";
    return true;
}

void
UCIEnableHelpCommand(const std::string& command)
{
    std::ostringstream ssCommand;
    ssCommand << command;
    std::string helpText("Puts engine into UCI mode.");
    std::cout << AddLineDivider(ssCommand.str(), helpText);
}

bool
NewGameCommand(std::list<std::string>&, GameContext& context)
{
    context.NewGame();
    return true;
}

void
NewGameHelpCommand(const std::string&)
{
    std::ostringstream ssCommand;
    ssCommand << "newgame";
    std::string helpText("Starts a new game.");
    std::cout << AddLineDivider(ssCommand.str(), helpText);
}

bool
AvailableMovesCommand(std::list<std::string>&, GameContext& context)
{
    std::cout << " Available Moves: \n";

    MoveGenerator moveGen(context);
    moveGen.generate();
    Search search;
    SearchParameters params;
    params.SearchDepth = 1;
    //params.QuiescenceDepth = 2;
    Evaluator evaluator;

    moveGen.forEachMove([&](const PrioratizedMove& pm) {
        bool maximizingPlayer = context.readToPlay() == Set::WHITE;
        context.MakeMove(pm.move);
        std::cout << " " << pm.move.toString();
        if (pm.move.isPromotion()) {
            // using black here since we want to print the type in lowercase.
            ChessPiece promoted(Set::BLACK, (PieceType)pm.move.readPromoteToPieceType());
            std::cout << promoted.toString();
        }
        // auto bestmove = search.CalculateBestMove(context, params);
        i32 evaluation = evaluator.Evaluate(context.readChessboard(), moveGen);

        i32 score = search.CalculateMove(context, maximizingPlayer, 3);
        std::cout << ": " << evaluation << " <" << score << ">\n";
        context.UnmakeMove();
        });

    std::cout << std::endl;
    return true;
}

void
AvailableMovesHelpCommand(const std::string&)
{
    std::ostringstream ssCommand;
    ssCommand << "show";
    std::string helpText("Prints all available moves.");
    std::cout << AddLineDivider(ssCommand.str(), helpText);
}

bool
AboutCommand(std::list<std::string>&, GameContext&)
{
    MESSAGE() << " Elephant Gambit Open Source Chess Engine 2021-2024";  // EGOSCE
    MESSAGE() << " versions:\n   cli:    " << ELEPHANT_CLI_VERSION_STR << "-" << ELEPHANT_CLI_VERSION_PRERELEASE
              << ELEPHANT_CLI_VERSION_SUFFIX << "\n   engine: " << ELEPHANT_GAMBIT_VERSION_STR << "-"
              << ELEPHANT_GAMBIT_VERSION_PRERELEASE << ELEPHANT_GAMBIT_VERSION_SUFFIX;
    MESSAGE() << " Source: https://github.com/looak/elephant";
    MESSAGE() << " Author: Alexander Loodin Ek\n";

    return true;
}

void
AboutHelpCommand(const std::string&)
{
    std::ostringstream ssCommand;
    ssCommand << "about";
    std::string helpText("Version etc.");
    std::cout << AddLineDivider(ssCommand.str(), helpText);
}

}  // namespace CliCommands
