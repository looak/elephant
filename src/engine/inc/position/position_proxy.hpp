// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021-2023  Alexander Loodin Ek

// This program is free software : you can redistribute it and /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.If not, see < http://www.gnu.org/licenses/>.

/**
 * @file position_editor.hpp
 * @brief class for updating the position of the board. has helpers for placing pieces
 * and making moves.
 */

#pragma once
#include <material/chess_piece.hpp>
#include <eval/pesto_accumulator.hpp>
#include <material/material_mask.hpp>
#include <move/move.hpp>
#include <position/en_passant_state_info.hpp>
#include <position/castling_state_info.hpp>
#include <position/position_access_policies.hpp>
#include <vector>

typedef ChessPiece Piece;

class Position;
struct PositionReadOnlyPolicy;
struct PositionEditPolicy;

// Assigning a piece to a square through PositionEditor::operator[], replaces whatever was on it.
struct MutableImplicitPieceSquare {
    MutableImplicitPieceSquare(u64& hash, MaterialPositionMask& material, PestoAccumulator& pesto, Square sqr) :
        m_hash(hash),
        m_material(material),
        m_pesto(pesto),
        m_sqr(sqr)
    {
    }

    void operator=(ChessPiece piece)
    {
        if (piece.isValid()) {
            ChessPiece oldPiece = m_material.pieceAt(m_sqr);
            if (oldPiece.isValid()) {
                m_material.clear(squareMaskTable[static_cast<u8>(m_sqr)], oldPiece.getSet(), oldPiece.index());
                m_hash = zobrist::updatePieceHash(m_hash, oldPiece, m_sqr);
                m_pesto.remove(oldPiece, m_sqr);
            }
            m_material.write(squareMaskTable[static_cast<u8>(m_sqr)], piece.getSet(), piece.index());
            m_hash = zobrist::updatePieceHash(m_hash, piece, m_sqr);
            m_pesto.add(piece, m_sqr);
        }
    }

private:
    u64& m_hash;
    MaterialPositionMask& m_material;
    PestoAccumulator& m_pesto;
    Square m_sqr;
};

template<typename AccessType>
class PositionProxy {
private:
    template<typename> friend class PositionProxy;
    typename AccessType::position_t m_position;

public:    
    PositionProxy(AccessType::position_t& position) : m_position(position) {}
    PositionProxy(const PositionProxy& other) : m_position(other.m_position) {}

    /**
     * allow a PositionProxy<PositionEditPolicy> to be implicitly converted to a PositionProxy<PositionReadOnlyPolicy>.    */
    template<typename A = AccessType>
        requires std::is_same_v<A, PositionReadOnlyPolicy>
    PositionProxy(const PositionProxy<PositionEditPolicy>& other)
        : m_position(other.m_position) {}

    /**
     * allow a PositionProxy<PositionEditPolicy> to be explicity converted to a PositionProxy<PositionReadOnlyPolicy>.     */
    template<typename A = AccessType>
        requires std::is_same_v<A, PositionEditPolicy>
    PositionProxy<PositionReadOnlyPolicy> asReader() const {    
        return PositionProxy<PositionReadOnlyPolicy>(m_position);
    }

    void clear();
    bool empty() const { return material().empty(); }
    Position copy() const;   
    
    /**
     * @brief Places multiple pieces on the board, pairs of <ChessPiece>, <Square>
     * @param placements: ChessPiece, Square      */
    template<typename... placementpairs>
    bool placePieces(placementpairs... placements);

    template<bool validation = false>
    bool placePiece(Piece piece, Square square); 
    
    template<bool validation = false>
    bool clearPiece(Square square);

    // Clears a piece the caller already knows is on square, skips looking it up. Used by unmake.
    void clearPiece(Piece piece, Square square);

    // Restores en passant, castling & hash from an undo. Writes the raw states, hashing them is wasted work since the
    // stored hash overwrites it anyway.
    void restoreState(byte enPassantState, byte castlingState, u64 hash) {
        static_assert(std::is_same_v<AccessType, PositionEditPolicy>, "Cannot call restoreState() on a read-only policy position.");
        m_position.m_enpassantState.write(enPassantState);
        m_position.m_castlingState.write(castlingState);
        m_position.m_hash = hash;
    }

    /**
     * Incremental evaluation hooks, every piece entering or leaving a square is reported here once the material
     * bitboards are updated. Keeps the PeSTO accumulator current, the place to hook in any other incremental eval.  */
    void pieceAdded(Piece piece, Square square) {
        static_assert(std::is_same_v<AccessType, PositionEditPolicy>, "Cannot call pieceAdded() on a read-only policy position.");
        m_position.m_pesto.add(piece, square);
    }
    void pieceRemoved(Piece piece, Square square) {
        static_assert(std::is_same_v<AccessType, PositionEditPolicy>, "Cannot call pieceRemoved() on a read-only policy position.");
        m_position.m_pesto.remove(piece, square);
    }

    const PestoAccumulator& pesto() const { return m_position.m_pesto; }

