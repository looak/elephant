#include <serializing/san_parser.hpp>

namespace san_parser {
    Square resolveSquare(PositionReader context, PieceType piece, Square target) {
        // Implement the logic to resolve the square based on the context, piece, and target
        return Square::NullSQ;
    }

    PackedMove resolve(PositionReader context, PieceType piece, Square target, Square source = Square::NullSQ) {
        if (source == Square::NullSQ) {
            source = resolveSource(context, piece, target);
        }
        //return PackedMove::fromData(source, target, piece);
    }

    PieceType parsePieceType(char pieceChar) {
        PieceType piece = PieceType::PAWN;
        if (std::isupper(pieceChar)) {
            piece = piece_constants::notation::piece_from_char(pieceChar);
        }
        return piece;
    }

    Square parseSquare(std::string_view san) {
        char file = san[0];
        char rank = san[1];
        LOG_ERROR_EXPR(std::isalpha(file) && std::isdigit(rank)) << "Invalid square notation: " << san;

        return toSquare(file - 'a', rank - '1');
    }

    PackedMove deserialize(PositionReader context, std::string_view san) {        
        auto cursor = san.begin();
        while (cursor != san.end()) {
            LOG_ERROR_EXPR(!std::isspace(*cursor)) << "Unexpected whitespace found in SAN: " << san;
            cursor++;
        }

        cursor = san.begin();
        PieceType piece = parsePieceType(*cursor);
        
        if (piece != PieceType::PAWN) {
            cursor++;
        }

        Square square = parseSquare(san.substr(cursor - san.begin(), 2));
        cursor += 2;

        // next digit special or another square?


        return PackedMove::NullMove();
    }

} // namespace san_parser