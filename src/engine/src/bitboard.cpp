#include "bitboard.h"
#include "chess_piece.h"
#include "notation.h"
#include "log.h"
#include <array>

#define to0x88(sqr) sqr + (sqr & ~7)
#define fr0x88(sq0x88) (sq0x88 + (sq0x88 & 7)) >> 1

Bitboard::Bitboard()
{
    std::memset(&m_material[0][0], 0, sizeof(u64) * 12);
}

Bitboard& Bitboard::operator=(const Bitboard& other)
{
    auto size = sizeof(m_material);
    std::memset(&m_material[0][0], 0, size);
    std::memcpy(&m_material[0][0], &other.m_material[0][0], size);
    return *this;
}

bool Bitboard::IsValidSquare(signed short currSqr)
{
    if (currSqr < 0)
        return false;
    if (currSqr > 127)
    {
        // LOG_ERROR() << "In case index is larger than 127 it will wrap around our board.";
        return false;
    }

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
    u8 pieceIndx = piece.type() - 1;
	m_material[piece.set()][pieceIndx] ^= mask;
	
	return true;
}

bool Bitboard::PlacePiece(const ChessPiece& piece, const Notation& target)
{
    u64 mask = UINT64_C(1) << target.index();
    u8 pieceIndx = piece.type() - 1;
	m_material[piece.set()][pieceIndx] |= mask;
	
	return true;
}

u64 Bitboard::InternalGenerateMask(byte curSqr, signed short dir, bool& sliding, ResolveMask resolveSquare, Validate valid) const
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
        if (!resolveSquare(sqrMask))
            break;
      
        ret |= valid(sqrMask);

    } while (sliding);

    return ret;
}

u64 Bitboard::GetAvailableMovesForPawn(u64 mat, u64 opMat, const Notation& source, const ChessPiece& piece, byte enPassant, u64 threatenedMask, bool checked, u64 kingMask) const
{
    u64 ret = ~universe;    
    u64 matComb = mat | opMat;

    // figure out if we're pinned
    u64 sqrMask = UINT64_C(1) << source.index();    
    bool pinned = (sqrMask & kingMask);
    if (checked || pinned)
    {
        threatenedMask &= kingMask;
        threatenedMask |= (opMat & kingMask);
    }

    // Figure out if we're moving a pawn out of it's starting position.
    // Pawn specific modifires and values, starting row to figure out if we can do a double step
    // moveModifier for allowing white & black pawn moves to be represented by one value.
    signed char startRow = 0;
    signed char moveMod = 1;

    // by default we assume our piece is black
    byte row = 6;
    moveMod = 1;

    if (piece.getSet() == Set::WHITE)
    {
        row = 1;
        moveMod = -1;
    }

    signed char sq0x88 = source.index() + (source.index() & ~7);
    startRow = (sq0x88 >> 4) == row ? 0 : -1;

    // remove one move count if piece is a pawn and we are not on start row.
    byte moveCount = ChessPieceDef::MoveCount(piece.type()) + startRow;

    auto resolve = [&](u64 sqrMask)
    {
        if (matComb & sqrMask)
            return false;
        
        return true;
    };
    
    for (byte moveIndx = 0; moveIndx < moveCount; ++moveIndx)
    {        
        byte curSqr = source.index();
		signed short dir = ChessPieceDef::Moves0x88(piece.type(), moveIndx);
        dir *= moveMod;

        bool sliding = false;
        
        // check if we are blocked by a friendly piece or a oponent piece.      
        ret |= InternalGenerateMask(curSqr, dir, sliding, resolve);
        
        if (ret == 0) // we've run into a piece, i.e. we can't do double move.
            break;
    }

    // add attacked squares to potential moves
    if (enPassant != 0xff)
    {
        u64 enPassantMask = UINT64_C(1) << enPassant;
        opMat |= enPassantMask;
    }

    u64 potentialAttacks = GetThreatenedSquares(source, piece);
    ret |= (potentialAttacks & opMat);

    if (checked || pinned)
        ret &= threatenedMask;

    return ret;
}

