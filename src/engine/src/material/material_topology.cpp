#include <material/material_topology.hpp>

#include <attacks/attacks.hpp>
#include <bitboard.hpp>
#include <bitboard_constants.hpp>
#include <material/material_mask.hpp>

template<Set us>
Bitboard MaterialTopology<us>::computeThreatenedSquaresPawnBulk() const {
    const size_t usIndx = static_cast<size_t>(us);
    Bitboard piecebb = m_material.pawns<us>();

    // special case for a file & h file, removing pawns from a & h file respectively
    // so we don't shift them "off" the board and we shift them only in one direction.
    // cache them and then we combine it with the main piecebb at the end.
    Bitboard westFilePawns = piecebb & board_constants::boundsRelativeMasks[usIndx][west];
    piecebb &= ~westFilePawns;
    westFilePawns = westFilePawns.shiftNorthEastRelative<us>();

    Bitboard eastFilePawns = piecebb & board_constants::boundsRelativeMasks[usIndx][east];
    piecebb &= ~eastFilePawns;
    eastFilePawns = eastFilePawns.shiftNorthWestRelative<us>();

    Bitboard threatbb = westFilePawns | eastFilePawns;
    threatbb |= piecebb.shiftNorthWestRelative<us>();
    threatbb |= piecebb.shiftNorthEastRelative<us>();
    return threatbb;
}

template Bitboard MaterialTopology<Set::BLACK>::computeThreatenedSquaresPawnBulk() const;
template Bitboard MaterialTopology<Set::WHITE>::computeThreatenedSquaresPawnBulk() const;

template<Set us>
Bitboard MaterialTopology<us>::computeThreatenedSquaresKnightBulk() const
{
    Bitboard result = 0;
    Bitboard knights = m_material.knights<us>();
    if (knights == 0)
        return result;  // early out

    while (knights.empty() == false) {
        auto sqr = knights.popLsb();
        result |= attacks::getKnightAttacks(sqr);
    }

    return result;
}

template Bitboard MaterialTopology<Set::WHITE>::computeThreatenedSquaresKnightBulk() const;
template Bitboard MaterialTopology<Set::BLACK>::computeThreatenedSquaresKnightBulk() const;

template<Set us>
template<u8 pieceId>
Bitboard MaterialTopology<us>::computeThreatenedSquaresBishopBulk() const {
    Bitboard occupancy = m_material.combine();
    Bitboard pieces = m_material.read<us>(pieceId);
    Bitboard result{};

    while (pieces.empty() == false) {
        auto sqr = pieces.popLsb();
        result |= attacks::getBishopAttacks(sqr, occupancy.read());
    }

    return result;
}

template Bitboard MaterialTopology<Set::WHITE>::computeThreatenedSquaresBishopBulk<bishopId>() const;
template Bitboard MaterialTopology<Set::BLACK>::computeThreatenedSquaresBishopBulk<bishopId>() const;
template Bitboard MaterialTopology<Set::WHITE>::computeThreatenedSquaresBishopBulk<queenId>() const;
template Bitboard MaterialTopology<Set::BLACK>::computeThreatenedSquaresBishopBulk<queenId>() const;

template<Set us>
template<u8 pieceId>
Bitboard MaterialTopology<us>::computeThreatenedSquaresRookBulk() const {
    Bitboard occupancy = m_material.combine();
    Bitboard pieces = m_material.read<us>(pieceId);
    Bitboard result{};

    while (pieces.empty() == false) {
        auto sqr = pieces.popLsb();
        result |= attacks::getRookAttacks(sqr, occupancy.read());
    }

    return result;
}

template Bitboard MaterialTopology<Set::WHITE>::computeThreatenedSquaresRookBulk<rookId>() const;
template Bitboard MaterialTopology<Set::BLACK>::computeThreatenedSquaresRookBulk<rookId>() const;
template Bitboard MaterialTopology<Set::WHITE>::computeThreatenedSquaresRookBulk<queenId>() const;
template Bitboard MaterialTopology<Set::BLACK>::computeThreatenedSquaresRookBulk<queenId>() const;

template<Set us>
Bitboard
MaterialTopology<us>::computeThreatenedSquaresQueenBulk() const
{
    Bitboard moves = 0;

    moves |= computeThreatenedSquaresRookBulk<queenId>();
    moves |= computeThreatenedSquaresBishopBulk<queenId>();

    return moves;
}

