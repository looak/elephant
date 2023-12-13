#include "commands_print.h"
#include <array>
#include <iostream>
#include <sstream>
#include "chessboard.h"
#include "commands_utils.h"
#include "game_context.h"

namespace CliPrintCommands {
bool
Board(const GameContext& context, const std::string&)
{
    const auto& board = context.readChessboard();
    auto boardItr = board.begin();
    std::array<std::stringstream, 8> ranks;

    byte prevRank = -1;
    do {
        if (prevRank != boardItr.rank()) {
            ranks[boardItr.rank()] << "\n " << (int)(boardItr.rank() + 1) << "  ";
        }

        ranks[boardItr.rank()] << '[' << (*boardItr).readPiece().toString() << ']';
        prevRank = boardItr.rank();
        ++boardItr;

    } while (boardItr != board.end());

    auto rankItr = ranks.rbegin();
    while (rankItr != ranks.rend()) {
        std::cout << (*rankItr).str();
        rankItr++;
    }

    auto ep = board.readPosition().readEnPassant();
    std::cout << "\n\n     A  B  C  D  E  F  G  H\n";
    std::cout << " move: " << context.readMoveCount()
              << "\tturn: " << (context.readToPlay() == Set::WHITE ? "White\n" : "Black\n");
    std::cout << " castling: " << board.readCastlingState().toString() << "\ten passant: " << ep.toString() << "\n\n";

    return true;
}

void
BoardHelp(int, const std::string& command)
{
    std::string helpText("Print the current board (default)");
    std::cout << AddLineDivider(command, helpText);
}

bool
HelpCommand(const GameContext&, const std::string&)
{
    std::cout << " Elephant Gambit CLI print Commands:" << std::endl;
    for (PrintCommandsMap::iterator iter = options.begin(); iter != options.end(); ++iter) {
        options.at(iter->first).second(0, iter->first);
        std::cout << std::endl;
    }

    return true;
}

void
HelpCommandHelp(int, const std::string& command)
{
    std::string helpText("Outputs this help message");
    std::cout << AddLineDivider(command, helpText);
}

bool
PGNCommand(const GameContext& context, const std::string&)
{
    // const auto& movehistory = context.readMoveHistory();
    // std::ostringstream outputPgn;

    // for (auto&& entry : movehistory) {
    //     bool odd = entry.PlyCount & 1;
    //     if (odd)
    //         outputPgn << entry.MoveCount << ". ";

    //     outputPgn << entry.SAN;
    //     if (odd)
    //         outputPgn << " ";
    //     else
    //         outputPgn << "\n";
    // }

    // std::cout << outputPgn.str() << std::endl;

    return true;
}
void
PGNHelpCommand(int, const std::string& command)
{
    std::string helpText("Print the current game in PGN format");
    std::cout << AddLineDivider(command, helpText);
}

}  // namespace CliPrintCommands