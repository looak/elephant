#include "position.hpp"
#include <array>
#include "bitboard.hpp"
#include "chess_piece.h"
#include "log.h"
#include "notation.h"

Position::Position()
{
    m_material[0] = {};
    m_material[1] = {};
}

Position&
Position::operator=(const Position& other)
{
    m_material[0] = {};
    m_material[1] = {};
    m_material[0] = other.m_material[0];
    m_material[1] = other.m_material[1];
    return *this;
}

bool
Position::IsValidSquare(signed short currSqr)
{
    if (currSqr < 0)
        return false;
    if (currSqr > 127) {
        // LOG_ERROR() << "In case index is larger than 127 it will wrap around our board.";
        return false;
    }

    byte sq0x88 = currSqr + (currSqr & ~7);
    return (sq0x88 & 0x88) == 0;
}

bool
Position::IsValidSquare(Notation source)
{
    return Position::IsValidSquare(source.index());
}

void
Position::Clear()
{
    m_material[0] = {};
    m_material[1] = {};
}

bool
Position::ClearPiece(ChessPiece piece, Notation target)
{
    u64 mask = squareMaskTable[target.index()];
    m_material[piece.set()].material[piece.index()] ^= mask;

    return true;
}

bool
Position::PlacePiece(ChessPiece piece, Notation target)
{
    u64 mask = squareMaskTable[target.index()];
    m_material[piece.set()].material[piece.index()] |= mask;

    return true;
}

