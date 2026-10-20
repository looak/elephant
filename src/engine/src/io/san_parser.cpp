#include <io/san_parser.hpp>
#include <diagnostics/exceptions.hpp>
#include <move/generation/tusk/move_generator.hpp>
#include <position/position.hpp>
#include <variant>
#include <format>
#include <optional>

namespace io {
namespace san_parser {
    PieceType parsePieceType(char pieceChar) {
        PieceType piece = PieceType::PAWN;
        if (std::isupper(pieceChar)) {
            piece = piece_constants::notation::fromChar(pieceChar);
        }
        return piece;
    }

    PieceType hasPromotion(std::string_view san) {
        if (std::isalpha(san.back())) {
            char promoteChar = san.back();
            PieceType promoteTo = piece_constants::notation::fromChar(promoteChar);
            THROW_EXPR(promoteTo != PieceType::NR_OF_PIECES && promoteTo != PieceType::KING, ephant::io_error,
                std::format("san_parser :: Invalid promotion in SAN: {}", san));
            return promoteTo;
        }
        return PieceType::NONE;
    }

    Square parseSquare(std::string_view san) {
        char file = san[0];
        char rank = san[1];
        THROW_EXPR(std::isalpha(file) && std::isdigit(rank), ephant::io_error, std::format("san_parser :: Invalid square notation: {}", san));
        THROW_EXPR(file >= 'a' && file <= 'h', ephant::io_error, std::format("san_parser :: Invalid file in square notation: {}", san));
        THROW_EXPR(rank >= '1' && rank <= '8', ephant::io_error, std::format("san_parser :: Invalid rank in square notation: {}", san));

        return to_square(static_cast<byte>(file - 'a'), static_cast<byte>(rank - '1'));
    }

    // SAN names a move by its piece, its target and as much of its source as tells it apart from the other legal
    // moves, so it is read as a filter over the legal moves and has to match exactly one of them.
    template<Set us>
    PackedMove identify(PositionReader position, std::string_view san, PieceType promotion) {
        THROW_EXPR(!san.empty(), ephant::io_error, std::format("san_parser :: Empty SAN after its suffixes: {}", san));
        PieceType piece = parsePieceType(san.front());
        THROW_EXPR(piece != PieceType::NR_OF_PIECES, ephant::io_error, std::format("san_parser :: Unknown piece in SAN: {}", san));

        if (piece != PieceType::PAWN) {
            san.remove_prefix(1);
        }

        Square target = Square::NullSQ;
        Square source = Square::NullSQ;
        std::optional<char> disambiguationChar;

        switch(san.length()) {
            case 2: {
                // e4, f3, b5, etc.
                target = parseSquare(san);
                break;
            }
            case 3: {
                // Nbd2, R1e5, etc.
                disambiguationChar = san[0];
                target = parseSquare(san.substr(1,2));
                break;
            }
            case 4: {
                source = parseSquare(san.substr(0,2));
                target = parseSquare(san.substr(2,2));
                break;
            }
            default:
                THROW_EXPR(false, ephant::io_error, std::format("san_parser :: Unsupported SAN notation length for identification: {}", san));
                return PackedMove::NullMove();
        }

        auto matches = [&](PackedMove move) {
            if (move.targetSqr() != target || position.pieceAt(move.sourceSqr()).getType() != piece)
                return false;
            // a pawn reaching the last rank promotes, and SAN names to what.
            if (move.isPromotion() != (promotion != PieceType::NONE))
                return false;
            if (move.isPromotion() && move.readPromoteToPieceType() != static_cast<i32>(promotion))
                return false;
            if (source != Square::NullSQ)
                return move.sourceSqr() == source;
            if (disambiguationChar.has_value()) {
                char disambig = disambiguationChar.value();
                if (disambig >= '1' && disambig <= '8')
                    return (move.sourceSqr() / 8) == static_cast<byte>(disambig - '1');
                if (disambig >= 'a' && disambig <= 'h')
                    return (move.sourceSqr() % 8) == static_cast<byte>(disambig - 'a');
                THROW_EXPR(false, ephant::io_error, std::format("san_parser :: Invalid disambiguation in SAN: {}", san));
                return false;
            }
            return true;
        };

        PackedMove found = PackedMove::NullMove();
        [[maybe_unused]] u32 count = 0;
        tusk::CheckInfo<us> checkInfo(position);
        tusk::MoveGenerator<us> gen(position, checkInfo);
        tusk::MoveGenResult<us> moves = gen.generateAll();
        while (PrioritizedMove mv = moves.next()) {
            if (matches(mv.move)) {
                if (count++ == 0)
                    found = mv.move;
            }
        }

        THROW_EXPR(count > 0, ephant::io_error, std::format("san_parser :: No matching moves found for SAN: {}", san));
        THROW_EXPR(count < 2, ephant::io_error, std::format("san_parser :: Ambiguous SAN notation could not be resolved: {}", san));
        return found;
    }

