#include <io/san_parser.hpp>
#include <move/generation/move_bulk_generator.hpp>
#include <move/generation/move_gen_isolation.hpp>
#include <position/position.hpp>

namespace san_parser {
    /**
     * Resolves the source square for a given target square in SAN notation.   */
    Square resolveSource(PositionReader context, bool whiteToMove, PieceType piece, Square target) {
        const u8 pieceId = toPieceIndex(piece);
        Position posCopy = context.copy();
        PositionEditor editor(posCopy);
        Bitboard& materialEditor = editor.material().editMaterial(pieceId);
        const Bitboard targetMask = squareMaskTable[*target]; 

        // remove white pieces since material is stored combined.
        materialEditor &= editor.material().set(whiteToMove ? 0 : 1);

        // copy current state of material so that we can use it as a iterator.
        Bitboard materialItr = materialEditor;

        // clear editor since we'll need to pass in the position to our move generator.
        materialEditor.reset();

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
                return source;
            }
        }       

        return Square::NullSQ;
    }

    PackedMove resolve(PositionReader context, bool whiteToMove, PieceType piece, Square target, Square source = Square::NullSQ) {
        if (source == Square::NullSQ) {
            source = resolveSource(context, whiteToMove, piece, target);
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
        LOG_ERROR_EXPR(std::isalpha(file) && std::isdigit(rank)) << "Invalid square notation: " << san;

        return toSquare(file - 'a', rank - '1');
    }

    PackedMove deserialize(PositionReader context, bool whiteToMove, std::string_view san) {        
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

        Square targetSquare = parseSquare(san.substr(cursor - san.begin(), 2));
        cursor += 2;

        PackedMove result = resolve(context, whiteToMove, piece, targetSquare);

        return result;
    }

} // namespace san_parser