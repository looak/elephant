#include <io/printer.hpp>
#include <core/chessboard.hpp>


namespace io {
namespace printer {

void board(std::ostream& output, const Chessboard& board) {
    io::printer::position(output, board.readPosition());
    auto ep = board.readPosition().enPassant();
    auto castling = board.readPosition().castling();

    output << " move: " << board.readMoveCount()
           << "\tturn: " << (board.readToPlay() == Set::WHITE ? "White\n" : "Black\n");
    output << " castling: " << castling.toString() << "\ten passant: " << ep.toString() << "\n\n";
}

void position(std::ostream& output, PositionReader reader) {
    auto posItr = reader.begin();
    std::array<std::stringstream, 8> ranks;

    byte prevRank = 0xFF;
    do {
        if (prevRank != posItr.rank()) {
            ranks[posItr.rank()] << "\n " << (int)(posItr.rank() + 1) << "  ";
        }

        ChessPiece cp = posItr.get();
        ranks[posItr.rank()] << '[' << cp.toString() << ']';
        prevRank = posItr.rank();
        ++posItr;

    } while (posItr != reader.end());

    auto rankItr = ranks.rbegin();
    while (rankItr != ranks.rend()) {
        output << (*rankItr).str();
        rankItr++;
    }
    output << "\n\n     A  B  C  D  E  F  G  H\n";
}

}  // namespace printer
}  // namespace io