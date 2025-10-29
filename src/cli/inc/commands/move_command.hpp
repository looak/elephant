#pragma once

#include "commands/command_api.hpp"
#include "commands/logic/command_registry.hpp"
#include <core/game_context.hpp>
#include <move/move_executor.hpp>
#include <io/san_parser.hpp>
#include <move/move.hpp>

struct MoveCommandArgs
{
    PackedMove move;
};

/**
 * @class MoveCommand
 * @brief Command to make a move in the current game.
 * 
 * This command parses a move in standard algebraic notation (SAN) and applies it to the current game context.
 * It requires a valid game context to function properly.
 * 
 * Usage:
 *   move <move>
 * 
 * Example:
 *   move e4
 * 
 * @author Alexander Loodin Ek
 */

class MoveCommand : public Command<MoveCommandArgs, true> {
public:
    static constexpr std::string_view description() { return "Makes a move in the current game."; }
    static constexpr int priority() { return 150; }
    static constexpr std::string_view name() { return "move"; }

    std::optional<MoveCommandArgs> parse(const std::vector<std::string>& args) override
    {
        MoveCommandArgs parsedArgs{};

        if (args.size() != 1) {
            prnt::err << " Error: 'move' command requires a move argument.";
            return std::nullopt;
        }
        PackedMove move;
        try {
            move = san_parser::deserialize(m_context->readChessPosition(), m_context->readToPlay() == Set::WHITE ? true : false, args[0]); 
        } catch (const std::exception& e) {
            prnt::err << e.what();
            return std::nullopt;        
        }

        parsedArgs.move = move;
        return parsedArgs;
    }

    bool execute(const MoveCommandArgs& args) override
    {   
        try {
            MoveExecutor exec(*m_context);
            exec.makeMove(args.move);
        } catch (const std::exception& e) {
            prnt::err << e.what();
            return false;
        }

        return true;
    }

    void help(bool extended) override
    {
        if (extended) {
            prnt::out << "\nUsage: " << MoveCommand::name() << " <move>" << std::endl << std::endl;
            prnt::out << "Makes a move in the current game. The move should be specified in standard algebraic notation.";
            return;
        }
        prnt::out << prnt::inject_line_divider(MoveCommand::name(), MoveCommand::description());
    }
};

REG_COMMAND(MoveCommand::name(), MoveCommand);