#include <io/san_parser.hpp>
#include <debug/elephant_exceptions.hpp>
#include <move/generation/move_bulk_generator.hpp>
#include <move/generation/move_gen_isolation.hpp>
#include <position/position.hpp>
#include <variant>

namespace io {
namespace san_parser {
    /**
     * Resolves the source square for a given target square in SAN notation.   */
    Square resolveSource(PositionReader context, bool whiteToMove, PieceType piece, Square target, std::variant<std::monostate, Square, char> disambiguation = std::monostate{}) {
        const u8 pieceId = toPieceIndex(piece);
        Position posCopy = context.copy();
        PositionEditor editor(posCopy);
        Bitboard& materialEditor = editor.material().editMaterial(pieceId);
        const Bitboard targetMask = squareMaskTable[*target]; 

        // remove opponent pieces since material is stored combined.
        materialEditor &= editor.material().set(whiteToMove ? 0 : 1);

        // copy current state of material so that we can use it as a iterator.
        Bitboard materialItr = materialEditor;

        // clear editor since we'll need to pass in the position to our move generator.
        materialEditor.reset();

        // cache possible source squares to ensure we don't have ambiguity.
        std::vector<Square> possibleSources;

        while (materialItr.empty() == false) {
            // piece by piece we'll generate their moves ...
            Square source = toSquare(materialItr.popLsb());
            materialEditor[source] = true;

            BulkMoveGenerator gen(editor);
            Bitboard movesbb;
            if (whiteToMove) {
                movesbb = gen.computeBulkMovesGeneric<Set::WHITE>(pieceId);
            }
            else {
                movesbb = gen.computeBulkMovesGeneric<Set::BLACK>(pieceId);
            }

            // ... checking the generated moves with our target mask.
            movesbb &= targetMask;

            if (movesbb.empty() == false) {
                // If we have valid moves, we can use the source square.
                possibleSources.push_back(source);
            }
        }

        if (possibleSources.size() > 1) {
            if (const char* file_or_rank = std::get_if<char>(&disambiguation)) {
                if (std::isdigit(*file_or_rank)) {
                    // disambiguation by rank
                    byte rank = *file_or_rank - '1';
                    possibleSources.erase(
                        std::remove_if(
                            possibleSources.begin(),
                            possibleSources.end(),
                            [rank](Square sqr) {
                                return (sqr / 8) != rank;
                            }),
                        possibleSources.end());
                }
                else if (std::isalpha(*file_or_rank)) {
                    // disambiguation by file
                    byte file = *file_or_rank - 'a';
                    possibleSources.erase(
                        std::remove_if(
                            possibleSources.begin(),
                            possibleSources.end(),
                            [file](Square sqr) {
                                return (sqr % 8) != file;
                            }),
                        possibleSources.end());
                }
                else {
                    THROW_EXPR(false, ephant::io_error, "san_parser :: Invalid disambiguation character in SAN parsing.");
                }
            }
        }

        // If we still have multiple possible sources, something has gone wrong...
        THROW_EXPR(possibleSources.size() < 2, ephant::io_error, "san_parser :: Ambiguous move detected when resolving source square in SAN parsing.");

        return possibleSources.empty() ? Square::NullSQ : possibleSources[0];
    }

    PackedMove resolve(PositionReader context, bool whiteToMove, PieceType piece, Square target, std::variant<std::monostate, Square, char> disambiguation = std::monostate{}) {
        Square source = Square::NullSQ;
        // if disambiguation context is a whole square we can directly use that as source.
        if (std::get_if<Square>(&disambiguation) == nullptr) {
            source = resolveSource(context, whiteToMove, piece, target, disambiguation);
        }
        PackedMove result(source, target);
        return result;
    }

    PieceType parsePieceType(char pieceChar) {
        PieceType piece = PieceType::PAWN;
        if (std::isupper(pieceChar)) {
            piece = piece_constants::notation::fromChar(pieceChar);
        }
        return piece;
    }

    Square parseSquare(std::string_view san) {
        char file = san[0];
        char rank = san[1];
        THROW_EXPR(std::isalpha(file) && std::isdigit(rank), ephant::io_error, std::format("san_parser :: Invalid square notation: {}", san));
        THROW_EXPR(file >= 'a' && file <= 'h', ephant::io_error, std::format("san_parser :: Invalid file in square notation: {}", san));
        THROW_EXPR(rank >= '1' && rank <= '8', ephant::io_error, std::format("san_parser :: Invalid rank in square notation: {}", san));

        return toSquare(file - 'a', rank - '1');
    }

    PackedMove deserialize(PositionReader context, bool whiteToMove, std::string_view san) {        
        auto cursor = san.begin();
        while (cursor != san.end()) {
            THROW_EXPR(!std::isspace(*cursor), ephant::io_error, std::format("san_parser :: Unexpected whitespace found in SAN: {}", san));
            cursor++;
        }

        cursor = san.begin();
        PieceType piece = parsePieceType(*cursor);
        
        // if no piece specified we assume pawn.
        if (piece != PieceType::PAWN) {
            // If we read a piece we move the cursor forward.
            cursor++;
        }

        size_t lengthLeft = std::distance(cursor, san.end());
        size_t containsCapture = san.find('x', cursor - san.begin()) != std::string_view::npos ? 1 : 0;
        std::variant<std::monostate, Square, char> disambiguation = std::monostate{};
        
        switch (lengthLeft - containsCapture) {
            case 3:
                // Nbd2, R1e1, etc. (disambiguation by file or rank)
                disambiguation = *cursor;
                cursor++;
                break;
            case 4:
                // Nxe5, Rxa8, etc. (captures)
                disambiguation = parseSquare(san.substr(cursor - san.begin(), 2));
                cursor += 2;
                break;
        }
        
        // no additional information to resolve ambiguity in the san.
        Square targetSquare = parseSquare(san.substr(cursor - san.begin(), 2));
        PackedMove result = resolve(context, whiteToMove, piece, targetSquare, disambiguation);

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