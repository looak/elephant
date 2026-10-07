#pragma once

#include "commands/command_api.hpp"
#include "commands/logic/command_registry.hpp"

#include <eval/evaluator.hpp>

class EvaluateCommand : public CommandNoArgs<true> {
public:
    static constexpr std::string_view description() { return "Evaluate the current position."; }
    static constexpr int priority() { return 50; }
    static constexpr std::string_view name() { return "evaluate"; }

    bool execute() override
    {
        const auto material = m_context->readChessPosition().material();
        if (material.whiteKing().count() == 0 || material.blackKing().count() == 0) {
            prnt::err << " Error: Position is not initialized. Use 'new' or 'fen' before evaluating.";
            return false;
        }

        Evaluator evaluator(m_context->readChessPosition());
        const i16 whitePerspective = evaluator.Evaluate();
        const i16 sideToMovePerspective = m_context->readToPlay() == Set::WHITE ? whitePerspective : -whitePerspective;

        prnt::out << " Evaluation (White perspective): " << whitePerspective << " cp" << std::endl;
        prnt::out << " Evaluation (Side-to-move perspective): " << sideToMovePerspective << " cp" << std::endl;
        return true;
    }

    void help(bool extended) override
    {
        if (extended) {
            prnt::out << "\nUsage: " << EvaluateCommand::name() << std::endl << std::endl;
            prnt::out << "Evaluate the current position and print the score in centipawns.";
            return;
        }
        prnt::out << prnt::inject_line_divider(EvaluateCommand::name(), EvaluateCommand::description());
    }
};

REG_COMMAND(EvaluateCommand::name(), EvaluateCommand);