    // castling is the legal castling move to that side, not any king move to its square.
    template<Set us>
    PackedMove castle(PositionReader position, bool kingSide) {
        const byte file = kingSide ? 6 : 2;
        tusk::CheckInfo<us> checkInfo(position);
        tusk::MoveGenerator<us> gen(position, checkInfo);
        tusk::MoveGenResult<us> moves = gen.generateAll();
        while (PrioritizedMove mv = moves.next()) {
            if (mv.move.isCastling() && (mv.move.targetSqr() % 8) == file)
                return mv.move;
        }
        THROW_EXPR(false, ephant::io_error, std::format("san_parser :: Castling {} isn't legal", kingSide ? "O-O" : "O-O-O"));
        return PackedMove::NullMove();
    }

    PackedMove deserialize(PositionReader context, bool whiteToMove, std::string_view san) {        
        auto cursor = san.begin();
        while (cursor != san.end()) {
            THROW_EXPR(!std::isspace(*cursor), ephant::io_error, std::format("san_parser :: Unexpected whitespace found in SAN: {}", san));
            cursor++;
        }

        std::string cleanSan(san);
        // remove check and mate indications
        if (cleanSan.ends_with('+') || cleanSan.ends_with('#')) {
            cleanSan.pop_back();
        }
        THROW_EXPR(!cleanSan.empty(), ephant::io_error, std::format("san_parser :: Empty SAN: '{}'", san));

        // handle castling, some writers castle with zeros
        if (cleanSan == "O-O" || cleanSan == "0-0" || cleanSan == "O-O-O" || cleanSan == "0-0-0") {
            const bool kingSide = cleanSan.size() == 3;
            return whiteToMove ? castle<Set::WHITE>(context, kingSide) : castle<Set::BLACK>(context, kingSide);
        }

        // remove capture indication
        cleanSan.erase(std::remove(cleanSan.begin(), cleanSan.end(), 'x'), cleanSan.end());

        PieceType promotion = hasPromotion(cleanSan);
        if (promotion != PieceType::NONE) {
            cleanSan.erase(std::remove(cleanSan.begin(), cleanSan.end(), '='), cleanSan.end());
            cleanSan.pop_back();
        }

        if (whiteToMove)
            return identify<Set::WHITE>(context, cleanSan, promotion);
        else
            return identify<Set::BLACK>(context, cleanSan, promotion);

    }

    PackedMove deserialize(std::string_view an) {
        std::string_view san(an);
        Square sourceSquare = parseSquare(san.substr(0, 2));
        Square targetSquare = parseSquare(san.substr(2, 2));

                
        THROW_EXPR(sourceSquare != Square::NullSQ && targetSquare != Square::NullSQ, ephant::io_error,
                std::format("san_parser :: Invalid square notation in simplified SAN deserialization: {}", san));

        PackedMove result(sourceSquare, targetSquare);

        if (san.length() > 4) {
            char promoteChar = san[4];
            if (promoteChar == '=') {
                promoteChar = san[5];
            }
            PieceType promoteTo = piece_constants::notation::fromChar(promoteChar);
            ChessPiece promotePiece(Set::WHITE, promoteTo); // set does not matter here.
            result.setPromoteTo(promotePiece);
        }

        return result;
    }

} // namespace san_parser
} // namespace io