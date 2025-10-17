#include <commands/command_api.hpp>
#include <command_registry/command_registry.hpp>
#include <core/game_context.hpp>
#include <io/fen_parser.hpp>
#include <printer/printer.hpp>

struct FenCommandArgs
{
    std::string fen;
};

static constexpr bool needs_context = true;
class FenCommand : public Command<FenCommandArgs, bool, needs_context> {
public:
	static constexpr std::string_view description() { return "Sets the board to the given FEN string or outputs the FEN string for current board."; }
	static constexpr int priority() { return 50; }
	static constexpr std::string_view name() { return "fen"; }

	// Parses the arguments from a vector of strings.
	FenCommandArgs parse(const std::vector<std::string>& args) override
	{
		if (args.empty()) 
			return {};
		
		std::string fen;
		for (const auto& arg : args) {
			fen += arg + " ";
		}
		return {fen};
	}

	// Executes the command with the given arguments.
	bool execute(const FenCommandArgs& args) override
	{		
		// If fen is empty we want to serialize current context.
		if (args.fen.empty()) {
			std::string output;
			bool result = fen_parser::serialize(m_context->readChessboard(), output);
			prnt::err << " " << (result ? output : "Serializing failed!") << "\n";
			return result;
		}
		m_context->Reset();
		bool ret = fen_parser::deserialize(args.fen.c_str(), m_context->editChessboard());
		if (!ret)
			prnt::err << " Invalid FEN: " << args.fen << "\n";
		return ret;
	}

	// Outputs help information for the command.
	void help() override
	{	
		prnt::out << prnt::inject_line_divider(FenCommand::name(), FenCommand::description());    
	}

};  // class FenCommand

// Register the command in the command registry.
REG_COMMAND(FenCommand::name(), FenCommand);