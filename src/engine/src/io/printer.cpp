#include <io/printer.hpp>
#include <io/fen_parser.hpp>
#include <core/chessboard.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/ostream_sink.h>

#include <iostream>

namespace io {
namespace printer {

void uciPrinterInit() {
    // Every UCI instance calls this, the logger is global so only the first call creates it.
    if (spdlog::get("uci"))
        return;

    // Synchronous and through std::cout so output is in order, flushed per line, and can be redirected (tests).
    auto cout_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(std::cout, /*force_flush*/ true);
    auto uci_logger = std::make_shared<spdlog::logger>("uci", cout_sink);

    uci_logger->set_level(spdlog::level::info);

    // Only the message, no timestamps or levels, and "\n" rather than the platform default eol.
    uci_logger->set_formatter(std::make_unique<spdlog::pattern_formatter>("%v", spdlog::pattern_time_type::local, "\n"));
    spdlog::register_logger(uci_logger);
}

void board(std::ostream& output, const Chessboard& board) {
    io::printer::position(output, board.readPosition());
    auto ep = board.readPosition().enPassant();
    auto castling = board.readPosition().castling();

    output << " move: " << board.readMoveCount()
           << "\tturn: " << (board.readToPlay() == Set::WHITE ? "White\n" : "Black\n");
    output << " castling: " << castling.toString() << "\ten passant: " << ep.toString() << "\n";
    
    std::string fen;
    io::fen_parser::serialize(board, fen);
    output << "FEN: " << fen << "\n\n";
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

void bitboard(std::ostream& output, const Bitboard& bitboard)
{
    output << "\n";
    for (int rank = 7; rank >= 0; --rank) {
        output << " " << (rank + 1) << "  ";
        for (int file = 0; file < 8; ++file) {
            Square sqr = static_cast<Square>(rank * 8 + file);
            output << '[' << (bitboard[sqr] ? "x" : ".") << ']';
        }
        output << "\n";
    }
    output << "\n     A  B  C  D  E  F  G  H\n\n";
}

void bitboardOperationResult(std::ostream& output, const Bitboard& result, const Bitboard& lhs, const Bitboard& rhs, const std::string& operation)
{
    // Print three boards in a row: lhs <operation> rhs = result
    output << "\n";
    for (int rank = 7; rank >= 0; --rank) {
        // rank label on the left
        output << " " << (rank + 1) << "  ";

        // lhs board
        for (int file = 0; file < 8; ++file) {
            Square sqr = static_cast<Square>(rank * 8 + file);
            output << '[' << (lhs[sqr] ? "x" : ".") << ']';
        }

        // operator between lhs and rhs
        output << "  " << operation << "  ";

        // rhs board
        for (int file = 0; file < 8; ++file) {
            Square sqr = static_cast<Square>(rank * 8 + file);
            output << '[' << (rhs[sqr] ? "x" : ".") << ']';
        }

        // equals and result board
        output << "  =  ";
        for (int file = 0; file < 8; ++file) {
            Square sqr = static_cast<Square>(rank * 8 + file);
            output << '[' << (result[sqr] ? "x" : ".") << ']';
        }

        output << "\n";
    }

    // File labels for each board (aligned with the printed boards)
    output << "\n     A  B  C  D  E  F  G  H   " << operation << "   A  B  C  D  E  F  G  H   =   A  B  C  D  E  F  G  H\n\n";

} 

}  // namespace printer
}  // namespace io