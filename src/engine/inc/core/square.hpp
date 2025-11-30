#pragma once
#include <core/square.hpp>
#include <diagnostics/assert.hpp>
#include <system/platform.hpp>
#include <math/math.hpp>

enum class Square : uint8_t {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    NullSQ
};

constexpr byte operator*(Square sqr) {
    return static_cast<byte>(sqr);
}

constexpr byte operator/(Square sqr, byte divisor) {
    return static_cast<byte>(sqr) / divisor;
}

constexpr byte operator%(Square sqr, byte modulus) {
    return static_cast<byte>(sqr) % modulus;
}

constexpr bool operator<(Square lhs, Square rhs) {
    return static_cast<byte>(lhs) < static_cast<byte>(rhs);
}

constexpr Square toSquare(byte file, byte rank)  {
    ASSERT_MSG(file <= 7 && rank <= 7, std::format("toSquare :: Invalid file or rank for square conversion. (file: {}, rank: {})", file, rank));
    return static_cast<Square>((rank * 8) + file);
}

template<typename T>
constexpr Square toSquare(T index) {
    static_assert(std::is_integral_v<T>, "toSquare source must be integral");
    ASSERT_MSG(index >= 0 && index <= 63, "toSquare :: Invalid index for square conversion.");
    return static_cast<Square>(index);
}
// TODO: rename these fucntions to file_of and rank_of
constexpr u8 toFile(Square sqr) {
    return mod_by_eight(static_cast<u8>(sqr));
}

constexpr u8 toRank(Square sqr) {
    u8 indx = static_cast<u8>(sqr);
    return indx >> 3; // divide by 8
}

constexpr Square shiftNorth(Square sqr) {
    if (static_cast<byte>(sqr) >= 56) {
        ASSERT_MSG(false, "shiftNorth :: Cannot shift north from rank 8.");
    }
    return static_cast<Square>(static_cast<byte>(sqr) + 8);
}
constexpr Square shiftSouth(Square sqr) {
    if (static_cast<byte>(sqr) < 8) {
        ASSERT_MSG(false, "shiftSouth :: Cannot shift south from rank 1.");
    }
    return static_cast<Square>(static_cast<byte>(sqr) - 8);
}
constexpr Square shiftEast(Square sqr) {
    if (toFile(sqr) == 7) {
        ASSERT_MSG(false, "shiftEast :: Cannot shift east from file h.");
    }
    return static_cast<Square>(static_cast<byte>(sqr) + 1);
}
constexpr Square shiftWest(Square sqr) {
    if (toFile(sqr) == 0) {
        ASSERT_MSG(false, "shiftWest :: Cannot shift west from file a.");
    }
    return static_cast<Square>(static_cast<byte>(sqr) - 1);
}