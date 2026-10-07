#pragma once

#include "commands/command_api.hpp"
#include "commands/logic/command_registry.hpp"
#include "printer/printer.hpp"

#include <eval/evaluator.hpp>
#include <algorithm>
#include <array>
#include <iomanip>
#include <sstream>

struct PrintCommandArgs
{
    bool pretty;
    bool flipped;
    bool pgn;
};

class PrintCommand : public ReadOnlyCommand<PrintCommandArgs> {
public:
    static constexpr std::string_view description() { return "Show board, history, evaluation, and position stats."; }
    static constexpr int priority() { return 200; }
    static constexpr std::string_view name() { return "print"; }    

    bool execute(const PrintCommandArgs&) override
    {        
        const auto position = readPosition();
        const auto& context = readContext();

        std::vector<std::string> boardPanel;
        {
            auto posItr = position.begin();
            std::array<std::stringstream, 8> ranks;
            byte prevRank = 0xFF;
            do {
                if (prevRank != posItr.rank()) {
                    ranks[posItr.rank()] << " " << static_cast<int>(posItr.rank() + 1) << "  ";
                }

                ranks[posItr.rank()] << '[' << posItr.get().toString() << ']';
                prevRank = posItr.rank();
                ++posItr;
            } while (posItr != position.end());

            for (auto rankItr = ranks.rbegin(); rankItr != ranks.rend(); ++rankItr) {
                boardPanel.push_back(rankItr->str());
            }
            boardPanel.push_back("");
            boardPanel.push_back("     A  B  C  D  E  F  G  H");
        }

        std::stringstream hashStream;
        hashStream << "0x" << std::hex << std::setfill('0') << std::setw(16) << position.hash();

        Evaluator evaluator(position);
        const i16 whitePerspective = evaluator.Evaluate();
        const i16 sideToMovePerspective = context.readToPlay() == Set::WHITE ? whitePerspective : -whitePerspective;

        auto formatEval = [](i16 centipawnScore) {
            std::stringstream stream;
            stream << std::showpos << std::fixed << std::setprecision(2) << static_cast<double>(centipawnScore) / 100.0;
            return stream.str();
        };

        std::vector<std::string> historyPanel;
        historyPanel.push_back("history:");

        const auto& history = context.readGameHistory().moveUndoUnits;
        if (history.empty()) {
            historyPanel.push_back(" (none)");
        }
        else {
            for (size_t i = 0; i < history.size(); i += 2) {
                std::stringstream line;
                line << ' ' << (i / 2 + 1) << ". " << history[i].move.toString();
                if (i + 1 < history.size()) {
                    line << "   " << history[i + 1].move.toString();
                }
                historyPanel.push_back(line.str());
            }
        }

        std::vector<std::string> statsPanel;
        statsPanel.push_back("stats:");
        statsPanel.push_back(" turn: " + std::string(context.readToPlay() == Set::WHITE ? "White" : "Black"));
        statsPanel.push_back(" move: " + std::to_string(context.readMoveCount()) + "  ply: " + std::to_string(context.readPly()));
        statsPanel.push_back(" castling: " + position.castling().toString());
        statsPanel.push_back(" en passant: " + position.enPassant().toString());
        statsPanel.push_back(" hash: " + hashStream.str());
        statsPanel.push_back(" eval (white): " + formatEval(whitePerspective));
        statsPanel.push_back(" eval (to move): " + formatEval(sideToMovePerspective));

        auto panelWidth = [](const std::vector<std::string>& lines) -> size_t {
            size_t width = 0;
            for (const auto& line : lines) {
                width = std::max(width, line.size());
            }
            return width;
        };
        auto lineAt = [](const std::vector<std::string>& lines, size_t index) -> std::string {
            return index < lines.size() ? lines[index] : "";
        };
        auto padRight = [](const std::string& text, size_t width) -> std::string {
            if (text.size() >= width) {
                return text;
            }
            return text + std::string(width - text.size(), ' ');
        };

        const size_t boardWidth = panelWidth(boardPanel) + 3;
        const size_t historyWidth = panelWidth(historyPanel) + 3;
        const size_t totalLines = std::max({boardPanel.size(), historyPanel.size(), statsPanel.size()});

        for (size_t i = 0; i < totalLines; ++i) {
            const std::string boardLine = padRight(lineAt(boardPanel, i), boardWidth);
            const std::string historyLine = padRight(lineAt(historyPanel, i), historyWidth);
            const std::string statsLine = lineAt(statsPanel, i);
            prnt::out << boardLine << historyLine << statsLine << std::endl;
        }

        return true;
    }

    std::optional<PrintCommandArgs> parse(const std::vector<std::string>& args) override
    {
        PrintCommandArgs parsedArgs{};
        for (const auto& arg : args) {
            if (arg == "--pretty") {
                parsedArgs.pretty = true;
            }
            else if (arg == "--flipped") {
                parsedArgs.flipped = true;
            }
            else if (arg == "--pgn") {
                parsedArgs.pgn = true;
            }
            else {
                prnt::err << "Error: Unknown argument '" << arg << "'";
                return std::nullopt;
            }
        }
        return parsedArgs;
    }

    void help(bool extended) override
    {
        if (extended) {
            prnt::out << "\nUsage: " << PrintCommand::name() << " [--pretty] [--flipped] [--pgn]" << std::endl << std::endl;
            prnt::out << "Show the current game state in the console, including board, history, evaluation, and key position stats.";
            prnt::out << "Options:";
            prnt::out << "  --pretty    Print the board in a human-friendly format.";
            prnt::out << "  --flipped   Print the board from Black's perspective.";
            prnt::out << "  --pgn       Print the game in PGN format.";
            return;
        }
        prnt::out << prnt::inject_line_divider(PrintCommand::name(), PrintCommand::description());

    }
};

REG_COMMAND(PrintCommand::name(), PrintCommand);
