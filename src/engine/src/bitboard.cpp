#include "bitboard.h"
#include "bitboard_constants.h"
#include "chess_piece.h"
#include "notation.h"
#include "log.h"
#include <array>

#define to0x88(sqr) sqr + (sqr & ~7)
#define fr0x88(sq0x88) (sq0x88 + (sq0x88 & 7)) >> 1

Bitboard::Bitboard()
{
    std::memset(&m_material[0], 0, sizeof(u64) * 12);
}

Bitboard& Bitboard::operator=(const Bitboard& other)
{
    auto size = sizeof(m_material);
    std::memset(&m_material[0], 0, size);
    std::memcpy(&m_material[0], &other.m_material[0], size);
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

bool Bitboard::IsValidSquare(Notation source)
{ 
    return Bitboard::IsValidSquare(source.index());
}

void Bitboard::Clear()
{
	std::memset(&m_material[0], 0, sizeof(u64) * 12);
}

bool Bitboard::ClearPiece(ChessPiece piece, Notation target)
{
    u64 mask = UINT64_C(1) << target.index();
	m_material[piece.set()].material[piece.index()] ^= mask;
	
	return true;
}

bool Bitboard::PlacePiece(ChessPiece piece, Notation target)
{
    u64 mask = UINT64_C(1) << target.index();
	m_material[piece.set()].material[piece.index()] |= mask;
	
	return true;
}

u64 Bitboard::internalGenerateMask(byte curSqr, signed short dir, bool& sliding, ResolveMask resolveSquare) const
{
    u64 ret = ~universe;

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
      
        ret |= sqrMask;

    } while (sliding);

    return ret;
}

const int index64[64] = {
	0, 47,  1, 56, 48, 27,  2, 60,
   57, 49, 41, 37, 28, 16,  3, 61,
   54, 58, 35, 52, 50, 42, 21, 44,
   38, 32, 29, 23, 17, 11,  4, 62,
   46, 55, 26, 59, 40, 36, 15, 53,
   34, 51, 20, 43, 31, 22, 10, 45,
   25, 39, 14, 33, 19, 30,  9, 24,
   13, 18,  8, 12,  7,  6,  5, 63
};

/**
 * bitScanForward
 * @author Kim Walisch (2012)
 * @param bb bitboard to scan
 * @precondition bb != 0
 * @return index (0..63) of least significant one bit
 */
int bitScanForward(u64 bb) {
	//return __builtin_ctzll(bb);
	const u64 debruijn64 = 0x03f79d71b4cb0a89;
	assert(bb != 0);
	return index64[((bb ^ (bb - 1)) * debruijn64) >> 58];
}

int Bitboard::BitScanFowrward(u64 bitboard) const
{
    //return __builtin_ctzll(bitboard);
    return bitScanForward(bitboard);
}