template Bitboard MaterialTopology<Set::WHITE>::computeThreatenedSquaresQueenBulk() const;
template Bitboard MaterialTopology<Set::BLACK>::computeThreatenedSquaresQueenBulk() const;

template<Set us>
Bitboard MaterialTopology<us>::computeThreatenedSquaresKing() const
{
    Bitboard moves;
    const auto kingbb = m_material.king<us>();
    if (kingbb.empty())
        return moves;

    const u32 kingSqr = kingbb.lsbIndex();

    i64 newKingSqr = (const i64)kingSqr - king_constants::maskOffset;

    if (newKingSqr >= 0) {
        moves = (king_constants::moveMask << newKingSqr);
    }
    else if (newKingSqr < 0) {
        moves = (king_constants::moveMask >> -newKingSqr);
    }

    // clean up opposing side
    if ((kingbb & board_constants::fileaMask).empty() == false)
        moves ^= (moves & board_constants::filehMask);
    else if ((kingbb & board_constants::filehMask).empty() == false)
        moves ^= (moves & board_constants::fileaMask);

    return moves;
}

template Bitboard MaterialTopology<Set::WHITE>::computeThreatenedSquaresKing() const;
template Bitboard MaterialTopology<Set::BLACK>::computeThreatenedSquaresKing() const;

template<Set us>
Bitboard MaterialTopology<us>::computeThreatenedDiagonals() const {
    Bitboard occupancy = m_material.combine();
    Bitboard pieces = m_material.queens<us>() | m_material.rooks<us>();
    Bitboard result{};

    while (pieces.empty() == false) {
        auto sqr = pieces.popLsb();
        result |= attacks::getBishopAttacks(sqr, occupancy.read());
    }

    return result;
}

template Bitboard MaterialTopology<Set::WHITE>::computeThreatenedDiagonals() const;
template Bitboard MaterialTopology<Set::BLACK>::computeThreatenedDiagonals() const;

template<Set us>
Bitboard MaterialTopology<us>::computeThreatenedOrthogonals() const {
    Bitboard occupancy = m_material.combine();
    Bitboard pieces = m_material.queens<us>() | m_material.rooks<us>();
    Bitboard result{};

    while (pieces.empty() == false) {
        auto sqr = pieces.popLsb();
        result |= attacks::getRookAttacks(sqr, occupancy.read());
    }

    return result;
}

template Bitboard MaterialTopology<Set::WHITE>::computeThreatenedOrthogonals() const;
template Bitboard MaterialTopology<Set::BLACK>::computeThreatenedOrthogonals() const;

template<Set us>
template<bool includeMaterial, bool pierceKing>
Bitboard MaterialTopology<us>::computeThreatenedSquares() const {
    Bitboard result = ~universe;
    [[maybe_unused]] constexpr Set op = opposing_set<us>();
    [[maybe_unused]] Bitboard kingMask = 0;

    // removing king from opmaterial so it doesn't stop our sliding.
    // needs to be reset later on.
    if constexpr (pierceKing) {
        // can we build a scoped struct to make this a bit cleaner?        
        kingMask = m_material.king<op>();
        m_material.clear<op, kingId>(kingMask);
    }

    result |= computeThreatenedSquaresPawnBulk();
    result |= computeThreatenedSquaresKnightBulk();
    result |= computeThreatenedSquaresBishopBulk();
    result |= computeThreatenedSquaresBishopBulk<queenId>();
    result |= computeThreatenedSquaresRookBulk();
    result |= computeThreatenedSquaresRookBulk<queenId>();
    result |= computeThreatenedSquaresKing();

    if constexpr (pierceKing)
        m_material.write<op, kingId>(kingMask);

    if constexpr (includeMaterial)
        result |= m_material.combine<us>();

    return result;
}

template Bitboard MaterialTopology<Set::WHITE>::computeThreatenedSquares<true, true>() const;
template Bitboard MaterialTopology<Set::WHITE>::computeThreatenedSquares<true, false>() const;
template Bitboard MaterialTopology<Set::WHITE>::computeThreatenedSquares<false, true>() const;
template Bitboard MaterialTopology<Set::BLACK>::computeThreatenedSquares<true, true>() const;
template Bitboard MaterialTopology<Set::BLACK>::computeThreatenedSquares<true, false>() const;
template Bitboard MaterialTopology<Set::BLACK>::computeThreatenedSquares<false, true>() const;