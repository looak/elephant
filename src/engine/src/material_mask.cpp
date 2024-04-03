#include "material_mask.hpp"

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