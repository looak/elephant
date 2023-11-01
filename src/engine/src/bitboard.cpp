#include "bitboard.hpp"

constexpr void
Bitboard::inclusiveFillWest(i16 file)
{
    m_board = 0;
    do {
        m_board |= board_constants::fileMasks[file];
        file--;
    } while (file >= 0);
}

// [[nodiscard]] constexpr u64
// inclusiveFillEast(i16 file)
// {
//     u64 result = 0;
//     do {
//         result |= board_constants::fileMasks[file];
//         file++;
//     } while (file < 8);
//     return result;
// }

// [[nodiscard]] constexpr u64
// inclusiveFillSouth(i16 rank)
// {
//     u64 result = 0;
//     do {
//         result |= board_constants::rankMasks[rank];
//         rank--;
//     } while (rank >= 0);
//     return result;
// }

// [[nodiscard]] constexpr u64
// inclusiveFillNorth(i16 rank)
// {
//     u64 result = 0;
//     do {
//         result |= board_constants::rankMasks[rank];
//         rank++;
//     } while (rank < 8);
//     return result;
// }

// [[nodiscard]] constexpr u64
// inclusiveFillNorthEast(i16 file, i16 rank)
// {
//     u64 result = 0;
//     i16 index = 7 + file - rank;
//     do {
//         result |= board_constants::backwardDiagonalMasks[index];
//         index++;
//     } while (index < 15);
//     return result;
// }

// [[nodiscard]] constexpr u64
// inclusiveFillSouthEast(i16 file, i16 rank)
// {
//     u64 result = 0;
//     i16 index = 7 + file - rank;
//     do {
//         result |= board_constants::forwardDiagonalMasks[index];
//         index++;
//     } while (index < 15);
//     return result;
// }

// [[nodiscard]] constexpr u64
// inclusiveFillSouthWest(i16 file, i16 rank)
// {
//     u64 result = 0;
//     i16 index = 7 + file - rank;
//     do {
//         result |= board_constants::backwardDiagonalMasks[index];
//         index--;
//     } while (index >= 0);
//     return result;
// }

// [[nodiscard]] constexpr u64
// inclusiveFillNorthWest(i16 file, i16 rank)
// {
//     u64 result = 0;
//     i16 index = 7 + file - rank;
//     do {
//         result |= board_constants::forwardDiagonalMasks[index];
//         index--;
//     } while (index >= 0);
//     return result;
// }