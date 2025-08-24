#include <material/material_mask.hpp>

bool MaterialPositionMask::empty() const
{
    // assuming all of the m_materials are empty if sets are empty.
    return m_set[0] == 0 && m_set[1] == 0;
}

const Bitboard MaterialPositionMask::combine() const
{
    return m_set[0] | m_set[1];
}

const Bitboard MaterialPositionMask::combine(Set set) const
{
    return m_set[static_cast<i8>(set)];
}

void MaterialPositionMask::write(Bitboard mask, Set set, i8 pieceId)
{
    m_set[static_cast<i8>(set)] |= mask;
    m_material[pieceId] |= mask;
}

Bitboard MaterialPositionMask::read(i32 pieceId) const
{
    return m_material[pieceId];
}

Bitboard MaterialPositionMask::read(Set set, i8 pieceId) const
{
    return m_material[pieceId] & m_set[static_cast<i8>(set)];
}

void MaterialPositionMask::clear(Bitboard mask, Set set, i8 pieceId)
{
    m_set[static_cast<i8>(set)] &= ~mask;
    m_material[pieceId] &= ~mask;
}

void MaterialPositionMask::clear()
{
    m_set[0] = 0;
    m_set[1] = 0;
    for (i32 i = 0; i < 6; i++) {
        m_material[i] = 0;
    }
}

ChessPiece MaterialPositionMask::pieceAt(Square sqr) const
{
        Bitboard mask(UINT64_C(1) << (u8)sqr);
 
    for (byte set = 0; set < 2; ++set) {
        if (m_set[set] & mask)
        {
            // unrolled this for loop in an attempt to make it quicker.
            if (m_material[pawnId] & mask)
                return piece_constants::pieces[set][pawnId];
            else if (m_material[knightId] & mask)
                return piece_constants::pieces[set][knightId];
            else if (m_material[bishopId] & mask)
                return piece_constants::pieces[set][bishopId];
            else if (m_material[rookId] & mask)
                return piece_constants::pieces[set][rookId];
            else if (m_material[queenId] & mask)
                return piece_constants::pieces[set][queenId];
            else if (m_material[kingId] & mask)
                return piece_constants::pieces[set][kingId];
        }
    }
    return piece_constants::null();
}

// this code is slightly slower than the above version with a for and a bunch of ifs, probably due to the branch prediction.
// ChessPiece
// Position::pieceAt(Square sqr) const
// {    
//     // 1. Create a mask for the square.
//     Bitboard mask(UINT64_C(1) << (u8)sqr);

//     // 2. Handle the most common case first: the square is empty.    
//     if ((m_materialMask.combine() & mask) == 0) {
//         return ChessPiece::None();
//     }

//     // 3. Determine the piece type using branchless arithmetic.    
//     const int pieceTypeId =
//         (bool)(m_materialMask.knights() & mask) * knightId +
//         (bool)(m_materialMask.bishops() & mask) * bishopId +
//         (bool)(m_materialMask.rooks() & mask) * rookId +
//         (bool)(m_materialMask.queens() & mask) * queenId +
//         (bool)(m_materialMask.kings() & mask) * kingId;
//         // If the result is 0, we know it must be a pawn.

//     // 4. Determine the color with a single bitwise test.    
//     const int colorIdx = (bool)(m_materialMask.black() & mask); // 1 if white, 0 if black

//     return piece_constants::pieces[colorIdx][pieceTypeId];
// }