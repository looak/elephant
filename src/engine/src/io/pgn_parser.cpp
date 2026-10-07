#include <io/pgn_parser.hpp>
#include <io/san_parser.hpp>
#include <system/platform.hpp>
#include <core/game_context.hpp>
#include <move/move_executor.hpp>

namespace io {
namespace pgn_parser {

    std::string removeComments(std::string_view san) {
        std::string cleaned_annotations;        
        cleaned_annotations.reserve(san.size());

        // remove comments by rebuilding the string from the string_view
        size_t last_pos = 0;
        size_t openBracket = san.find('{');
        while (openBracket != std::string_view::npos) {
            // Append the part before the comment
            cleaned_annotations.append(san.substr(last_pos, openBracket - last_pos));

            size_t closeBracket = san.find('}', openBracket);
            if (closeBracket == std::string_view::npos) {
                // Malformed PGN with an unclosed comment, stop processing here.
                break;
            }
            // Move the cursor past the comment to the character after '}'
            last_pos = closeBracket + 1;
            openBracket = san.find('{', last_pos);
        }
        // Append the rest of the string after the last comment
        cleaned_annotations.append(san.substr(last_pos));
        return cleaned_annotations;
    }

    void parseMovePair(GameContext& game, const std::array<std::string, 2>& move_pair) {
        // Parse the individual moves from the move pair
        std::string whiteMove = move_pair[0];
        std::string blackMove = move_pair[1];
        
        bool blackToMove = false;

        do {            
            PackedMove move = san_parser::deserialize(game.readChessboard().readPosition(), !blackToMove, move_pair[(u8)blackToMove]);
            blackToMove = !blackToMove;
            game.MakeMove<true>(move);
        } while(blackToMove);

    }

    void deserialize(GameContext& game, std::string_view san_view) {
        // identify the beginning of the move annotations by finding the last ']'.
        size_t lastBracket = san_view.find_last_of(']');
        if (lastBracket != std::string_view::npos) {
            // Extract the move annotations
            san_view = san_view.substr(lastBracket + 1);
        }

        std::string annotations = removeComments(san_view);

        u32 expectedMoveNumber = 1;

        // Process the annotations
        for (size_t i = 0; i < annotations.size();) {
            char c = annotations[i];
            if (c == ' ') continue; // move past whitespaces
            if (std::isdigit(c)) {
                // we're expecting a move number like 22. or 22...
                u32 moveNumber = static_cast<u32>(c - '0');
                // Check for the full move number (could be more than one digit)
                while (std::isdigit(annotations[++i])) {
                    moveNumber = moveNumber * 10 + static_cast<u32>(annotations[i] - '0');
                }
                // Now we have the full move number
                if (moveNumber == expectedMoveNumber) {
                    // This is the expected move
                    expectedMoveNumber++;
                }
                i++; // move past the period behind move numbers.
            }

            u8 pairIndex = 0;
            std::array<std::string, 2> move_pair;
            while (pairIndex < 2) {
                // identify the move pair by looking for white spaces.
                size_t nextSpace = annotations.find(' ', i);
                // if no space assume eol.
                if (nextSpace == std::string_view::npos) 
                    nextSpace = annotations.size();

                move_pair[pairIndex++] = annotations.substr(i, nextSpace - i);
                i = nextSpace + 1; // move past the space
            }

            parseMovePair(game, move_pair);

        }

    }

} // namespace pgn_parser
} // namespace io