u64 Bitboard::calcAvailableMovesForPawn(u64 mat, u64 opMat, Notation source, ChessPiece piece, byte enPassant, u64 threatenedMask, KingMask checkedMaskStruct, KingMask kingMaskStruct) const
{
    u64 ret = ~universe;    
    u64 matComb = mat | opMat;
    u64 checkedMask = checkedMaskStruct.combined();
    u64 kingMask = kingMaskStruct.combined();

    // by default we assume our piece is black
    byte curSet = 1;
    byte row = 6;
    signed char moveMod = 1;
    
    if (piece.getSet() == Set::WHITE)
    {
        row = 1;
        moveMod = -1;
        curSet = 0;
    }
    byte epTargetRank = (enPassant / 8) + moveMod;
    // figure out if we're pinned
    u64 sqrMask = UINT64_C(1) << source.index();
    bool pinned = (sqrMask & kingMask);
    // this magic code is to solve the issue where we have a pinned pawn and a en passant capture opportunity,
    // but capturing the en passant would put our king in check.
    bool wasPinned = false;
    if (pinned && kingMaskStruct.pawnMask)
    {
        // verify we're on the rank of en passant target;
        if (rankMasks[epTargetRank] & sqrMask)
        {
            pinned = false;
            wasPinned = true;
        }
    }

    if(checkedMask > 0) // figure out if en passant target is the one putting our king in check
    {
        threatenedMask = checkedMask;
        
        if (enPassant != 0xff)
        {
            Notation ept(enPassant);
            Notation enPassantPieceSqr(ept.file, ept.rank + moveMod);
            // calculate attacked squares
            ChessPiece opPawn = ChessPiece(ChessPiece::FlipSet((Set)curSet), PieceType::PAWN);
            u64 enPassantAttack = calcThreatenedSquares(enPassantPieceSqr, opPawn);
            
            if (enPassantAttack & m_material[piece.set()].material[5])
            {
				threatenedMask = checkedMask | (UINT64_C(1) << enPassant);
            }    
        }        

        if (pinned)
            threatenedMask &= kingMask;
    }
    else if (pinned)
    {
        for (byte i = 0; i < 8; ++i)
        {
            if (sqrMask & kingMaskStruct.threats[i])
            {                
                threatenedMask = kingMaskStruct.threats[i];
                break;
            }        
        }
    }    

    // Figure out if we're moving a pawn out of it's starting position.
    // Pawn specific modifires and values, starting row to figure out if we can do a double step
    // moveModifier for allowing white & black pawn moves to be represented by one value.
    signed char startRow = 0;
    signed char sq0x88 = source.index() + (source.index() & ~7);
    startRow = (sq0x88 >> 4) == row ? 0 : -1;

    // remove one move count if piece is a pawn and we are not on start row.
    byte moveCount = ChessPieceDef::MoveCount(piece.index()) + startRow;

    auto resolve = [&](u64 sqrMask)
    {
        if (matComb & sqrMask)
            return false;
        
        return true;
    };
    
    for (byte moveIndx = 0; moveIndx < moveCount; ++moveIndx)
    {        
        byte curSqr = source.index();
		signed short dir = ChessPieceDef::Moves0x88(piece.index(), moveIndx);
        dir *= moveMod;

        bool sliding = false;
        
        // check if we are blocked by a friendly piece or a oponent piece.      
        ret |= internalGenerateMask(curSqr, dir, sliding, resolve);
        
        if (ret == 0) // we've run into a piece, i.e. we can't do double move.
            break;
    }

    // add attacked squares to potential moves
    if (enPassant != 0xff && !wasPinned)
    {
        u64 enPassantMask = UINT64_C(1) << enPassant;
        opMat |= enPassantMask;
    }

    u64 potentialAttacks = calcThreatenedSquares(source, piece);
    ret |= (potentialAttacks & opMat);

    if ((checkedMask > 0) || pinned)
        ret &= threatenedMask;

    return ret;
}

