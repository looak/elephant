#pragma once
#include <bitboard/bitboard.hpp>
#include <string_view>
#include <array>
#include <string>
#include <stdexcept>

namespace ElephantTest {

class BitboardResultFactory {
public:
    template<typename... occupiedSquares>
    static Bitboard buildExpectedBoard(const occupiedSquares&... squares)
    {
        return internalUnrollOccupiedSquares(squares...);
    }

    // Build a board from a visual ASCII diagram. Flexible parsing:
    // - Provide at least 64 cell characters total (8 ranks × 8 files) in reading order.
    // - We advance one file for each character in the sets below:
    //     set:    x X 1 o O * +
    //     empty:  . - _ 0
    // - Any other characters (spaces, brackets, rank/file labels) are ignored.
    // - Topmost 8 recognized cells map to rank 8, left-to-right A..H; then rank 7, ..., down to rank 1.
    // This lets you paste compact grids like "..x....." or spaced layouts like ". . x . . . . ." or
    // even bracketed comment art; only cell-mark characters are counted.    
    static Bitboard buildBoardFromAscii(const std::array<std::string_view, 8>& rows)
    {
        Bitboard result;
        for (size_t visualRow = 0; visualRow < 8; ++visualRow) {
            const size_t rank = 7 - visualRow; // rows[0] => rank 8
            size_t file = 0;

            for (char ch : rows[visualRow]) {
                if (isSetChar(ch) || isEmptyChar(ch)) {
                    if (file < 8) {
                        if (isSetChar(ch)) {
                            const size_t idx = rank * 8 + file;
                            result[static_cast<Square>(idx)] = true;
                        }
                        ++file;                        
                    }
                }
            }

            if (file < 8) {
                throw std::invalid_argument("BitboardResultFactory::buildBoardFromRows: row has fewer than 8 recognized cells");
            }
        }

        return result;
    }

private:
    static constexpr bool isSetChar(char ch) noexcept {
        switch (ch) {
            case 'x': case 'X': case '1': case 'o': case 'O': case '*': case '+':
                return true;
            default:
                return false;
        }
    }

    static constexpr bool isEmptyChar(char ch) noexcept {
        switch (ch) {
            case '.': case '-': case '_': case '0':
                return true;
            default:
                return false;
        }
    }
    template<typename sqr, typename... occupiedSquares>
    static Bitboard internalUnrollOccupiedSquares(sqr square, const occupiedSquares&... squares)
    {
        return internalUnrollOccupiedSquares(square) | internalUnrollOccupiedSquares(squares...);
    }

    template<typename sqr>
    static Bitboard internalUnrollOccupiedSquares(sqr square)
    {
        Bitboard board;
        board[static_cast<Square>(square)] = true;
        return board;
    }

};

}  // namespace ElephantTest