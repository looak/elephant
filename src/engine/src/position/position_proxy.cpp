#include <position/position_proxy.hpp>
#include <position/position.hpp>

#include <position/hash_zobrist.hpp>
template<typename AccessType>
template<bool validation>
bool PositionProxy<AccessType>::placePiece(Piece piece, Square square) {
    if constexpr (std::is_same_v<AccessType, PositionEditPolicy>) {
        if constexpr (validation) {
            if (pieceAt(square).isValid()) {
                LOG_WARNING() << "Trying to place piece " << piece.toString() << " at square " << SquareNotation(square).toString() << " but it's already occupied by " << pieceAt(square).toString();
                return false;
            }
            if (!piece.isValid()) {
                LOG_WARNING() << "Trying to place an invalid piece at square " << SquareNotation(square).toString();
                return false;
            }
        }
        auto& material = m_position.m_materialMask;
        material.editSet(piece.set())[square] = true;
        material.editMaterial(piece.index())[square] = true;

        hash() = zobrist::updatePieceHash(hash(), piece, square);
        
        return true;      
    }
    else {
        static_assert(false, "Cannot call placePiece() on a read-only policy position.");
    }

    return false;
}


template bool PositionProxy<PositionEditPolicy>::placePiece<true>(Piece, Square);
template bool PositionProxy<PositionEditPolicy>::placePiece<false>(Piece, Square);
// template bool PositionProxy<PositionReadOnlyPolicy>::placePiece<true>(Piece, Square); // This will static assert if used.
// template bool PositionProxy<PositionReadOnlyPolicy>::placePiece<false>(Piece, Square); // This will static assert if used.

template<typename AccessType>
template<bool validation>
bool PositionProxy<AccessType>::clearPiece(Square square) {
    if constexpr (std::is_same_v<AccessType, PositionEditPolicy>) {
        auto targetPiece = pieceAt(square);
        if constexpr (validation) {
            if (!targetPiece.isValid()) {
                LOG_WARNING() << "Trying to clear piece at square " << SquareNotation(square).toString() << " but it's already empty.";
                return false;
            }
            if (targetPiece.isValid() && targetPiece.isKing()) {
                LOG_WARNING() << "Trying to clear king piece at square " << SquareNotation(square).toString() << ", this is not allowed.";
                return false;
            }
        }
        auto& material = m_position.m_materialMask;
        material.editSet(targetPiece.set())[square] = false;
        material.editMaterial(targetPiece.index())[square] = false;
        hash() = zobrist::updatePieceHash(hash(), targetPiece, square);
        return true;
    }
    else {
        static_assert(false, "Cannot call clearPiece() on a read-only policy position.");
    }
    return false;
}

template bool PositionProxy<PositionEditPolicy>::clearPiece<true>(Square);
template bool PositionProxy<PositionEditPolicy>::clearPiece<false>(Square);
// template bool PositionProxy<PositionReadOnlyPolicy>::clearPiece<true>(Square); // This will static assert if used.
// template bool PositionProxy<PositionReadOnlyPolicy>::clearPiece<false>(Square); // This will static assert if used.

template<typename AccessType>
AccessType::chess_piece_t PositionProxy<AccessType>::pieceAt(Square sqr) const {
    return m_position.m_materialMask.pieceAt(sqr);
}

template PositionEditPolicy::chess_piece_t PositionProxy<PositionEditPolicy>::pieceAt(Square) const;
template PositionReadOnlyPolicy::chess_piece_t PositionProxy<PositionReadOnlyPolicy>::pieceAt(Square) const;

template<typename AccessType>
void PositionProxy<AccessType>::clear() {
    if constexpr (std::is_same_v<AccessType, PositionEditPolicy>) {
        m_position.m_materialMask.clear();
        m_position.m_castlingState = {};
        m_position.m_enpassantState = {};
        m_position.m_hash = 0;
    }
    else
    {
        static_assert(false, "Cannot call clear() on a read-only policy position.");
    }
}

template void PositionProxy<PositionEditPolicy>::clear();


template<typename AccessType>
Position PositionProxy<AccessType>::copy() const {
    return m_position; 
}

template Position PositionProxy<PositionReadOnlyPolicy>::copy() const;
template Position PositionProxy<PositionEditPolicy>::copy() const;