    AccessType::chess_piece_t pieceAt(Square square) const;

    AccessType::material_t material() const { return m_position.m_materialMask; }
    AccessType::hash_t hash() const { return m_position.m_hash; }

    AccessType::en_passant_t enPassant() const { 
        if constexpr (std::is_same_v<AccessType, PositionEditPolicy>) {
            return EnPassantStateProxy(m_position.m_enpassantState, m_position.m_hash);
        }
        else {
            return m_position.m_enpassantState;
        }
    }

    AccessType::castling_t castling() const { 
        if constexpr (std::is_same_v<AccessType, PositionEditPolicy>) {
            return CastlingStateProxy(m_position.m_castlingState, m_position.m_hash);
        }
        else {
            return m_position.m_castlingState;
        }
    }

    MutableMaterialProxy materialEditor(Set set, PieceType type)
    {
        if constexpr (std::is_same_v<AccessType, PositionEditPolicy>) {
            return MutableMaterialProxy(&material().editSet(toSetId(set)), 
                                        &material().editMaterial(toPieceIndex(type)));
        }
        else {
            static_assert(false, "Cannot call materialEditor() on a position with a read-only policy.");
        }
    }

    ChessPiece operator[](Square sqr) const {
        return pieceAt(sqr);
    }

    MutableImplicitPieceSquare operator[](Square sqr) 
    {
        if constexpr (std::is_same_v<AccessType, PositionEditPolicy>) {
            return MutableImplicitPieceSquare(m_position.m_hash, m_position.m_materialMask, m_position.m_pesto, sqr);
        }
        else {
            static_assert(false, "Cannot call and modify position with operator[] on a position with a read-only policy.");
        }
    }

    class PositionIterator {
    public:
        PositionIterator(AccessType::position_t& position, byte index = 0)
            : m_position(position), m_index(index) {}
        PositionIterator(const PositionIterator& other)
            : m_position(other.m_position), m_index(other.m_index) {}
        PositionIterator& operator=(const PositionIterator& other) {
            if (this != &other) {
                m_position = other.m_position;
                m_index = other.m_index;
            }
            return *this;
        }
        bool operator==(const PositionIterator& other) const {
            return m_index == other.m_index && &m_position == &other.m_position;
        }
        bool operator!=(const PositionIterator& other) const {
            return !(*this == other);
        }

        bool end() const {
            return m_index >= 64;  // assuming 64 squares on the board
        }
        PositionIterator& operator++() {
            ++m_index;
            return *this;
        }
        PositionIterator operator++(int) {
            PositionIterator temp = *this;
            ++(*this);
            return temp;
        }
        PositionIterator& operator+=(int incre) {
            m_index += incre;
            return *this;
        }

        byte index() const { return m_index; }
        Square square() const { return static_cast<Square>(m_index); }
        byte file() const { return to_file(square()); }
        byte rank() const { return to_rank(square()); }

        ChessPiece get() const {
            return m_position.read().pieceAt(static_cast<Square>(m_index));
        }

        void set(ChessPiece piece) 
        {
            if constexpr (std::is_same_v<AccessType, PositionEditPolicy>) {
                auto currentPiece = get();
                if (currentPiece.isValid()) {                    
                    auto materialEditor = m_position.m_materialMask.edit(currentPiece.getSet(), currentPiece.getType());
                    materialEditor[square()] = false;  // remove the current piece
                }
                auto materialEditor = m_position.m_materialMask.edit(piece.getSet(), piece.getType());
                materialEditor[square()] = true;
            }
            else {
                static_assert(false, "Cannot call set() on a position with a read-only policy.");
            }
        }

    private:
        AccessType::position_t& m_position;
        byte m_index;            
    };

    PositionIterator begin() {
        return PositionIterator(m_position, 0);
    }
    PositionIterator end() {
        return PositionIterator(m_position, 64);
    }
    PositionIterator begin() const {
        return PositionIterator(m_position, 0);
    }
    PositionIterator end() const {
        return PositionIterator(m_position, 64);
    }

private:
    template<typename piece, typename square, typename... placements>
    bool internalUnrollPlacementPairs(const piece& p, const square& sqr, const placements&... _placements);
    bool internalUnrollPlacementPairs() { return true; }

};

template<typename AccessType>
template<typename piece, typename square, typename... placements>
bool PositionProxy<AccessType>::internalUnrollPlacementPairs(const piece& p, const square& sqr, const placements&... _placements) 
{
    if (placePiece(p, sqr) == false)
        return false;

    return internalUnrollPlacementPairs(_placements...);
}

template<typename AccessType>
template<typename... placements>
bool PositionProxy<AccessType>::placePieces(placements... _placement) 
{
    if constexpr (std::is_same_v<AccessType, PositionEditPolicy>) {
        static_assert(sizeof...(_placement) % 2 == 0, "Number of arguments must be even");
        return internalUnrollPlacementPairs(_placement...);
    }
    else {
        static_assert(false, "Cannot call placePieces() on a read-only policy position.");
    }
}