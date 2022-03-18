#include "bitboard.h"
#include "chess_piece.h"
#include "notation.h"
#include "log.h"


bool Bitboard::IsValidSquare(byte currSqr)
{
    if (currSqr > 127)
        LOG_ERROR() << "In case index is larger than 127 it will wrap around our board.";
    byte sq0x88 = currSqr + (currSqr & ~7);
    return (sq0x88 & 0x88) == 0;
}

bool Bitboard::IsValidSquare(const Notation& source)
{ 
    return Bitboard::IsValidSquare(source.index());
}

bool Bitboard::PlacePiece(const ChessPiece& piece, const Notation& target)
{
    u64 mask = UINT64_C(1) << target.index();
	m_material[piece.set()][piece.type()] |= mask;
	
	return true;
}

bool Bitboard::IsValidPawnMove(byte srcSqr, byte trgSqr, byte set)
{
    // move modifier to help us determine given move is valid for set.
    int8_t moveModifier = 1;
    if (set == (byte)PieceSet::WHITE)
    {
        moveModifier = -1;
    }
    
    if (!IsValidSquare(trgSqr))
        return false;
}

bool Bitboard::IsValidMove(const Notation& source, const ChessPiece& piece, const Notation& target)
{
    //byte currSqr = source.index();

    u64 srcMask = UINT64_C(1) << source.index();
	if ((m_material[piece.set()][piece.type()] & srcMask) == 0)
        return false; // no piece at src square

    u64 trgMask = UINT64_C(1) << target.index();

    // combine material into one mask to check if we're blocking ourselves.
    u64 combMaterialMask = 0;
    for (u64 msk : m_material[piece.set()])
    {
        combMaterialMask |= msk;
    }

	if ((combMaterialMask & srcMask) == 0)
        return false; // target square is blocked by our own pieces

    // byte pIndex = piece.type() - 1;
    // byte mCount = ChessPieceDef::MoveCount(pIndex);
    
//    byte sq0x88 = currSqr + (currSqr & (byte)~7);

    // board0x88 out of bounds check.
    //ChessPieceDef::Moves0x88(pIndex, )
    

    return true;
}
