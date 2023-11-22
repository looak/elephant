#include "position.hpp"
#include <array>
#include "bitboard.hpp"
#include "chess_piece.h"
#include "log.h"
#include "notation.h"

std::string
CastlingStateInfo::toString() const
{
    std::string result;
    if (hasWhiteKingSide())
        result += "K";
    if (hasWhiteQueenSide())
        result += "Q";
    if (hasBlackKingSide())
        result += "k";
    if (hasBlackQueenSide())
        result += "q";

    if (result.empty())
        result = "-";

    return result;
}

Position::Position() :
    m_material(),
    m_castlingState(),
    m_enpassantState()
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
    m_castlingState = other.m_castlingState;
    m_enpassantState = other.m_enpassantState;
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
    m_enpassantState = {};
    m_castlingState = {};
}

bool
Position::ClearPiece(ChessPiece piece, Notation target)
{
    Square sqr = target.toSquare();
    Bitboard& piecebb = m_material[piece.set()].material[piece.index()];
    piecebb[sqr] = false;
    return true;
}

bool
Position::PlacePiece(ChessPiece piece, Notation target)
{
    auto sqr = target.toSquare();
    Bitboard& piecebb = m_material[piece.set()].material[piece.index()];
    piecebb[sqr] = true;
    return piecebb[sqr];
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

            if (m_material[piece.set()].material[5] & enPassantAttack) {
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

template<Set us>
KingMask
Position::calcKingMask() const
{
    constexpr Set op = opposing_set<us>();
    auto slidingMask = calcMaterialSlidingMasksBulk<op>();
    auto king = ChessPiece(us, PieceType::KING);
    auto kingSqr = m_material[(size_t)us].material[kingId].lsbIndex();
    return calcKingMask(king, Notation(kingSqr), slidingMask);
}

template KingMask Position::calcKingMask<Set::WHITE>() const;
template KingMask Position::calcKingMask<Set::BLACK>() const;

KingMask
Position::calcKingMask(ChessPiece king, Notation source, const MaterialSlidingMask& opponentSlidingMask) const
{
    byte moveCount = ChessPieceDef::MoveCount(king.index());

    KingMask ret;

    const u8 opSet = ChessPiece::FlipSet(king.set());
    const u64 c_diagnoalMat = m_material[opSet].material[bishopId].read() | m_material[opSet].material[queenId].read();
    const u64 c_orthogonalMat = m_material[opSet].material[rookId].read() | m_material[opSet].material[queenId].read();
    const u64 allMat = MaterialCombined().read();

    const u64 slideMatCache[2]{opponentSlidingMask.orthogonal.read() & c_orthogonalMat,
                               opponentSlidingMask.diagonal.read() & c_diagnoalMat};

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

    const u64 knightMat = m_material[opSet].material[knightId].read();

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

    if (m_material[opSet].material[pawnId].empty() == false) {
        // figure out if we're checked by a pawn
        i8 pawnMod = king.getSet() == Set::WHITE ? 1 : -1;
        auto pawnSqr = Notation(source.file + 1, source.rank + pawnMod);
        if (Position::IsValidSquare(pawnSqr)) {
            u64 sqrMak = squareMaskTable[pawnSqr.index()];
            sqrMak &= m_material[opSet].material[pawnId].read();
            if (sqrMak > 0) {
                ret.knightsAndPawns |= sqrMak;
                ret.knightOrPawnCheck = true;
            }
        }
        pawnSqr = Notation(source.file - 1, source.rank + pawnMod);
        if (Position::IsValidSquare(pawnSqr)) {
            u64 sqrMak = squareMaskTable[pawnSqr.index()];
            sqrMak &= m_material[opSet].material[pawnId].read();
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
    return 55;
    u64 ret = ~universe;
    u64 matComb = MaterialCombined(piece.set()).read();
    u64 opMatComb = MaterialCombined(ChessPiece::FlipSet(piece.set())).read();

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

Bitboard
Position::GetMaterial(ChessPiece piece) const
{
    return m_material[piece.set()].material[piece.index()];
}

u64
Position::calcThreatenedSquares(Notation source, ChessPiece piece, bool pierceKing) const
{
    u64 ret = ~universe;
    auto opSet = ChessPiece::FlipSet(piece.set());
    u64 matComb = MaterialCombined(piece.set()).read();
    u64 opMatComb = MaterialCombined(opSet).read();

    // removing king from opmaterial so it doesn't stop our sliding.
    if (pierceKing)
        opMatComb &= ~m_material[opSet].material[kingId].read();

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
    u64 opMatComb = MaterialCombined(ChessPiece::FlipSet(piece.set())).read();
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
    u64 combMat = MaterialCombined().read();

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

Bitboard
Position::GetMaterialCombined(Set set) const
{
    return MaterialCombined(static_cast<byte>(set));
}

Bitboard
Position::MaterialCombined() const
{
    return MaterialCombined(0) | MaterialCombined(1);
}

Bitboard
Position::MaterialCombined(byte set) const
{
    return (m_material[set].material[pawnId] | m_material[set].material[knightId] | m_material[set].material[bishopId] |
            m_material[set].material[rookId] | m_material[set].material[queenId] | m_material[set].material[kingId]);
}

Bitboard
Position::SlidingMaterialCombined(byte set) const
{
    return (m_material[set].material[bishopId] | m_material[set].material[rookId] | m_material[set].material[queenId]);
}

template<Set us>
Bitboard
Position::calcAvailableMovesPawnBulk(const KingMask& kingMask) const
{
    const size_t usIndx = static_cast<size_t>(us);
    const size_t opIndx = static_cast<size_t>(opposing_set<us>());
    const Bitboard usMat = m_material[usIndx].combine();
    const Bitboard opMat = m_material[opIndx].combine();
    const Bitboard unoccupied(~(usMat | opMat));
    const Bitboard piecebb = m_material[usIndx].material[pawnId];

    Bitboard mvsbb;
    mvsbb = piecebb.shiftNorthRelative<us>();
    Bitboard doublePush = mvsbb & pawn_constants::baseRank[usIndx] & unoccupied;
    mvsbb |= doublePush.shiftNorthRelative<us>();

    mvsbb &= unoccupied;

    mvsbb |= (opMat | m_enpassantState.readBitboard()) & calcThreatenedSquaresPawnBulk<us>();

    if (kingMask.isChecked())
        mvsbb &= kingMask.combined();

    return mvsbb;
}

template Bitboard Position::calcAvailableMovesPawnBulk<Set::WHITE>(const KingMask& kingMask) const;
template Bitboard Position::calcAvailableMovesPawnBulk<Set::BLACK>(const KingMask& kingMask) const;

template<Set us>
Bitboard
Position::calcThreatenedSquaresPawnBulk() const
{
    const size_t usIndx = static_cast<size_t>(us);
    Bitboard piecebb = m_material[usIndx].material[pawnId];

    // special case for a file & h file, removing pawns from a & h file respectively
    // so we don't shift them "off" the board and we shift them only in one direction.
    // cache them and then we combine it with the main piecebb at the end.
    Bitboard westFilePawns = piecebb & board_constants::boundsRelativeMasks[usIndx][west];
    piecebb &= ~westFilePawns;
    westFilePawns = westFilePawns.shiftNorthEastRelative<us>();

    Bitboard eastFilePawns = piecebb & board_constants::boundsRelativeMasks[usIndx][east];
    piecebb &= ~eastFilePawns;
    eastFilePawns = eastFilePawns.shiftNorthWestRelative<us>();

    Bitboard threatbb = westFilePawns | eastFilePawns;
    threatbb |= piecebb.shiftNorthWestRelative<us>();
    threatbb |= piecebb.shiftNorthEastRelative<us>();
    return threatbb;
}

template Bitboard Position::calcThreatenedSquaresPawnBulk<Set::WHITE>() const;
template Bitboard Position::calcThreatenedSquaresPawnBulk<Set::BLACK>() const;

template<Set us, Set op>
Bitboard
Position::calcAvailableMovesKing(byte castlingRights) const
{
    bool constexpr includeMaterial = false;
    bool constexpr pierceKing = true;
    Bitboard treatened = calcThreatenedSquares<op, includeMaterial, pierceKing>();
    Bitboard moves = calcThreatenedSquaresKing<us>();
    moves &= ~treatened;
    moves |= Castling((byte)us, castlingRights, treatened.read());
    return moves;
}

template Bitboard Position::calcAvailableMovesKing<Set::WHITE, Set::BLACK>(byte castlingRights) const;
template Bitboard Position::calcAvailableMovesKing<Set::BLACK, Set::WHITE>(byte castlingRights) const;

template<Set us>
Bitboard
Position::calcThreatenedSquaresKing() const
{
    const size_t setIndx = (size_t)us;
    Bitboard moves;
    const auto kingbb = m_material[setIndx].material[kingId];
    if (kingbb.empty())
        return moves;

    const u32 kingSqr = kingbb.lsbIndex();

    i64 newKingSqr = (const i64)kingSqr - king_constants::maskOffset;

    if (newKingSqr >= 0) {
        moves = (king_constants::moveMask << newKingSqr);
    }
    else if (newKingSqr < 0) {
        moves = (king_constants::moveMask >> -newKingSqr);
    }

    // clean up opposing side
    if ((kingbb & board_constants::fileaMask).empty() == false)
        moves ^= (moves & board_constants::filehMask);
    else if ((kingbb & board_constants::filehMask).empty() == false)
        moves ^= (moves & board_constants::fileaMask);

    // lastly remove any squares blocked by our own pieces.
    moves &= ~m_material[setIndx].combine();

    return moves;
}

template Bitboard Position::calcThreatenedSquaresKing<Set::WHITE>() const;
template Bitboard Position::calcThreatenedSquaresKing<Set::BLACK>() const;

template<Set us, u8 pieceId>
Bitboard
Position::calcAvailableMovesBishopBulk(const KingMask& kingMask) const
{
    const Bitboard materialbb = readMaterial<us>().combine();
    Bitboard moves = calcThreatenedSquaresBishopBulk<us, pieceId>();

    if (kingMask.isChecked())
        moves &= kingMask.combined();
    else
        moves ^= (materialbb & moves);

    return moves;
}

template Bitboard Position::calcAvailableMovesBishopBulk<Set::WHITE, bishopId>(const KingMask& kingMask) const;
template Bitboard Position::calcAvailableMovesBishopBulk<Set::BLACK, bishopId>(const KingMask& kingMask) const;
template Bitboard Position::calcAvailableMovesBishopBulk<Set::WHITE, queenId>(const KingMask& kingMask) const;
template Bitboard Position::calcAvailableMovesBishopBulk<Set::BLACK, queenId>(const KingMask& kingMask) const;

template<Set us, u8 pieceId>
Bitboard
Position::calcAvailableMovesRookBulk(const KingMask& kingMask) const
{
    const Bitboard materialbb = readMaterial<us>().combine();

    Bitboard moves = calcThreatenedSquaresRookBulk<us, pieceId>();

    if (kingMask.isChecked())
        moves &= kingMask.combined();
    else
        moves ^= (materialbb & moves);

    return moves;
}

template Bitboard Position::calcAvailableMovesRookBulk<Set::WHITE, rookId>(const KingMask& kingMask) const;
template Bitboard Position::calcAvailableMovesRookBulk<Set::BLACK, rookId>(const KingMask& kingMask) const;
template Bitboard Position::calcAvailableMovesRookBulk<Set::WHITE, queenId>(const KingMask& kingMask) const;
template Bitboard Position::calcAvailableMovesRookBulk<Set::BLACK, queenId>(const KingMask& kingMask) const;

template<Set us>
Bitboard
Position::calcAvailableMovesQueenBulk(const KingMask& kingMask) const
{
    Bitboard moves = 0;
    moves |= calcAvailableMovesBishopBulk<us, queenId>(kingMask);
    moves |= calcAvailableMovesRookBulk<us, queenId>(kingMask);
    return moves;
}

template Bitboard Position::calcAvailableMovesQueenBulk<Set::WHITE>(const KingMask& kingMask) const;
template Bitboard Position::calcAvailableMovesQueenBulk<Set::BLACK>(const KingMask& kingMask) const;

template<Set us>
Bitboard
Position::calcAvailableAttacksPawnBulk(const KingMask& kingMask) const
{
    constexpr Set op = opposing_set<us>();

    Bitboard threats = calcThreatenedSquaresPawnBulk<us>();
    Bitboard opMaterial = readMaterial<op>().combine();

    return threats & opMaterial;
}

template Bitboard Position::calcAvailableAttacksPawnBulk<Set::WHITE>(const KingMask& kingMask) const;
template Bitboard Position::calcAvailableAttacksPawnBulk<Set::BLACK>(const KingMask& kingMask) const;

template<Set us>
Bitboard
Position::calcAvailableMovesKnightBulk(const KingMask& kingMask) const
{
    auto moves = calcThreatenedSquaresKnightBulk<us>();

    if (kingMask.isChecked())
        return moves & kingMask.combined();

    return moves;
}

template Bitboard Position::calcAvailableMovesKnightBulk<Set::WHITE>(const KingMask& kingMask) const;
template Bitboard Position::calcAvailableMovesKnightBulk<Set::BLACK>(const KingMask& kingMask) const;

template<Set us>
Bitboard
Position::calcThreatenedSquaresKnightBulk() const
{
    Bitboard result = 0;
    const Bitboard ourMaterial = readMaterial<us>().combine();
    const Bitboard knights = readMaterial<us>()[knightId];
    if (knights == 0)
        return result;  // early out

    const u8 moveCount = ChessPieceDef::MoveCount(knightId);

    for (u8 moveIndx = 0; moveIndx < moveCount; ++moveIndx) {
        const u16 dir = ChessPieceDef::Attacks0x88(knightId, moveIndx);

        Bitboard knightsCopy = knights;
        while (knightsCopy.empty() == false) {
            byte curSqr = knightsCopy.popLsb();
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
            result |= sqrMask;
        }
    }

    return result & ~ourMaterial;
}

template Bitboard Position::calcThreatenedSquaresKnightBulk<Set::WHITE>() const;
template Bitboard Position::calcThreatenedSquaresKnightBulk<Set::BLACK>() const;

template<Set us, u8 pieceId>
Bitboard
Position::calcThreatenedSquaresBishopBulk() const
{
    const auto bounds = board_constants::boundsRelativeMasks[(size_t)us];

    Bitboard moves = 0;
    moves |= internalCalculateThreat<us, northeast, pieceId>(bounds[north] | bounds[east]);
    moves |= internalCalculateThreat<us, southeast, pieceId>(bounds[south] | bounds[east]);
    moves |= internalCalculateThreat<us, southwest, pieceId>(bounds[south] | bounds[west]);
    moves |= internalCalculateThreat<us, northwest, pieceId>(bounds[north] | bounds[west]);

    return moves;
}

template Bitboard Position::calcThreatenedSquaresBishopBulk<Set::WHITE, bishopId>() const;
template Bitboard Position::calcThreatenedSquaresBishopBulk<Set::BLACK, bishopId>() const;
template Bitboard Position::calcThreatenedSquaresBishopBulk<Set::WHITE, queenId>() const;
template Bitboard Position::calcThreatenedSquaresBishopBulk<Set::BLACK, queenId>() const;

template<Set us, u8 pieceId>
Bitboard
Position::calcThreatenedSquaresRookBulk() const
{
    const auto bounds = board_constants::boundsRelativeMasks[(size_t)us];

    Bitboard moves = 0;
    moves |= internalCalculateThreat<us, north, pieceId>(bounds[north]);
    moves |= internalCalculateThreat<us, east, pieceId>(bounds[east]);
    moves |= internalCalculateThreat<us, south, pieceId>(bounds[south]);
    moves |= internalCalculateThreat<us, west, pieceId>(bounds[west]);

    return moves;
}

template Bitboard Position::calcThreatenedSquaresRookBulk<Set::WHITE, rookId>() const;
template Bitboard Position::calcThreatenedSquaresRookBulk<Set::BLACK, rookId>() const;
template Bitboard Position::calcThreatenedSquaresRookBulk<Set::WHITE, queenId>() const;
template Bitboard Position::calcThreatenedSquaresRookBulk<Set::BLACK, queenId>() const;

template<Set us>
Bitboard
Position::calcThreatenedSquaresQueenBulk() const
{
    Bitboard moves = 0;

    moves |= calcThreatenedSquaresRookBulk<us, queenId>();
    moves |= calcThreatenedSquaresBishopBulk<us, queenId>();

    return moves;
}

template Bitboard Position::calcThreatenedSquaresQueenBulk<Set::WHITE>() const;
template Bitboard Position::calcThreatenedSquaresQueenBulk<Set::BLACK>() const;

template<Set us>
std::tuple<Bitboard, Bitboard>
Position::isolatePiece(u8 pieceId, Notation source, Bitboard movesbb, const KingMask& kingMask) const
{
    switch (pieceId) {
        case pawnId:
            return internalIsolatePawn<us>(source, movesbb, kingMask);
        case bishopId:
            return internalIsolateBishop<us>(source, movesbb, kingMask);
        case rookId:
            return internalIsolateRook<us>(source, movesbb);
        case knightId:
            return internalIsolateKnightMoves<us>(source, movesbb);
        case queenId: {
            auto [moves, captures] = internalIsolateBishop<us>(source, movesbb, kingMask);
            auto [rookMoves, rookCaptures] = internalIsolateRook<us>(source, movesbb);
            return {moves | rookMoves, captures | rookCaptures};
        }
        default:
            FATAL_ASSERT(false) << "Not implemented";
    }

    return {0, 0};
}

template std::tuple<Bitboard, Bitboard> Position::isolatePiece<Set::WHITE>(u8 pieceId, Notation source, Bitboard movesbb,
                                                                           const KingMask& kingMask) const;
template std::tuple<Bitboard, Bitboard> Position::isolatePiece<Set::BLACK>(u8 pieceId, Notation source, Bitboard movesbb,
                                                                           const KingMask& kingMask) const;

template<Set us>
std::tuple<Bitboard, Bitboard>
Position::internalIsolatePawn(Notation source, Bitboard movesbb, const KingMask& kingMask) const
{
    Bitboard opMatCombined = readMaterial<opposing_set<us>()>().combine() | m_enpassantState.readBitboard();
    Bitboard srcMask = Bitboard(squareMaskTable[source.index()]);

    if (srcMask & kingMask.combinedPins()) {
        movesbb &= kingMask.combinedPins();
        return {movesbb & ~opMatCombined, movesbb & opMatCombined};
    }

    if (readMaterial<us>()[pawnId].count() <= 1) {
        return {movesbb & ~opMatCombined, movesbb & opMatCombined};
    }

    const size_t usIndx = static_cast<size_t>(us);
    Bitboard isolatedbb = srcMask.shiftNorthRelative<us>();
    Bitboard unoccupied = ~(readMaterial<us>().combine() | opMatCombined);
    Bitboard doublePush = isolatedbb & pawn_constants::baseRank[usIndx] & unoccupied;
    isolatedbb |= doublePush.shiftNorthRelative<us>();

    Bitboard threatns;
    if ((srcMask & board_constants::boundsRelativeMasks[usIndx][west]).empty())
        threatns |= srcMask.shiftNorthWestRelative<us>();
    if ((srcMask & board_constants::boundsRelativeMasks[usIndx][east]).empty())
        threatns |= srcMask.shiftNorthEastRelative<us>();

    return {movesbb & isolatedbb, (opMatCombined & threatns)};
}

template std::tuple<Bitboard, Bitboard> Position::internalIsolatePawn<Set::WHITE>(Notation, Bitboard, const KingMask&) const;
template std::tuple<Bitboard, Bitboard> Position::internalIsolatePawn<Set::BLACK>(Notation, Bitboard, const KingMask&) const;

template<Set us>
std::tuple<Bitboard, Bitboard>
Position::internalIsolateKnightMoves(Notation source, Bitboard movesbb) const
{
    Bitboard opMatCombined = readMaterial<opposing_set<us>()>().combine();
    if (readMaterial<us>()[knightId].count() <= 1)
        return {movesbb & ~opMatCombined, movesbb & opMatCombined};

    Bitboard isolatedbb;

    const u8 moveCount = ChessPieceDef::MoveCount(knightId);
    for (u8 moveIndx = 0; moveIndx < moveCount; ++moveIndx) {
        byte curSqr = source.index();
        // build a 0x88 square out of current square.
        signed char sq0x88 = to0x88(curSqr);
        // do move
        const u16 dir = ChessPieceDef::Attacks0x88(knightId, moveIndx);
        sq0x88 += dir;
        if (sq0x88 & 0x88)  // validate move, are we still on the board?
            continue;

        // convert our sqr0x88 back to a square index
        curSqr = fr0x88(sq0x88);
        // build a square mask from current square
        u64 sqrMask = squareMaskTable[curSqr];
        isolatedbb |= sqrMask;
    }
    Bitboard ourMaterial = readMaterial<us>().combine();
    isolatedbb &= ~ourMaterial;

    return {isolatedbb & ~opMatCombined, isolatedbb & opMatCombined};
}

template<Set us>
std::tuple<Bitboard, Bitboard>
Position::internalIsolateBishop(Notation source, Bitboard movesbb, const KingMask& kingMask) const
{
    const Bitboard opMatCombined = readMaterial<opposing_set<us>()>().combine();
    // figure out if piece is pinned
    u64 srcMask = squareMaskTable[source.index()];
    bool pinned = srcMask & kingMask.combinedPins();
    if (pinned == true) {
        movesbb &= kingMask.combinedPins();
        return {movesbb & ~opMatCombined, movesbb & opMatCombined};
    }

    if (readMaterial<us>()[bishopId].count() <= 1) {
        return {movesbb & ~opMatCombined, movesbb & opMatCombined};
    }

    // figure out forward diagnoal
    u16 index = 7 + source.file - source.rank;
    u64 forwardDiag = board_constants::forwardDiagonalMasks[index];
    u16 bindex = source.file + source.rank;
    u64 backwardDiag = board_constants::backwardDiagonalMasks[bindex];

    u64 mask = forwardDiag | backwardDiag;
    u64 pieceMask = mask & readMaterial<us>()[bishopId].read();
    i32 count = intrinsics::popcnt(pieceMask);
    if (count == 1)
        return {movesbb & mask, 0};

    movesbb &= mask;
    pieceMask &= ~squareMaskTable[source.index()];
    i32 otherSquare = intrinsics::lsbIndex(pieceMask);
    Notation other(otherSquare);

    Bitboard excludeMask{};
    i32 nsDiff = otherSquare - source.index();
    i32 weDiff = diffWestEast(source, other);
    if (nsDiff < 0) {
        if (weDiff < 0)
            excludeMask = excludeMask.inclusiveFillSouthWest(other.file, other.rank);
        else
            excludeMask = excludeMask.inclusiveFillSouthEast(other.file, other.rank);
    }
    else {
        if (weDiff < 0)
            excludeMask = excludeMask.inclusiveFillNorthWest(other.file, other.rank);
        else
            excludeMask = excludeMask.inclusiveFillNorthEast(other.file, other.rank);
    }

    movesbb &= (~excludeMask).read();
    movesbb &= (forwardDiag | backwardDiag);
    return {movesbb & ~opMatCombined, movesbb & opMatCombined};
}

template std::tuple<Bitboard, Bitboard> Position::internalIsolateBishop<Set::WHITE>(Notation source, Bitboard movesbb,
                                                                                    const KingMask& kingMask) const;
template std::tuple<Bitboard, Bitboard> Position::internalIsolateBishop<Set::BLACK>(Notation source, Bitboard movesbb,
                                                                                    const KingMask& kingMask) const;

template<Set us>
std::tuple<Bitboard, Bitboard>
Position::internalIsolateRook(Notation source, Bitboard movesbb) const
{
    Bitboard opThreatenedPieces = readMaterial<opposing_set<us>()>().combine() & movesbb;
    if (readMaterial<us>()[rookId].count() <= 1)
        return {movesbb & ~opThreatenedPieces, opThreatenedPieces};

    Bitboard mask(board_constants::fileMasks[source.file] | board_constants::rankMasks[source.rank]);
    Bitboard pieceMask = readMaterial<us>()[rookId] & mask;
    if (pieceMask.count() == 1) {
        // rooks don't intersect so no need to isolate further.
        return {movesbb & mask, opThreatenedPieces & mask};
    }

    movesbb &= mask;
    pieceMask &= ~squareMaskTable[source.index()];
    Bitboard includeMask = squareMaskTable[source.index()];

    Bitboard usMaterial = readMaterial<us>().combine();

    // remove self from us material
    usMaterial &= ~squareMaskTable[source.index()];

    if (pieceMask & board_constants::fileMasks[source.file])  // on same file
    {
        while (!(includeMask & board_constants::rank7Mask) && !(includeMask.shiftNorth() & usMaterial))
            includeMask |= includeMask.shiftNorth();

        while (!(includeMask & board_constants::rank0Mask) && !(includeMask.shiftSouth() & usMaterial))
            includeMask |= includeMask.shiftSouth();

        Bitboard includedrank = movesbb & board_constants::rankMasks[source.rank];
        movesbb &= includeMask;
        movesbb |= includedrank;
    }
    else {
        while (!(includeMask & board_constants::fileaMask) && !(includeMask.shiftWest() & usMaterial))
            includeMask |= includeMask.shiftWest();

        while (!(includeMask & board_constants::filehMask) && !(includeMask.shiftEast() & usMaterial))
            includeMask |= includeMask.shiftEast();

        Bitboard includedfile = movesbb & board_constants::fileMasks[source.file];
        movesbb &= includeMask;
        movesbb |= includedfile;
    }

    return {movesbb & ~opThreatenedPieces, movesbb & opThreatenedPieces};
}

template std::tuple<Bitboard, Bitboard> Position::internalIsolateRook<Set::WHITE>(Notation source, Bitboard movesbb) const;
template std::tuple<Bitboard, Bitboard> Position::internalIsolateRook<Set::BLACK>(Notation source, Bitboard movesbb) const;

i32
Position::diffWestEast(Notation a, Notation b) const
{
    i32 a_flattened = mod_by_eight(a.index());
    i32 b_flattened = mod_by_eight(b.index());
    return b_flattened - a_flattened;
}