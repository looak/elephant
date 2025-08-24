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

    AccessType::chess_piece_t pieceAt(Square square) const;

    AccessType::material_t material() const { return m_position.m_materialMask; }
    AccessType::en_passant_t enPassant() const { return m_position.m_enpassantState; }
    AccessType::castling_t castling() const { return m_position.m_castlingState; }
    AccessType::hash_t hash() const { return m_position.m_hash; }

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
            return MutableImplicitPieceSquare(m_position.m_hash, m_position.m_materialMask, sqr);
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
            return m_index == other.m_index && m_position.read().hash() == other.m_position.read().hash();
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

        Square square() const { return static_cast<Square>(m_index); }
        byte file() const { return mod_by_eight(m_index); }
        byte rank() const { return m_index / 8; }

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