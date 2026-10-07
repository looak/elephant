#pragma once

#include "commands/command_api.hpp"
#include "commands/logic/command_registry.hpp"
#include <core/game_context.hpp>
#include <io/fen_parser.hpp>
#include <printer/printer.hpp>

struct FenCommandArgs
{
    std::string fen;
};

static constexpr bool needs_context = true;
class FenCommand : public Command<FenCommandArgs, needs_context> {
public:
	static constexpr std::string_view description() { return "Set the board from a FEN string, or show the current board FEN."; }
	static constexpr int priority() { return 50; }
	static constexpr std::string_view name() { return "fen"; }

	// Parses the arguments from a vector of strings.
	std::optional<FenCommandArgs> parse(const std::vector<std::string>& args) override
	{
		if (args.empty()) 
			return std::nullopt;
		
		std::string fen;
		for (const auto& arg : args) {
			fen += arg + " ";
		}
		FenCommandArgs commandArgs;
		commandArgs.fen = fen;
		return commandArgs;
	}

	// Executes the command with the given arguments.
	bool execute(const FenCommandArgs& args) override
	{		
		// If fen is empty we want to serialize current context.
		if (args.fen.empty()) {
			std::string output;
			bool result = io::fen_parser::serialize(m_context->readChessboard(), output);
			prnt::err << " " << (result ? output : "Serializing failed!");
			return result;
		}
		m_context->Reset();
		bool ret = io::fen_parser::deserialize(args.fen.c_str(), m_context->editChessboard());
		if (!ret)
			prnt::err << " Invalid FEN: " << args.fen << std::endl;
		return ret;
	}

	// Outputs help information for the command.
	void help(bool extended) override
	{	
		if (extended) {
			prnt::out << "\nUsage: " << FenCommand::name() << " <fen_string>" << std::endl << std::endl;
			prnt::out << "Set the board from a FEN string, or show the current board FEN." << std::endl;
			prnt::out << "  An invalid FEN string will result in nothing being set." << std::endl;
			prnt::out << "Inputs:" << std::endl;
			prnt::out << "  <fen_string> - Optional. FEN string to set the board to; if not provided, the current board FEN is output." << std::endl;
			return;
		}
		prnt::out << prnt::inject_line_divider(FenCommand::name(), FenCommand::description());
	}

};  // class FenCommand

// Register the command in the command registry.
REG_COMMAND(FenCommand::name(), FenCommand);