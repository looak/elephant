#pragma once
#include <bitboard.hpp>

namespace ElephantTest {

class BitboardResultFactory {
public:
    template<typename... occupiedSquares>
    static Bitboard buildExpectedBoard(const occupiedSquares&... squares)
    {
        return internalUnrollOccupiedSquares(squares...);
    }

private:
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