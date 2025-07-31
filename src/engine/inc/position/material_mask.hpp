#pragma once
#include "bitboard.hpp"
#include "chess_piece.h"


struct MaterialMask {
    Bitboard material[6]{};

    inline constexpr Bitboard combine() const
    {
        return material[pawnId] | material[knightId] | material[bishopId] | material[rookId] | material[queenId] |
            material[kingId];
    }

    // [[nodiscard]] constexpr Bitboard operator[](i32 indx) { return material[indx]; }
    // [[nodiscard]] constexpr Bitboard operator[](i32 indx) const { return material[indx]; }

    [[nodiscard]] constexpr const Bitboard& kings() const { return material[kingId]; }
    [[nodiscard]] constexpr const Bitboard& queens() const { return material[queenId]; }
    [[nodiscard]] constexpr const Bitboard& rooks() const { return material[rookId]; }
    [[nodiscard]] constexpr const Bitboard& bishops() const { return material[bishopId]; }
    [[nodiscard]] constexpr const Bitboard& knights() const { return material[knightId]; }
    [[nodiscard]] constexpr const Bitboard& pawns() const { return material[pawnId]; }
};

struct MutableMaterialProxySquare {
public:
    MutableMaterialProxySquare(u64& proxyMaterial, u64& proxySet, Square sqr) : m_setProxy(proxySet), m_matProxy(proxyMaterial), m_sqr(sqr) {}

    void operator=(bool value) {
        m_setProxy[m_sqr] = value;
        m_matProxy[m_sqr] = value;
    }

private:
    BitboardImpl<u64&> m_setProxy;
    BitboardImpl<u64&> m_matProxy;
    Square m_sqr;
};

struct MutableMaterialProxy {
public:
    MutableMaterialProxy(Bitboard* set, Bitboard* material) : m_set(set), m_material(material) {}
    MutableMaterialProxy(const MutableMaterialProxy& other) : m_set(other.m_set), m_material(other.m_material) {}

    MutableMaterialProxy& operator=(MutableMaterialProxy other) {
        if (this != &other) {
            std::swap(m_set, other.m_set);
            std::swap(m_material, other.m_material);
        }
        return *this;
    };

    constexpr bool operator[](Square sqr) const {
        // extract bit in each board individually to verify they are the same
        bool setMask = (*m_set & squareMaskTable[static_cast<u8>(sqr)]) != 0;
        bool matMask = (*m_material & squareMaskTable[static_cast<u8>(sqr)]) != 0;

        FATAL_ASSERT(setMask == matMask) << "Inconsistent Bitboards, these values should be the same, terminating program.";
        return setMask && matMask;
    }

    MutableMaterialProxySquare operator[](Square sqr) {
        return MutableMaterialProxySquare(m_set->edit(), m_material->edit(), sqr);
    }

private:
    Bitboard* m_set;
    Bitboard* m_material;
};

struct MaterialPositionMask {
    friend class Position;
private:
    Bitboard m_set[2];
    Bitboard m_material[6];
public:
    bool empty() const;

    void write(Bitboard mask, Set set, i8 pieceId);
    template<Set us> void write(Bitboard mask, i32 pieceId);
    template<Set us, i32 pieceId> void write(Bitboard mask);

    [[nodiscard]] Bitboard read(i32 pieceId) const;
    [[nodiscard]] Bitboard read(Set set, i8 pieceId) const;
    template<Set us, i32 pieceId> [[nodiscard]] Bitboard read() const;
    template<Set us> [[nodiscard]] Bitboard read(i8 pieceId) const;

    [[nodiscard]] const Bitboard combine() const;
    [[nodiscard]] const Bitboard combine(Set set) const;
    template<Set us> [[nodiscard]] constexpr Bitboard combine() const;

    void clear();
    void clear(Bitboard mask, Set set, i8 pieceId);
    template<Set us> void clear(Bitboard mask);
    template<Set us> void clear(Bitboard mask, i32 pieceId);
    template<Set us, i32 pieceId> void clear(Bitboard mask);

    template<Set us> [[nodiscard]] constexpr Bitboard kings() const;
    template<Set us> [[nodiscard]] constexpr Bitboard queens() const;
    template<Set us> [[nodiscard]] constexpr Bitboard rooks() const;
    template<Set us> [[nodiscard]] constexpr Bitboard bishops() const;
    template<Set us> [[nodiscard]] constexpr Bitboard knights() const;
    template<Set us> [[nodiscard]] constexpr Bitboard pawns() const;

    [[nodiscard]] constexpr Bitboard kings() const { return m_material[kingId]; };
    [[nodiscard]] constexpr Bitboard queens() const { return m_material[queenId]; };
    [[nodiscard]] constexpr Bitboard rooks() const { return m_material[rookId]; };
    [[nodiscard]] constexpr Bitboard bishops() const { return m_material[bishopId]; };
    [[nodiscard]] constexpr Bitboard knights() const { return m_material[knightId]; };
    [[nodiscard]] constexpr Bitboard pawns() const { return m_material[pawnId]; };

