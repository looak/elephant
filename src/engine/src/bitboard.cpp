#include "bitboard.h"
#include "chess_piece.h"
#include "notation.h"
#include "log.h"
#include <array>


bool Bitboard::IsValidSquare(signed short currSqr)
{
    if (currSqr < 0)
        return false;
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

u64 Bitboard::GetAvailableMoves(const Notation& source, const ChessPiece& piece)
{
    u64 ret = ~universe;

    // Figure out if we're moving a pawn out of it's starting position.
    // Pawn specific modifires and values, starting row to figure out if we can do a double step
    // moveModifier for allowing white & black pawn moves to be represented by one value.
    signed char startRow = 0;
    signed char moveMod = 1;

    if (piece.getType() == PieceType::PAWN)
    {
        // by default we assume our piece is black
        byte row = 96 >> 4;
        moveMod = 1;

        if (piece.getSet() == PieceSet::WHITE)
        {
            row = 16 >> 4;
            moveMod = -1;
        }

		signed char sq0x88 = source.index() + (source.index() & ~7);
        startRow = (sq0x88 >> 4) == row ? 0 : -1;
    }

    // remove one move count if piece is a pawn and we are not on start row.
    byte moveCount = ChessPieceDef::MoveCount(piece.type()) + startRow;
    
    for (byte moveIndx = 0; moveIndx < moveCount; ++moveIndx)
    {
        byte curSqr = source.index();
        bool sliding = ChessPieceDef::Slides(piece.type());
		signed short dir = ChessPieceDef::Moves0x88(piece.type(), moveIndx);
        dir *= moveMod;
        bool dirCheck = dir < INT8_MAX || dir > INT8_MIN;
        if (!dirCheck)
		    LOG_ERROR() << "dir is out of bounds\n";

        do
		{
            // build a 0x88 square out of current square.
			signed char sq0x88 = curSqr + (curSqr & ~7);
            // do move
			sq0x88 += dir;
            if (sq0x88 & 0x88) // validate move, are we still on the board?
            {
                sliding = false;
                break;
            }

            // convert our sqr0x88 back to a square index
			curSqr = (sq0x88 + (sq0x88 & 7)) >> 1;
            // build a square mask from current square
			u64 sqrMask = UINT64_C(1) << curSqr;

			// if (matComb & sqrMask) 
			// 	sliding = false;
			// if (matCombOp & sqrMask)
			// {
			// 	sliding = false;
			// }
			
            ret |= sqrMask;

		} while (sliding);
    }

    return ret;
}

u64 Bitboard::GetAttackedSquares(const Notation& source, const ChessPiece& piece)
{
    u64 ret = ~universe;

    return ret;
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

    return false;
}

bool Bitboard::IsValidMove(const Notation& source, const ChessPiece& piece, const Notation& target)
{
    //byte currSqr = source.index();

    u64 srcMask = UINT64_C(1) << source.index();
	if ((m_material[piece.set()][piece.type()] & srcMask) == 0)
        return false; // no piece at src square

    //u64 trgMask = UINT64_C(1) << target.index();

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
    

    return false;
}