KingMask 
Bitboard::calcKingMask(ChessPiece king, Notation source, const MaterialSlidingMask& opponentSlidingMask) const
{
    byte moveCount = ChessPieceDef::MoveCount(king.index());

    u8 bishop = 2;
    u8 rook = 3;
    u8 queen = 4;

    KingMask ret;

    u8 opSet = ChessPiece::FlipSet(king.set());
    u64 slideMat = SlidingMaterialCombined(opSet);
    const u64 c_diagnoalMat = m_material[opSet].bishops | m_material[opSet].queens;
    const u64 c_orthogonalMat = m_material[opSet].rooks | m_material[opSet].queens;
    u64 diagnoalMat = opponentSlidingMask.diagonal;
	u64 orthogonalMat = opponentSlidingMask.orthogonal;
    u64 allMat = MaterialCombined();
    u64 knightMat = m_material[opSet].knights;
    //u64 ret = ~universe;
    u8 checks = 0;
    
    if (c_diagnoalMat > 0 || c_orthogonalMat > 0)
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
            byte curSqr = source.index();
            signed short dir = ChessPieceDef::Moves0x88(king.index(), moveIndx);
            bool diagonal = ChessPieceDef::IsDiagonalMove(dir);

            ret.threats[moveIndx] = ~universe;
			
            if (diagonal)
				slideMat = diagnoalMat & c_diagnoalMat;
            else
				slideMat = orthogonalMat & c_orthogonalMat;

			if (slideMat == 0)
				continue;			

            u64 mvMask = internalGenerateMask(curSqr, dir, sliding, resolve);
            // comparing against two here since we'll find the sliding piece causing the pin
            // and at least one piece in between our king and this piece. This found piece isn't
            // necessarily pinned, if there are no more pieces between king and sliding piece
            // they are pinned
            if (mvMask & slideMat && matCount <= 2)
            {
                ret.threats[moveIndx] |= mvMask;
                if (matCount == 1)
                {
                    ret.checked[moveIndx] = true;
                    checks++;
                }
            }
        }
    }

    ret.knightsAndPawns = ~universe;
    if (knightMat > 0)
    {
        // figure out if we're checked by a knight
        bool sliding = false;
        static const ChessPiece knight(Set::WHITE, PieceType::KNIGHT);
        moveCount = ChessPieceDef::MoveCount(knight.index());
        
        auto nResolve = [&](u64 sqrMask)
        {
            return true;
        };

        for (byte moveIndx = 0; moveIndx < moveCount; ++moveIndx)
        {
            
            byte curSqr = source.index();
            signed short dir = ChessPieceDef::Moves0x88(knight.index(), moveIndx);

            u64 mvMask = internalGenerateMask(curSqr, dir, sliding, nResolve);
            if (mvMask & knightMat)
            {
                ret.knightsAndPawns |= mvMask;
                ret.knightOrPawnCheck = true;
                checks++;
            }
        }
    }

    if (m_material[opSet].pawns > 0)
    {
        // figure out if we're checked by a pawn
        i8 pawnMod = king.getSet() == Set::WHITE ? 1 : -1;
        auto pawnSqr = Notation(source.file + 1, source.rank + pawnMod);
        if (Bitboard::IsValidSquare(pawnSqr))
        {
            u64 sqrMak = UINT64_C(1) << pawnSqr.index();
            sqrMak &= m_material[opSet].pawns;
            if (sqrMak > 0)
            {
                ret.knightsAndPawns |= sqrMak;
                ret.knightOrPawnCheck = true;
                checks++;
            }
        }
        pawnSqr = Notation(source.file - 1, source.rank + pawnMod);
        if (Bitboard::IsValidSquare(pawnSqr))
        {
            u64 sqrMak = UINT64_C(1) << pawnSqr.index();
            sqrMak &= m_material[opSet].pawns;            
            if (sqrMak > 0)
            {
                ret.knightsAndPawns |= sqrMak;
                ret.knightOrPawnCheck = true;
                checks++;
            }
        }
    }

    return ret;
}

u64 Bitboard::calcAvailableMovesForKing(u64 mat, u64 threatenedMask, Notation source, ChessPiece piece, byte castling) const
{
    u64 ret = ~universe;

    byte moveCount = ChessPieceDef::MoveCount(piece.index());

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
		signed short dir = ChessPieceDef::Moves0x88(piece.index(), moveIndx);

        bool sliding = false;
        ret |= internalGenerateMask(curSqr, dir, sliding, resolve);
    }
    
    ret |= Castling(piece.set(), castling, threatenedMask);

    return ret;
}