    [[nodiscard]] constexpr Bitboard whiteKing() const { return kings<Set::WHITE>(); }
    [[nodiscard]] constexpr Bitboard blackKing() const { return kings<Set::BLACK>(); }
    [[nodiscard]] constexpr Bitboard whiteQueens() const { return queens<Set::WHITE>(); }
    [[nodiscard]] constexpr Bitboard blackQueens() const { return queens<Set::BLACK>(); }
    [[nodiscard]] constexpr Bitboard whiteRooks() const { return rooks<Set::WHITE>(); }
    [[nodiscard]] constexpr Bitboard blackRooks() const { return rooks<Set::BLACK>(); }
    [[nodiscard]] constexpr Bitboard whiteBishops() const { return bishops<Set::WHITE>(); }
    [[nodiscard]] constexpr Bitboard blackBishops() const { return bishops<Set::BLACK>(); }
    [[nodiscard]] constexpr Bitboard whiteKnights() const { return knights<Set::WHITE>(); }
    [[nodiscard]] constexpr Bitboard blackKnights() const { return knights<Set::BLACK>(); }
    [[nodiscard]] constexpr Bitboard whitePawns() const { return pawns<Set::WHITE>(); }
    [[nodiscard]] constexpr Bitboard blackPawns() const { return pawns<Set::BLACK>(); }

    [[nodiscard]] constexpr Bitboard white() const { return m_set[0]; }
    [[nodiscard]] constexpr Bitboard black() const { return m_set[1]; }

    [[nodiscard]] Bitboard& editSet(byte set) { return m_set[set]; }
    [[nodiscard]] Bitboard& editMaterial(byte pieceId) { return m_material[pieceId]; }
    
};

struct MutableImplicitPieceSquare {
    MutableImplicitPieceSquare(MaterialPositionMask& material, Square sqr) : m_material(material), m_sqr(sqr) {}

    void operator=(ChessPiece piece)
    {
        if (piece.isValid()) {
            m_material.write(squareMaskTable[static_cast<u8>(m_sqr)], piece.getSet(), piece.index());
        }
    }

private:
    MaterialPositionMask& m_material;
    Square m_sqr;
};

template<Set us>
constexpr Bitboard MaterialPositionMask::combine() const
{
    return m_set[static_cast<i8>(us)];
}

template<Set us>
void MaterialPositionMask::write(Bitboard mask, i32 pieceId)
{
    if constexpr (us == Set::WHITE) {
        m_set[0] |= mask;
    }
    else {
        m_set[1] |= mask;
    }

    m_material[pieceId] |= mask;
}

template<Set us, i32 pieceId>
void MaterialPositionMask::write(Bitboard mask)
{
    write<us>(mask, pieceId);
}

template<Set us, i32 pieceId>
Bitboard MaterialPositionMask::read() const
{
    return m_material[pieceId] & m_set[static_cast<i8>(us)];
}

template<Set us>
Bitboard MaterialPositionMask::read(i8 pieceId) const
{
    return m_material[pieceId] & m_set[static_cast<i8>(us)];
}


template<Set us>
void MaterialPositionMask::clear(Bitboard mask)
{
    if constexpr (us == Set::WHITE) {
        m_set[0] &= ~mask;
    }
    else {
        m_set[1] &= ~mask;
    }

    for (i32 i = 0; i < 6; i++) {
        m_material[i] &= ~mask;
    }
}

template<Set us>
void MaterialPositionMask::clear(Bitboard mask, i32 pieceId)
{
    if constexpr (us == Set::WHITE) {
        m_set[0] &= ~mask;
    }
    else {
        m_set[1] &= ~mask;
    }
    m_material[pieceId] &= ~mask;
}

template<Set us, i32 pieceId>
void MaterialPositionMask::clear(Bitboard mask)
{
    clear<us>(mask, pieceId);
}


template<Set us>
constexpr Bitboard MaterialPositionMask::kings() const {
    if constexpr (us == Set::WHITE) {
        return m_set[0] & m_material[kingId];
    }
    else {
        return m_set[1] & m_material[kingId];
    }
}

template<Set us>
constexpr Bitboard MaterialPositionMask::queens() const {
    if constexpr (us == Set::WHITE) {
        return m_set[0] & m_material[queenId];
    }
    else {
        return m_set[1] & m_material[queenId];
    }
}

template<Set us>
constexpr Bitboard MaterialPositionMask::rooks() const {
    if constexpr (us == Set::WHITE) {
        return m_set[0] & m_material[rookId];
    }
    else {
        return m_set[1] & m_material[rookId];
    }
}

template<Set us>
constexpr Bitboard MaterialPositionMask::bishops() const {
    if constexpr (us == Set::WHITE) {
        return m_set[0] & m_material[bishopId];
    }
    else {
        return m_set[1] & m_material[bishopId];
    }
}

template<Set us>
constexpr Bitboard MaterialPositionMask::knights() const {
    if constexpr (us == Set::WHITE) {
        return m_set[0] & m_material[knightId];
    }
    else {
        return m_set[1] & m_material[knightId];
    }
}

template<Set us>
constexpr Bitboard MaterialPositionMask::pawns() const {
    if constexpr (us == Set::WHITE) {
        return m_set[0] & m_material[pawnId];
    }
    else {
        return m_set[1] & m_material[pawnId];
    }
}