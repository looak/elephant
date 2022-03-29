#include "bitboard.h"
#include "chess_piece.h"
#include "notation.h"
#include "log.h"
#include <array>

#define to0x88(sqr) sqr + (sqr & ~7)
#define fr0x88(sq0x88) (sq0x88 + (sq0x88 & 7)) >> 1


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

bool Bitboard::ClearPiece(const ChessPiece& piece, const Notation& target)
{
    u64 mask = UINT64_C(1) << target.index();
	m_material[piece.set()][piece.type()] ^= mask;
	
	return true;
}

bool Bitboard::PlacePiece(const ChessPiece& piece, const Notation& target)
{
    u64 mask = UINT64_C(1) << target.index();
	m_material[piece.set()][piece.type()] |= mask;
	
	return true;
}

Bitboard::Bitboard()
{
    std::memset(&m_material[0][0], 0, sizeof(u64)*(2*6));
}

u64 Bitboard::InternalGenerateMask(byte curSqr, u64 mat, u64 opMat, signed short dir, bool sliding)
{
    u64 ret = ~universe;

    bool dirCheck = dir < INT8_MAX || dir > INT8_MIN;
    if (!dirCheck)
        LOG_ERROR() << "dir is out of bounds\n";

    do
    {
        // build a 0x88 square out of current square.
        signed char sq0x88 = to0x88(curSqr);
        // do move
        sq0x88 += dir;
        if (sq0x88 & 0x88) // validate move, are we still on the board?
        {
            sliding = false;
            break;
        }

        // convert our sqr0x88 back to a square index
        curSqr = fr0x88(sq0x88);
        // build a square mask from current square
        u64 sqrMask = UINT64_C(1) << curSqr;

        // check if we are blocked by a friendly piece or a oponent piece.
        if (mat & sqrMask) 
        {
            sliding = false;
            break;
        }
        else if (opMat & sqrMask)
        {
            sliding = false;
        }
        
        ret |= sqrMask;

    } while (sliding);

    return ret;
}

u64 Bitboard::GetAvailableMovesForPawn(u64 mat, u64 opMat, const Notation& source, const ChessPiece& piece, byte enPassant)
{
    u64 ret = ~universe;    
    u64 matComb = mat | opMat;
    // Figure out if we're moving a pawn out of it's starting position.
    // Pawn specific modifires and values, starting row to figure out if we can do a double step
    // moveModifier for allowing white & black pawn moves to be represented by one value.
    signed char startRow = 0;
    signed char moveMod = 1;

    // by default we assume our piece is black
    byte row = 6;
    moveMod = 1;

    if (piece.getSet() == PieceSet::WHITE)
    {
        row = 1;
        moveMod = -1;
    }

    signed char sq0x88 = source.index() + (source.index() & ~7);
    startRow = (sq0x88 >> 4) == row ? 0 : -1;

    // remove one move count if piece is a pawn and we are not on start row.
    byte moveCount = ChessPieceDef::MoveCount(piece.type()) + startRow;
    
    for (byte moveIndx = 0; moveIndx < moveCount; ++moveIndx)
    {        
        byte curSqr = source.index();
		signed short dir = ChessPieceDef::Moves0x88(piece.type(), moveIndx);
        dir *= moveMod;
    
        // build a 0x88 square out of current square.
        signed char sq0x88 = to0x88(curSqr);
        // do move
        sq0x88 += dir;
        if (sq0x88 & 0x88) // validate move, are we still on the board?
            break;

        // convert our sqr0x88 back to a square index
        curSqr = fr0x88(sq0x88);
        // build a square mask from current square
        u64 sqrMask = UINT64_C(1) << curSqr;

        // check if we are blocked by a friendly piece or a oponent piece.
        if (matComb & sqrMask)
            break;
        
        ret |= sqrMask;
    }

    // add attacked quares to potential moves
    if (enPassant != 0xff)
    {
        u64 enPassantMask = UINT64_C(1) << enPassant;
        opMat |= enPassantMask;
        u64 potentialAttacks = GetThreatenedSquares(source, piece);        
        ret |= (potentialAttacks & opMat);
    }

    return ret;
}

