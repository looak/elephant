#include <position/position_proxy.hpp>
#include <position/position.hpp>

#include <position/hash_zorbist.hpp>
template<typename AccessType>
template<bool validation>
bool PositionProxy<AccessType>::placePiece(Piece piece, Square square) {
    if constexpr (std::is_same_v<AccessType, PositionEditPolicy>) {
        if constexpr (validation) {
            if (pieceAt(square).isValid()) {
                LOG_WARNING() << "Trying to place piece " << piece.toString() << " at square " << square << " but it's already occupied by " << pieceAt(square).toString();
                return false;
            }
            if (!piece.isValid()) {
                LOG_WARNING() << "Trying to place an invalid piece at square " << square;
                return false;
            }
        }
        auto material = m_position.editMaterialMask();
        material.editSet(piece.set())[square] = true;
        material.editMaterial(piece.index())[square] = true;

        hash() = ZorbistHash::Instance().HashPiecePlacement(hash(), piece, Notation(square));
        
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
        if constexpr (validation) {
            if (!pieceAt(square).isValid()) {
                LOG_WARNING() << "Trying to clear piece at square " << square << " but it's already empty.";
                return false;
            }
            if (pieceAt(square).isValid() && pieceAt(square).isKing()) {
                LOG_WARNING() << "Trying to clear king piece at square " << square << ", this is not allowed.";
                return false;
            }
        }
        auto material = m_position.editMaterialMask();
        material.editSet(pieceAt(square).set())[square] = false;
        material.editMaterial(pieceAt(square).index())[square] = false;
        hash() = ZorbistHash::Instance().HashPiecePlacement(hash(), ChessPiece::None(), Notation(square));
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
AccessType::chess_piece_t PositionProxy<AccessType>::pieceAt(Square sqr) const
{
    Bitboard mask(UINT64_C(1) << (u8)sqr);
 
    for (byte set = 0; set < 2; ++set) {
        if (material().set(set) & mask)
        {
            // unrolled this for loop in an attempt to make it quicker.
            if (material().pawns() & mask)
                return piece_constants::pieces[set][pawnId];
            else if (material().knights() & mask)
                return piece_constants::pieces[set][knightId];
            else if (material().bishops() & mask)
                return piece_constants::pieces[set][bishopId];
            else if (material().rooks() & mask)
                return piece_constants::pieces[set][rookId];
            else if (material().queens() & mask)
                return piece_constants::pieces[set][queenId];
            else if (material().kings() & mask)
                return piece_constants::pieces[set][kingId];
        }
    }
    return ChessPiece::None();
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

template PositionEditPolicy::chess_piece_t PositionProxy<PositionEditPolicy>::pieceAt(Square) const;
template PositionReadOnlyPolicy::chess_piece_t PositionProxy<PositionReadOnlyPolicy>::pieceAt(Square) const;