u64
Position::calcAvailableMovesForPawn(u64 mat, u64 opMat, Notation source, ChessPiece piece, Notation enPassant,
                                    u64 threatenedMask, KingMask checkedMaskStruct, KingMask kingMaskStruct) const
{
    u64 ret = ~universe;
    u64 matComb = mat | opMat;
    u64 checkedMask = checkedMaskStruct.combined();
    u64 kingMask = kingMaskStruct.combined();

    // by default we assume our piece is black
    byte curSet = 1;
    byte row = 6;
    signed char moveMod = 1;

    if (piece.getSet() == Set::WHITE) {
        row = 1;
        moveMod = -1;
        curSet = 0;
    }

    // figure out if we're pinned
    u64 sqrMask = squareMaskTable[source.index()];
    bool pinned = (sqrMask & kingMask);
    // this magic code is to solve the issue where we have a pinned pawn and a en passant capture
    // opportunity, but capturing the en passant would put our king in check.
    bool wasPinned = false;
    if (pinned && kingMaskStruct.pawnMask) {
        // verify we're on the rank of en passant target;
        if (board_constants::rankMasks[enPassant.rank + moveMod] & sqrMask) {
            pinned = false;
            wasPinned = true;
        }
    }

    if (checkedMask > 0)  // figure out if en passant target is the one putting our king in check
    {
        threatenedMask = checkedMask;

        if (enPassant.isValid()) {
            // calculate attacked squares
            ChessPiece opPawn = ChessPiece(ChessPiece::FlipSet((Set)curSet), PieceType::PAWN);
            Notation enPassantPieceSqr(enPassant.file, enPassant.rank + moveMod);
            u64 enPassantAttack = calcThreatenedSquares(enPassantPieceSqr, opPawn);

            if (enPassantAttack & m_material[piece.set()].material[5]) {
                threatenedMask = checkedMask | squareMaskTable[enPassant.index()];
            }
        }

        if (pinned)
            threatenedMask &= kingMask;
    }
    else if (pinned) {
        for (byte i = 0; i < 8; ++i) {
            if (sqrMask & kingMaskStruct.threats[i]) {
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

    for (byte moveIndx = 0; moveIndx < moveCount; ++moveIndx) {
        byte curSqr = source.index();
        signed short dir = ChessPieceDef::Moves0x88(piece.index(), moveIndx);
        dir *= moveMod;

        // build a 0x88 square out of current square.
        signed char sq0x88 = to0x88(curSqr);
        // do move
        sq0x88 += dir;
        if (sq0x88 & 0x88)  // validate move, are we still on the board?
            continue;

        // convert our sqr0x88 back to a square index
        curSqr = fr0x88(sq0x88);
        // build a square mask from current square
        u64 sqrMask = squareMaskTable[curSqr];

        // check if we are blocked by a friendly piece or a oponent piece.
        if (matComb & sqrMask)
            break;

        ret |= sqrMask;
    }

    // add attacked squares to potential moves
    if (enPassant.isValid() && !wasPinned) {
        u64 enPassantMask = squareMaskTable[enPassant.index()];
        opMat |= enPassantMask;
    }

    u64 potentialAttacks = calcThreatenedSquares(source, piece);
    ret |= (potentialAttacks & opMat);

    if ((checkedMask > 0) || pinned)
        ret &= threatenedMask;

    return ret;
}

KingMask
Position::calcKingMask(ChessPiece king, Notation source, const MaterialSlidingMask& opponentSlidingMask) const
{
    byte moveCount = ChessPieceDef::MoveCount(king.index());

    KingMask ret;

    const u8 opSet = ChessPiece::FlipSet(king.set());
    const u64 c_diagnoalMat = m_material[opSet].material[bishopId] | m_material[opSet].material[queenId];
    const u64 c_orthogonalMat = m_material[opSet].material[rookId] | m_material[opSet].material[queenId];
    const u64 diagnoalMat = opponentSlidingMask.diagonal;
    const u64 orthogonalMat = opponentSlidingMask.orthogonal;
    const u64 knightMat = m_material[opSet].material[knightId];
    const u64 allMat = MaterialCombined();

    const u64 slideMatCache[2]{orthogonalMat & c_orthogonalMat, diagnoalMat & c_diagnoalMat};

    if (c_diagnoalMat > 0 || c_orthogonalMat > 0) {
        u8 matCount = 0;
        bool sliding = true;

        for (byte moveIndx = 0; moveIndx < moveCount; ++moveIndx) {
            matCount = 0;
            sliding = true;
            byte curSqr = source.index();
            signed short dir = ChessPieceDef::Moves0x88(king.index(), moveIndx);

            ret.threats[moveIndx] = ~universe;

            bool diagonal = ChessPieceDef::IsDiagonalMove(dir);
            u64 slideMat = slideMatCache[diagonal];

            if (slideMat == 0)
                continue;

            u64 mvMask = 0;
            do {
                // build a 0x88 square out of current square.
                signed char sq0x88 = to0x88(curSqr);
                // do move
                sq0x88 += dir;
                if (sq0x88 & 0x88)  // validate move, are we still on the board?
                {
                    sliding = false;
                    break;
                }

                // convert our sqr0x88 back to a square index
                curSqr = fr0x88(sq0x88);
                // build a square mask from current square
                u64 sqrMask = squareMaskTable[curSqr];

                if (allMat & sqrMask)
                    matCount++;

                if (slideMat & sqrMask) {
                    sliding = false;
                }

                mvMask |= sqrMask;

            } while (sliding);

            // comparing against two here since we'll find the sliding piece causing the pin
            // and at least one piece in between our king and this piece. This found piece isn't
            // necessarily pinned, if there are no more pieces between king and sliding piece
            // they are pinned
            if (mvMask & slideMat && matCount <= 2) {
                ret.threats[moveIndx] |= mvMask;
                if (matCount == 1) {
                    ret.checked[moveIndx] = true;
                }
            }
        }
    }

    ret.knightsAndPawns = ~universe;
    if (knightMat > 0) {
        // figure out if we're checked by a knight
        static const ChessPiece knight(Set::WHITE, PieceType::KNIGHT);
        moveCount = ChessPieceDef::MoveCount(knight.index());

        for (byte moveIndx = 0; moveIndx < moveCount; ++moveIndx) {
            byte curSqr = source.index();
            signed short dir = ChessPieceDef::Moves0x88(knight.index(), moveIndx);

            u64 mvMask = 0;

            // build a 0x88 square out of current square.
            signed char sq0x88 = to0x88(curSqr);
            // do move
            sq0x88 += dir;
            if (sq0x88 & 0x88)  // validate move, are we still on the board?
            {
                continue;
            }

            // convert our sqr0x88 back to a square index
            curSqr = fr0x88(sq0x88);
            // build a square mask from current square
            u64 sqrMask = squareMaskTable[curSqr];
            mvMask |= sqrMask;

            if (mvMask & knightMat) {
                ret.knightsAndPawns |= mvMask;
                ret.knightOrPawnCheck = true;
            }
        }
    }

    if (m_material[opSet].material[pawnId] > 0) {
        // figure out if we're checked by a pawn
        i8 pawnMod = king.getSet() == Set::WHITE ? 1 : -1;
        auto pawnSqr = Notation(source.file + 1, source.rank + pawnMod);
        if (Position::IsValidSquare(pawnSqr)) {
            u64 sqrMak = squareMaskTable[pawnSqr.index()];
            sqrMak &= m_material[opSet].material[pawnId];
            if (sqrMak > 0) {
                ret.knightsAndPawns |= sqrMak;
                ret.knightOrPawnCheck = true;
            }
        }
        pawnSqr = Notation(source.file - 1, source.rank + pawnMod);
        if (Position::IsValidSquare(pawnSqr)) {
            u64 sqrMak = squareMaskTable[pawnSqr.index()];
            sqrMak &= m_material[opSet].material[pawnId];
            if (sqrMak > 0) {
                ret.knightsAndPawns |= sqrMak;
                ret.knightOrPawnCheck = true;
            }
        }
    }

    return ret;
}

u64
Position::calcAvailableMovesForKing(u64 mat, u64 threatenedMask, Notation source, ChessPiece piece, byte castling) const
{
    u64 ret = ~universe;

    byte moveCount = ChessPieceDef::MoveCount(piece.index());

    u64 combinedMatAndThreat = mat | threatenedMask;

    for (byte moveIndx = 0; moveIndx < moveCount; ++moveIndx) {
        byte curSqr = source.index();
        signed short dir = ChessPieceDef::Moves0x88(piece.index(), moveIndx);

        // build a 0x88 square out of current square.
        signed char sq0x88 = to0x88(curSqr);
        // do move
        sq0x88 += dir;
        if (sq0x88 & 0x88)  // validate move, are we still on the board?
            continue;

        // convert our sqr0x88 back to a square index
        curSqr = fr0x88(sq0x88);

        // build a square mask from current square
        u64 sqrMask = squareMaskTable[curSqr];

        // check if we are blocked by a friendly piece or a oponent piece.
        if (sqrMask & combinedMatAndThreat)
            continue;

        ret |= sqrMask;
    }

    ret |= Castling(piece.set(), castling, threatenedMask);

    return ret;
}

u64
Position::calcAvailableMoves(Notation source, ChessPiece piece, byte castling, Notation enPassant, u64 threatened,
                             KingMask checkedMask, KingMask kingMask) const
{
    u64 ret = ~universe;
    u64 matComb = MaterialCombined(piece.set());
    u64 opMatComb = MaterialCombined(ChessPiece::FlipSet(piece.set()));

    if (piece.getType() == PieceType::PAWN)
        return calcAvailableMovesForPawn(matComb, opMatComb, source, piece, enPassant, threatened, checkedMask, kingMask);
    else if (piece.getType() == PieceType::KING)
        return calcAvailableMovesForKing(matComb, threatened, source, piece, castling);

    bool checked = !checkedMask.zero();
    bool pinned = false;
    byte moveIndx = 0;

    // figure out if we're pinned and if so overwrite the threatened mask.
    u64 sqrMask = squareMaskTable[source.index()];

    for (byte i = 0; i < 8; ++i) {
        if (sqrMask & kingMask.threats[i]) {
            pinned = true;
            threatened = kingMask.threats[i];
        }
    }

    if (checked) {
        if (pinned)
            threatened &= checkedMask.combined();
        else
            threatened = checkedMask.combined();
    }

    bool sliding = false;
    byte moveCount = ChessPieceDef::MoveCount(piece.index());
    for (; moveIndx < moveCount; ++moveIndx) {
        sliding = ChessPieceDef::Slides(piece.index());
        byte curSqr = source.index();
        signed short dir = ChessPieceDef::Moves0x88(piece.index(), moveIndx);

        u64 mvMask = 0;
        do {
            // build a 0x88 square out of current square.
            signed char sq0x88 = to0x88(curSqr);
            // do move
            sq0x88 += dir;
            if (sq0x88 & 0x88)  // validate move, are we still on the board?
            {
                sliding = false;
                break;
            }

            // convert our sqr0x88 back to a square index
            curSqr = fr0x88(sq0x88);
            // build a square mask from current square
            u64 sqrMask = squareMaskTable[curSqr];

            if (matComb & sqrMask) {
                sliding = false;
                break;
            }
            else if (opMatComb & sqrMask) {
                sliding = false;
            }

            ret |= sqrMask;

        } while (sliding);

        ret |= mvMask;
    }

    if (checked || pinned)
        ret &= threatened;

    return 0;
}

// u64
// Position::GetThreatenedSquaresWithMaterial(Notation source, ChessPiece piece, bool pierceKing) const
// {
//     u64 retValue = calcThreatenedSquares(source, piece, pierceKing);
//     u64 mask = squareMaskTable[source.index()];
//     return retValue | mask;
// }

u64
Position::GetMaterial(ChessPiece piece) const
{
    return m_material[piece.set()].material[piece.index()];
}

u64
Position::calcThreatenedSquares(Notation source, ChessPiece piece, bool pierceKing) const
{
    u64 ret = ~universe;
    auto opSet = ChessPiece::FlipSet(piece.set());
    u64 matComb = MaterialCombined(piece.set());
    u64 opMatComb = MaterialCombined(opSet);

    // removing king from opmaterial so it doesn't stop our sliding.
    if (pierceKing)
        opMatComb &= ~m_material[opSet].material[kingId];

    signed char moveMod = 1;
    if (piece.getSet() == Set::WHITE)
        moveMod = -1;

    bool sliding = false;
    matComb |= opMatComb;

    byte moveCount = ChessPieceDef::MoveCount(piece.index());
    for (byte moveIndx = 0; moveIndx < moveCount; ++moveIndx) {
        sliding = ChessPieceDef::Slides(piece.index());
        byte curSqr = source.index();
        signed short dir = ChessPieceDef::Attacks0x88(piece.index(), moveIndx);
        dir *= moveMod;

        do {
            // build a 0x88 square out of current square.
            signed char sq0x88 = to0x88(curSqr);
            // do move
            sq0x88 += dir;
            if (sq0x88 & 0x88)  // validate move, are we still on the board?
                break;

            // convert our sqr0x88 back to a square index
            curSqr = fr0x88(sq0x88);
            // build a square mask from current square
            u64 sqrMask = squareMaskTable[curSqr];

            if ((matComb & sqrMask) > 0)
                sliding = false;

            ret |= sqrMask;

        } while (sliding);
    }

    return ret;
}

u64
Position::calcAttackedSquares(Notation source, ChessPiece piece) const
{
    u64 opMatComb = MaterialCombined(ChessPiece::FlipSet(piece.set()));
    u64 ret = calcThreatenedSquares(source, piece);
    return ret & opMatComb;
}

bool
Position::IsValidMove(Notation source, ChessPiece piece, Notation target, byte castling, Notation enPassant,
                      u64 threatenedMask) const
{
    u64 movesMask = calcAvailableMoves(source, piece, castling, enPassant, threatenedMask);
    movesMask |= calcAttackedSquares(source, piece);

    u64 targetMask = squareMaskTable[target.index()];

    return movesMask & targetMask;
}

u64
Position::Castling(byte set, byte castling, u64 threatenedMask) const
{
    u64 retVal = ~universe;
    byte rank = 0;
    if (set == 1)  // BLACK
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
    if (castling & 1) {
        // build castling square mask
        byte fsqr = (rank * 8) + 5;
        byte gsqr = fsqr + 1;
        u64 mask = ~universe;
        mask |= squareMaskTable[fsqr];
        mask |= squareMaskTable[gsqr];

        if (!(attacked & mask) && !(combMat & mask))
            retVal |= squareMaskTable[gsqr];
    }
    // check queen side
    if (castling & 2) {
        // build castling square mask
        byte bsqr = (rank * 8) + 1;
        byte csqr = bsqr + 1;
        byte dsqr = csqr + 1;
        u64 threatMask = ~universe;
        u64 blockedMask = ~universe;
        threatMask |= squareMaskTable[csqr];
        threatMask |= squareMaskTable[dsqr];

        blockedMask |= threatMask;
        blockedMask |= squareMaskTable[bsqr];

        if (!(attacked & threatMask) && !(combMat & blockedMask))
            retVal |= squareMaskTable[csqr];
    }
    return retVal;
}

MaterialMask
Position::GetMaterial(Set set) const
{
    return m_material[(size_t)set];
}

u64
Position::GetMaterialCombined(Set set) const
{
    return MaterialCombined(static_cast<byte>(set));
}

u64
Position::MaterialCombined() const
{
    return MaterialCombined(0) | MaterialCombined(1);
}

u64
Position::MaterialCombined(byte set) const
{
    return m_material[set].material[pawnId] | m_material[set].material[knightId] | m_material[set].material[bishopId] |
           m_material[set].material[rookId] | m_material[set].material[queenId] | m_material[set].material[kingId];
}

u64
Position::SlidingMaterialCombined(byte set) const
{
    return m_material[set].material[bishopId] | m_material[set].material[rookId] | m_material[set].material[queenId];
}

template<Set us>
std::tuple<u64, u64>
Position::internalIsolatePawn(Notation source, u64 movesbb) const
{
    if (intrinsics::popcnt(m_material[(size_t)us].material[pawnId]) <= 1)
        return {movesbb, 0};

    u64 unoccupied = ~(m_material[0].combine() | m_material[1].combine());

    u64 srcMask = UINT64_C(1) << source.index();
    u64 isolatedbb = shiftNorthRelative<us>(srcMask);
    u64 doublePush = isolatedbb & pawn_constants::baseRank[(size_t)us] & unoccupied;
    isolatedbb |= shiftNorthRelative<us>(doublePush);

    u64 threatns = 0;
    if ((srcMask & board_constants::boundsRelativeMasks[(size_t)us][west]) == 0)
        threatns |= shiftNorthWestRelative<us>(srcMask);
    if ((srcMask & board_constants::boundsRelativeMasks[(size_t)us][east]) == 0)
        threatns |= shiftNorthEastRelative<us>(srcMask);

    auto them = opposing_set<us>();
    return {movesbb & isolatedbb, m_material[(size_t)them].combine() & threatns};
}

template std::tuple<u64, u64> Position::internalIsolatePawn<Set::WHITE>(Notation source, u64 movesbb) const;
template std::tuple<u64, u64> Position::internalIsolatePawn<Set::BLACK>(Notation source, u64 movesbb) const;

template<Set us>
std::tuple<u64, u64>
Position::internalIsolateKnight(Notation source, u64 movesbb) const
{
    return {0, 0};
}

std::tuple<u64, u64>
Position::internalIsolateBishop(Set set, Notation source, u64 movesbb) const
{
    if (intrinsics::popcnt(m_material[(size_t)set].material[bishopId]) <= 1)
        return {movesbb, 0};

    // figure out forward diagnoal
    u16 index = 7 + source.file - source.rank;
    u64 forwardDiag = board_constants::forwardDiagonalMasks[index];
    u16 bindex = source.file + source.rank;
    u64 backwardDiag = board_constants::backwardDiagonalMasks[bindex];

    u64 mask = forwardDiag | backwardDiag;
    u64 pieceMask = mask & m_material[(size_t)set].material[bishopId];
    i32 count = intrinsics::popcnt(pieceMask);
    if (count == 1)
        return {movesbb & mask, 0};

    movesbb &= mask;
    pieceMask &= ~squareMaskTable[source.index()];
    i32 otherSquare = intrinsics::lsbIndex(pieceMask);
    Notation other(otherSquare);

    // Bitboard excludeMask{};
    // i32 nsDiff = otherSquare - source.index();
    // i32 weDiff = diffWestEast(source, other);
    // if (nsDiff < 0) {
    //     if (weDiff < 0)
    //         excludeMask = excludeMask.inclusiveFillSouthWest(other.file, other.rank);
    //     else
    //         excludeMask = excludeMask.inclusiveFillSouthEast(other.file, other.rank);
    // }
    // else {
    //     if (weDiff < 0)
    //         excludeMask = excludeMask.inclusiveFillNorthWest(other.file, other.rank);
    //     else
    //         excludeMask = excludeMask.inclusiveFillNorthEast(other.file, other.rank);
    // }

    // movesbb &= (~excludeMask).read();
    return {movesbb & (forwardDiag | backwardDiag), 0};
}

std::tuple<u64, u64>
Position::internalIsolateRook(Set set, Notation source, u64 movesbb) const
{
    if (intrinsics::popcnt(m_material[(size_t)set].material[rookId]) <= 1)
        return {movesbb, 0};

    u64 mask = board_constants::fileMasks[source.file] | board_constants::rankMasks[source.rank];
    u64 pieceMask = mask & m_material[(size_t)set].material[rookId];
    i32 count = intrinsics::popcnt(pieceMask);
    if (count == 1)
        return {movesbb & mask, 0};

    movesbb &= mask;
    pieceMask &= ~squareMaskTable[source.index()];
    i32 otherSquare = intrinsics::lsbIndex(pieceMask);

    // i32 diff = otherSquare - source.index();
    // if ((pieceMask & board_constants::fileMasks[source.file]) == 0)  // not on same file
    // {
    //     if (diff < 0) {
    //         u64 excludeMask = inclusiveFillWest(source.file + diff);
    //         movesbb &= ~excludeMask;
    //     }
    //     else {
    //         u64 excludeMask = inclusiveFillEast(source.file + diff);
    //         movesbb &= ~excludeMask;
    //     }
    // }
    // else {
    //     i32 shiftValue = abs(diff);  // - 8;
    //     if (diff < 0) {
    //         movesbb = movesbb >> shiftValue;
    //         movesbb = movesbb << shiftValue;
    //     }
    //     else {
    //         shiftValue -= 8;
    //         movesbb = movesbb << shiftValue;
    //         movesbb = movesbb >> shiftValue;
    //     }
    // }

    return {movesbb, 0};
}

i32
Position::diffWestEast(Notation a, Notation b) const
{
    i32 a_flattened = mod_by_eight(a.index());
    i32 b_flattened = mod_by_eight(b.index());
    return b_flattened - a_flattened;
}

template<Set s>
u64
Position::calcAvailableMovesPawnBulk() const
{
    const u64 usMat = m_material[(size_t)s].combine();
    const u64 opMat = m_material[(size_t)opposing_set<s>()].combine();
    const u64 unoccupied = ~(usMat | opMat);
    const u64 piecebb = m_material[(size_t)s].material[pawnId];
    u64 mvsbb = ~universe;

    mvsbb = shiftNorthRelative<s>(piecebb);
    u64 doublePush = mvsbb & pawn_constants::baseRank[(size_t)s] & unoccupied;
    mvsbb |= shiftNorthRelative<s>(doublePush);

    mvsbb &= unoccupied;

    mvsbb |= opMat & calcThreatenedSquaresPawnBulk<s>();
    return mvsbb;
}

template u64 Position::calcAvailableMovesPawnBulk<Set::WHITE>() const;
template u64 Position::calcAvailableMovesPawnBulk<Set::BLACK>() const;

template<Set s>
u64
Position::readCombinedMaterial() const
{
    return readMaterial<s>().combine();
}

template u64 Position::readCombinedMaterial<Set::WHITE>() const;
template u64 Position::readCombinedMaterial<Set::BLACK>() const;

template<Set us>
std::tuple<u64, u64>
Position::isolatePiece(u8 pieceId, Notation source, u64 movesbb) const
{
    switch (pieceId) {
        case pawnId:
            return internalIsolatePawn<us>(source, movesbb);
        case bishopId:
            return internalIsolateBishop(us, source, movesbb);
        case rookId:
            return internalIsolateRook(us, source, movesbb);
        case knightId:
            return internalIsolateKnight<us>(source, movesbb);
        default:
            FATAL_ASSERT(false) << "Not implemented";
    }

    return {0, 0};
}

template std::tuple<u64, u64> Position::isolatePiece<Set::WHITE>(u8 pieceId, Notation source, u64 movesbb) const;
template std::tuple<u64, u64> Position::isolatePiece<Set::BLACK>(u8 pieceId, Notation source, u64 movesbb) const;