u64 Bitboard::calcAvailableMoves(Notation source, ChessPiece piece, byte castling, byte enPassant, u64 threatened, KingMask checkedMask, KingMask kingMask) const
{
    u64 ret = ~universe;
    u64 matComb = MaterialCombined(piece.set());
    u64 opMatComb = MaterialCombined(ChessPiece::FlipSet(piece.set()));
    
    if (piece.getType() == PieceType::PAWN)
        return calcAvailableMovesForPawn(matComb, opMatComb, source, piece, enPassant, threatened, checkedMask, kingMask);
    else if(piece.getType() == PieceType::KING)
        return calcAvailableMovesForKing(matComb, threatened, source, piece, castling);
    
    bool checked = !checkedMask.zero();
    bool pinned = false;
    byte moveIndx = 0;

    // figure out if we're pinned and if so overwrite the threatened mask.
    u64 sqrMask = UINT64_C(1) << source.index();

    for (byte i = 0; i < 8; ++i)
    {
        if(sqrMask & kingMask.threats[i])
        {
            pinned = true;
            threatened = kingMask.threats[i];
        }
    }
    
    if (checked)
    {
        if (pinned)
            threatened &= checkedMask.combined();
        else
            threatened = checkedMask.combined();
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

    // auto validChecked = [&](u64 sqrMask)
    // {
    //     sqrMask &= threatened;
    //     return sqrMask;            
    // };

    byte moveCount = ChessPieceDef::MoveCount(piece.index());
    for (; moveIndx < moveCount; ++moveIndx)
    {
        sliding = ChessPieceDef::Slides(piece.index());
        byte curSqr = source.index();
		signed short dir = ChessPieceDef::Moves0x88(piece.index(), moveIndx);

        u64 mvMask = 0;
        if (checked || pinned)
            mvMask = internalGenerateMask(curSqr, dir, sliding, resolve);
        else
            mvMask = internalGenerateMask(curSqr, dir, sliding, resolve);       

        ret |= mvMask;
    }

    if (checked || pinned)
        ret &= threatened;

    return ret;
}

u64 Bitboard::GetThreatenedSquaresWithMaterial(Notation source, ChessPiece piece, bool pierceKing) const
{
    u64 retValue = calcThreatenedSquares(source, piece, pierceKing);
    u64 mask = UINT64_C(1) << source.index();
    return retValue | mask;
}

u64 Bitboard::GetMaterial(ChessPiece piece) const
{
    return m_material[piece.set()].material[piece.index()];
}

u64 Bitboard::calcThreatenedSquares(Notation source, ChessPiece piece, bool pierceKing) const
{
    u64 ret = ~universe;
    auto opSet = ChessPiece::FlipSet(piece.set());
    u64 matComb = MaterialCombined(piece.set());
    u64 opMatComb = MaterialCombined(opSet);

    // removing king from opmaterial so it doesn't stop our sliding.
    if (pierceKing)
        opMatComb &= ~m_material[opSet].kings;

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

    byte moveCount = ChessPieceDef::MoveCount(piece.index());
    for (byte moveIndx = 0; moveIndx < moveCount; ++moveIndx)
    {
        sliding = ChessPieceDef::Slides(piece.index());
        byte curSqr = source.index();
		signed short dir = ChessPieceDef::Attacks0x88(piece.index(), moveIndx);
        dir *= moveMod;
        ret |= internalGenerateMask(curSqr, dir, sliding, resolve);
    }

    return ret;
}

u64 Bitboard::calcAttackedSquares(Notation source, ChessPiece piece) const
{
    u64 opMatComb = MaterialCombined(ChessPiece::FlipSet(piece.set()));
    u64 ret = calcThreatenedSquares(source, piece);
    return ret & opMatComb;
}

bool Bitboard::IsValidMove(Notation source, ChessPiece piece, Notation target, byte castling, byte enPassant, u64 threatenedMask) const
{
    u64 movesMask = calcAvailableMoves(source, piece, castling, enPassant, threatenedMask);
    movesMask |= calcAttackedSquares(source, piece);

    u64 targetMask = UINT64_C(1) << target.index();

    return movesMask & targetMask;
}

u64 Bitboard::Castling(byte set, byte castling, u64 threatenedMask) const
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
    
    u64 attacked = threatenedMask; 
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
        u64 threatMask = ~universe;
        u64 blockedMask = ~universe;
        threatMask |= UINT64_C(1) << csqr;
        threatMask |= UINT64_C(1) << dsqr;

        blockedMask |= threatMask;
        blockedMask |= UINT64_C(1) << bsqr;


        if (!(attacked & threatMask) && !(combMat & blockedMask))
            retVal |= UINT64_C(1) << csqr;
    }
    return retVal;
}

MaterialMask Bitboard::GetMaterial(Set set) const
{
    return m_material[(size_t)set];
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
    for (u64 msk : m_material[set].material)
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
    return m_material[set].bishops | m_material[set].rooks | m_material[set].queens;
}