/// <summary>
/// From Kings position, we look at all directions until we hit end of board and see if 
/// we run into a piece wich is threatening the king or pinning another piece
/// </summary>
/// <param name="king"></param>
/// <param name="target"></param>
/// <returns></returns>
u64 Bitboard::GetKingMask(const ChessPiece& king, const Notation& target, u64 opponentSlidingMask) const
{
    byte moveCount = ChessPieceDef::MoveCount(king.type());

    u8 opSet = ChessPiece::FlipSet(king.set());
    u64 slideMat = SlidingMaterialCombined(opSet);
    u64 allMat = MaterialCombined();
    u64 knightMat = m_material[opSet][1];
    u64 ret = ~universe;
    u8 checks = 0;
    
    if (slideMat > 0)
    {
        u8 matCount = 0;
        bool sliding = true;
        auto resolve = [&](u64 sqrMask)
        {
            if (allMat & sqrMask)
                matCount++;
            
            if (slideMat & sqrMask)
            {
                sliding = false;
            }
            return true;
        };

        for (byte moveIndx = 0; moveIndx < moveCount; ++moveIndx)
        {
            matCount = 0;
            sliding = true;
            byte curSqr = target.index();
            signed short dir = ChessPieceDef::Moves0x88(king.type(), moveIndx);

            u64 mvMask = InternalGenerateMask(curSqr, dir, sliding, resolve);
            // comparing agiainst two here since we'll find the sliding piece causing the pin
            // and at least one piece in between our king and this piece. This found piece isn't
            // necessarily pinned, but if there are any more pieces between king and sliding piece
            // they won't be pinned.
            if (mvMask & slideMat && matCount <= 2)
            {
                ret |= mvMask;
                if (matCount == 1)
                    checks++;
            }
        }
        // checking the sliding pieces we've run into are the same diagnoals
        // as we've identified them being threatning.
        // this is to avoid false positives for bishops in straight columns or rows.
        ret &= opponentSlidingMask;
    }

    if (knightMat > 0)
    {
        // figure out if we're checked by a knight
        bool sliding = false;
        moveCount = ChessPieceDef::MoveCount((byte)PieceType::KNIGHT);
        
        auto nResolve = [&](u64 sqrMask)
        {
            return true;
        };

        for (byte moveIndx = 0; moveIndx < moveCount; ++moveIndx)
        {
            
            byte curSqr = target.index();
            signed short dir = ChessPieceDef::Moves0x88((byte)PieceType::KNIGHT, moveIndx);

            u64 mvMask = InternalGenerateMask(curSqr, dir, sliding, nResolve);
            if (mvMask & knightMat)
            {
                ret |= mvMask;
                checks++;
            }
        }
    }

    if (m_material[opSet][0] > 0)
    {
        // figure out if we're checked by a pawn
        s8 pawnMod = king.getSet() == Set::WHITE ? 1 : -1;
        auto pawnSqr = Notation(target.file + 1, target.rank + pawnMod);
        if (Bitboard::IsValidSquare(pawnSqr))
        {
            u64 sqrMak = UINT64_C(1) << pawnSqr.index();
            sqrMak &= m_material[opSet][0];
            if (sqrMak > 0)
            {
                ret |= sqrMak;
                checks++;
            }
        }
        pawnSqr = Notation(target.file - 1, target.rank + pawnMod);
        if (Bitboard::IsValidSquare(pawnSqr))
        {
            u64 sqrMak = UINT64_C(1) << pawnSqr.index();
            sqrMak &= m_material[opSet][0];            
            if (sqrMak > 0)
            {
                ret |= sqrMak;
                checks++;
            }
        }
    }

    if (checks > 1)
        return 0;
    return ret;
}

u64 Bitboard::GetAvailableMovesForKing(u64 mat, u64 threatenedMask, const Notation& source, const ChessPiece& piece, byte castling) const
{
    u64 ret = ~universe;

    byte moveCount = ChessPieceDef::MoveCount(piece.type());

    auto resolve = [&](u64 sqrMask) {
        // check if we are blocked by a friendly piece or a oponent piece.
        if (mat & sqrMask) 
            return false;

        else if (threatenedMask & sqrMask)
            return false;

        return true;
    };
    
    for (byte moveIndx = 0; moveIndx < moveCount; ++moveIndx)
    {
        byte curSqr = source.index();
		signed short dir = ChessPieceDef::Moves0x88(piece.type(), moveIndx);

        bool sliding = false;
        ret |= InternalGenerateMask(curSqr, dir, sliding, resolve);
    }
    
    ret |= Castling(piece.set(), castling);

    return ret;
}

