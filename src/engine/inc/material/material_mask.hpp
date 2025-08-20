#pragma once
#include <bitboard/bitboard.hpp>
#include <material/chess_piece.hpp>
#include <material/material_topology.hpp>

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

/**
 * @brief A proxy for material that allows editing of the material bitboards.
 * Because the material bitboards are split into two parts (set and material), this is a helper to allow easy access to
 * the material and read or edit both bitboards with a single operation. */
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

    template<Set us> [[nodiscard]] constexpr Bitboard king() const;
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

    [[nodiscard]] constexpr Bitboard whiteKing() const { return king<Set::WHITE>(); }
    [[nodiscard]] constexpr Bitboard blackKing() const { return king<Set::BLACK>(); }
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

    [[nodiscard]] constexpr Bitboard set(byte set) const {
        FATAL_ASSERT(set < 2) << "Invalid set index: " << (int)set;
        return m_set[set];
    }
    [[nodiscard]] constexpr Bitboard white() const { return m_set[0]; }
    [[nodiscard]] constexpr Bitboard black() const { return m_set[1]; }

    [[nodiscard]] Bitboard& editSet(byte set) { return m_set[set]; }
    [[nodiscard]] Bitboard& editMaterial(byte pieceId) { return m_material[pieceId]; }

    template<Set us>
    MaterialTopology<us> topology() const {
        return MaterialTopology<us>(*this);
    }

    [[nodiscard]] constexpr bool operator==(const MaterialPositionMask& other) const {
        return m_set[0] == other.m_set[0]
         && m_set[1] == other.m_set[1]
         && m_material[0] == other.m_material[0]
         && m_material[1] == other.m_material[1]
         && m_material[2] == other.m_material[2]
         && m_material[3] == other.m_material[3]
         && m_material[4] == other.m_material[4]
         && m_material[5] == other.m_material[5];
    }

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
constexpr Bitboard MaterialPositionMask::king() const {
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