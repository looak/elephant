#include "commands_print.h"
#include <iostream>

namespace PrintCommands
{
void Board(const Chessboard& board, const std::string input)
{
    auto boardItr = Chessboard::Iterator(const_cast<Chessboard&>(board));

    while (!boardItr.end())
    {
        
    }
    for (byte rank = 8; rank >= 1; rank--)
    {
        std::cout << " > " << (int)rank << "  ";
        for (byte file = 'a'; file <= 'h'; file++)
        {
            Notation position = Notation::BuildPosition(file, rank);
            const auto& piece = board.readTile(std::move(position));
            // auto val = PieceDef::printable(tmp);
            std::cout << '[' << piece.toString() << ']';
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    std::cout << " >     A  B  C  D  E  F  G  H";
}

void BoardHelp(int option, const std::string command)
{

}

void HelpCommand(const Chessboard& board, const std::string input)
{

}

void HelpCommandHelp(int option, const std::string command)
{
    
}
} // namespace PrintCommands