u64 Bitboard::GetAvailableMoves(const Notation& source, const ChessPiece& piece, byte castling, byte enPassant, u64 threatened, bool checked, u64 kingMask) const
{
    u64 ret = ~universe;
    u64 matComb = MaterialCombined(piece.set());
    u64 opMatComb = MaterialCombined(ChessPiece::FlipSet(piece.set()));
    
    if (piece.getType() == PieceType::PAWN)
        return GetAvailableMovesForPawn(matComb, opMatComb, source, piece, enPassant, threatened, checked, kingMask);
    else if(piece.getType() == PieceType::KING)
        return GetAvailableMovesForKing(matComb, threatened, source, piece, castling);

    // figure out if we're pinned
    u64 sqrMask = UINT64_C(1) << source.index();    
    bool pinned = (sqrMask & kingMask);
    if (checked || pinned)
    {
        threatened &= kingMask;
        threatened |= (opMatComb & kingMask);
    }

    bool sliding = false;
    auto resolve = [&](u64 sqrMask)
    {
        if (matComb & sqrMask) 
        {
            sliding = false;
            return false;
        }
        else if (opMatComb & sqrMask)
        {
            sliding = false;
            return true;
        }

        return true;
    };

    auto validChecked = [&](u64 sqrMask)
    {
        sqrMask &= threatened;
        return sqrMask;            
    };

    byte moveCount = ChessPieceDef::MoveCount(piece.type());
    for (byte moveIndx = 0; moveIndx < moveCount; ++moveIndx)
    {
        sliding = ChessPieceDef::Slides(piece.type());
        byte curSqr = source.index();
		signed short dir = ChessPieceDef::Moves0x88(piece.type(), moveIndx);

        u64 mvMask = 0;
        if (checked || pinned)
            mvMask = InternalGenerateMask(curSqr, dir, sliding, resolve, validChecked);
        else
            mvMask = InternalGenerateMask(curSqr, dir, sliding, resolve);       

        ret |= mvMask;
    }

    return ret;
}

u64 Bitboard::GetThreatenedSquares(const Notation& source, const ChessPiece& piece) const
{
    u64 ret = ~universe;
    u64 matComb = MaterialCombined(piece.set());
    u64 opMatComb = MaterialCombined(ChessPiece::FlipSet(piece.set()));
    signed char moveMod = 1;
    if (piece.getSet() == Set::WHITE)
        moveMod = -1;
    
    bool sliding = false;
    auto resolve = [&](u64 sqrMask)
    {
        // if i collide with my own piece I stop sliding but I'm guarding it.
        if (matComb & sqrMask) 
        {
            sliding = false;
            return true;
        }
        // if I collide with oponent piece it is threatened and I stop slding.
        else if (opMatComb & sqrMask)
        {
            sliding = false;
            return true;
        }

        return true;
    };

    byte moveCount = ChessPieceDef::MoveCount(piece.type());
    for (byte moveIndx = 0; moveIndx < moveCount; ++moveIndx)
    {
        sliding = ChessPieceDef::Slides(piece.type());
        byte curSqr = source.index();
		signed short dir = ChessPieceDef::Attacks0x88(piece.type(), moveIndx);
        dir *= moveMod;
        ret |= InternalGenerateMask(curSqr, dir, sliding, resolve);
    }

    return ret;
}

u64 Bitboard::GetAttackedSquares(const Notation& source, const ChessPiece& piece) const
{
    u64 opMatComb = MaterialCombined(ChessPiece::FlipSet(piece.set()));
    u64 ret = GetThreatenedSquares(source, piece);
    return ret & opMatComb;
}

bool Bitboard::IsValidPawnMove(byte srcSqr, byte trgSqr, byte set)
{
    // move modifier to help us determine given move is valid for set.
    int8_t moveModifier = 1;
    if (set == (byte)Set::WHITE)
    {
        moveModifier = -1;
    }
    
    if (!IsValidSquare(trgSqr))
        return false;

    return false;
}

bool Bitboard::IsValidMove(const Notation& source, const ChessPiece& piece, const Notation& target, byte castling, byte enPassant, u64 threatenedMask) const
{
    u64 movesMask = GetAvailableMoves(source, piece, castling, enPassant, threatenedMask);

    u64 targetMask = UINT64_C(1) << target.index();

    return movesMask & targetMask;
}

u64 Bitboard::Castling(byte set, byte castling) const
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

u64 Bitboard::GetMaterialCombined(Set set) const
{
    return MaterialCombined(static_cast<byte>(set));
}

u64 Bitboard::MaterialCombined() const
{
    return MaterialCombined(0) | MaterialCombined(1);
}

u64 Bitboard::MaterialCombined(byte set) const
{
    u64 combMaterialMask = 0;
    for (u64 msk : m_material[set])
    {
        combMaterialMask |= msk;
    }
    return combMaterialMask;
}

u64 Bitboard::SlidingMaterialCombined(byte set) const
{
    u8 bishop = 2;
    u8 rook = 3;
    u8 queen = 4;
    return m_material[set][bishop] | m_material[set][rook] | m_material[set][queen];
}