u64 Bitboard::GetAvailableMoves(const Notation& source, const ChessPiece& piece, byte castling, byte enPassant)
{
    u64 ret = ~universe;
    u64 matComb = MaterialCombined(piece.set());
    u64 opMatComb = MaterialCombined(ChessPiece::FlipSet(piece.set()));
    
    if (piece.getType() == PieceType::PAWN)
        return GetAvailableMovesForPawn(matComb, opMatComb, source, piece, enPassant);
    
    bool sliding = ChessPieceDef::Slides(piece.type());
    byte moveCount = ChessPieceDef::MoveCount(piece.type());
    for (byte moveIndx = 0; moveIndx < moveCount; ++moveIndx)
    {
        byte curSqr = source.index();
		signed short dir = ChessPieceDef::Moves0x88(piece.type(), moveIndx);

        ret |= InternalGenerateMask(curSqr, matComb, opMatComb, dir, sliding);
    }

    if (piece.getType() == PieceType::KING)
        ret |= Castling(piece.set(), castling);

    return ret;
}

u64 Bitboard::GetThreatenedSquares(const Notation& source, const ChessPiece& piece)
{
    u64 ret = ~universe;
    u64 matComb = MaterialCombined(piece.set());
    u64 opMatComb = MaterialCombined(ChessPiece::FlipSet(piece.set()));
    bool sliding = ChessPieceDef::Slides(piece.type());
    signed char moveMod = 1;
    if (piece.getSet() == PieceSet::WHITE)
        moveMod = -1;
        
    byte moveCount = ChessPieceDef::MoveCount(piece.type());
    for (byte moveIndx = 0; moveIndx < moveCount; ++moveIndx)
    {
        byte curSqr = source.index();
		signed short dir = ChessPieceDef::Attacks0x88(piece.type(), moveIndx);
        dir *= moveMod;
        ret |= InternalGenerateMask(curSqr, matComb, opMatComb, dir, sliding);
    }

    return ret;
}

u64 Bitboard::GetAttackedSquares(const Notation& source, const ChessPiece& piece)
{
    u64 opMatComb = MaterialCombined(ChessPiece::FlipSet(piece.set()));
    u64 ret = GetThreatenedSquares(source, piece);
    return ret & opMatComb;
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

bool Bitboard::IsValidMove(const Notation& source, const ChessPiece& piece, const Notation& target, byte castling, byte enPassant)
{
    u64 movesMask = GetAvailableMoves(source, piece, castling, enPassant);

    u64 targetMask = UINT64_C(1) << target.index();

    return movesMask & targetMask;
}

u64 Bitboard::Castling(byte set, byte castling)
{
   	u64 retVal = ~universe;
	byte rank = 0;
	if (set == 1) // BLACK
	{
		rank = 7;
        // shift castling right
        // this should make black caslting 1 & 2
		castling = castling >> 2;
	}

    // early out in case we don't have any castling available to us.
    if (castling == 0) 
        return retVal;
    
    u64 attacked = 0; //GetAttackedSquares(ChessPiece::FlipSet((PieceSet)set));
	u64 combMat = MaterialCombined();

    // check king side
	if (castling & 1)
    {
        // build castling square mask
        byte fsqr = (rank * 8) + 5;
        byte gsqr = fsqr + 1;
        u64 mask = ~universe;
        mask |= UINT64_C(1) << fsqr;
        mask |= UINT64_C(1) << gsqr;

        if (!(attacked & mask) && !(combMat & mask))
            retVal |= UINT64_C(1) << gsqr;
    }
    // check queen side
    if (castling & 2)
    {
        // build castling square mask
        byte bsqr = (rank * 8) + 1;
        byte csqr = bsqr + 1;
        byte dsqr = csqr + 1;
        u64 mask = ~universe;
        mask |= UINT64_C(1) << bsqr;
        mask |= UINT64_C(1) << csqr;
        mask |= UINT64_C(1) << dsqr;

        if (!(attacked & mask) && !(combMat & mask))
            retVal |= UINT64_C(1) << csqr;
    }
    return retVal;
}

u64 Bitboard::MaterialCombined()
{
    return MaterialCombined(0) | MaterialCombined(1);
}

u64 Bitboard::MaterialCombined(byte set)
{
    u64 combMaterialMask = 0;
    for (u64 msk : m_material[set])
    {
        combMaterialMask |= msk;
    }
    return combMaterialMask;
}
