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

void MaterialPositionMask::write(Bitboard mask, Set set, u8 pieceId)
{
    m_set[static_cast<i8>(set)] |= mask;
    m_material[pieceId] |= mask;
}

Bitboard MaterialPositionMask::read(i32 pieceId) const
{
    return m_material[pieceId];
}

Bitboard MaterialPositionMask::read(Set set, u8 pieceId) const
{
    return m_material[pieceId] & m_set[static_cast<i8>(set)];
}

void MaterialPositionMask::clear(Bitboard mask, Set set, u8 pieceId)
{
    m_set[static_cast<u8>(set)] &= ~mask;
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
    // Branchless, the piece bitboards are one-hot per square so summing bit * (pieceId + 1) gives the PieceType, NONE
    // on an empty square. All eight bitboards share a cache line.
    const u32 shift = static_cast<u32>(sqr);
    u32 type = 0;
    for (u32 pieceId = 0; pieceId < 6; ++pieceId)
        type += static_cast<u32>((m_material[pieceId].read() >> shift) & 1) * (pieceId + 1);

    const u32 black = static_cast<u32>((m_set[1].read() >> shift) & 1);
    return ChessPiece(static_cast<Set>(black), static_cast<PieceType>(type));
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