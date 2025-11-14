#include <io/san_parser.hpp>
#include <debug/elephant_exceptions.hpp>
#include <move/generation/move_generator.hpp>
#include <position/position.hpp>
#include <variant>

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
            return piece_constants::notation::fromChar(promoteChar);
        }
        return PieceType::NONE;
    }

    Square parseSquare(std::string_view san) {
        char file = san[0];
        char rank = san[1];
        THROW_EXPR(std::isalpha(file) && std::isdigit(rank), ephant::io_error, std::format("san_parser :: Invalid square notation: {}", san));
        THROW_EXPR(file >= 'a' && file <= 'h', ephant::io_error, std::format("san_parser :: Invalid file in square notation: {}", san));
        THROW_EXPR(rank >= '1' && rank <= '8', ephant::io_error, std::format("san_parser :: Invalid rank in square notation: {}", san));

        return toSquare(file - 'a', rank - '1');
    }

    template<Set us>
    PackedMove identify(PositionReader position, std::string_view san) {

        auto cursor = san.begin();
        PieceType piece = parsePieceType(*cursor);

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
        }

        std::vector<PrioritizedMove> candidateMoves;

        MoveGenParams params;
        PackedMove candidate = PackedMove::NullMove();
        MoveGenerator<us> gen(position, params);
        do {
            PrioritizedMove mv = gen.pop();
            candidate = mv.move;

            if (candidate.isNull() == true)
                break;

            auto pieceAtSource = position.pieceAt(mv.move.sourceSqr());
            if (pieceAtSource.getType() != piece)
                continue;
                
            if (mv.move.targetSqr() == target)
                candidateMoves.push_back(mv);

        } while(true);

        THROW_EXPR(candidateMoves.size() > 0, ephant::io_error, std::format("san_parser :: No matching moves found for SAN: {}", san));

        if (candidateMoves.size() == 1) {            
            return candidateMoves[0].move;
        }
        else {
            for(const auto& candMove : candidateMoves) {
                if (source != Square::NullSQ) {
                    if (candMove.move.sourceSqr() == source) {
                        return candMove.move;
                    }
                }
                else if (disambiguationChar.has_value()) {
                    char disambig = disambiguationChar.value();
                    if (std::isdigit(disambig)) {
                        byte rank = disambig - '1';
                        if ((candMove.move.sourceSqr() / 8) == rank) {
                            return candMove.move;
                        }
                    }
                    else if (std::isalpha(disambig)) {
                        byte file = disambig - 'a';
                        if ((candMove.move.sourceSqr() % 8) == file) {
                            return candMove.move;
                        }
                    }
                }
            }
        }

        THROW_EXPR(false, ephant::io_error, std::format("san_parser :: Ambiguous SAN notation could not be resolved: {}", san));
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
        // remove capture indication
        cleanSan.erase(std::remove(cleanSan.begin(), cleanSan.end(), 'x'), cleanSan.end());

        PieceType promotion = hasPromotion(cleanSan);
        if (promotion != PieceType::NONE) {
            cleanSan.erase(std::remove(cleanSan.begin(), cleanSan.end(), '='), cleanSan.end());
            cleanSan.pop_back();
        }

        if (whiteToMove)
            return identify<Set::WHITE>(context, cleanSan);
        else
            return identify<Set::BLACK>(context, cleanSan);

    }

    PackedMove deserialize(std::string_view an)
    {
        std::string_view san(an);
        size_t length = san.length();        
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

/*
We could look at the san in reverse, we know that the san will always end with a square notation (e.g., e4, d5, f8, etc.)
By extracting that we can then check the board if to see if we should expect a capture, would not work with e.p. captures.
However, we could then check the first character to see what piece we're dealing with, and by doing so we would have eliminated
the key characters in the san and would be left with any disambiguation characters.


who should own ambiguity resolution? san_parser or move generator?







*/