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
    m_castlingState(),
    m_enpassantState()
{
    m_materialMask = {};
}

Position&
Position::operator=(const Position& other)
{
    m_materialMask = other.m_materialMask;
    m_castlingState = other.m_castlingState;
    m_enpassantState = other.m_enpassantState;
    return *this;
}

bool
Position::IsValidSquare(signed short currSqr)
{
    if (currSqr < 0)
        return false;
    if (currSqr > 63) {
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
    m_materialMask.clear();
    m_enpassantState = {};
    m_castlingState = {};
}

bool
Position::empty() const
{
    return m_materialMask.empty();
}

bool
Position::ClearPiece(ChessPiece piece, Square target)
{
    Bitboard pieceMask;
    pieceMask[target] = true;
    m_materialMask.clear(pieceMask, piece.getSet(), piece.index());
    return true;
}

bool
Position::PlacePiece(ChessPiece piece, Square target)
{
    Bitboard piecebb;
    piecebb[target] = true;
    m_materialMask.write(piecebb, piece.getSet(), piece.index());

    return true;
}

ChessPiece
Position::readPieceAt(Square sqr) const
{
    Bitboard mask(UINT64_C(1) << (u8)sqr);

    for (byte set = 0; set < 2; ++set) {
        if (m_materialMask.m_set[set] & mask)
        {
            // unrolled this for loop in an attempt to make it quicker.
            if (m_materialMask.pawns() & mask)
                return piece_constants::pieces[set][pawnId];
            else if (m_materialMask.knights() & mask)
                return piece_constants::pieces[set][knightId];
            else if (m_materialMask.bishops() & mask)
                return piece_constants::pieces[set][bishopId];
            else if (m_materialMask.rooks() & mask)
                return piece_constants::pieces[set][rookId];
            else if (m_materialMask.queens() & mask)
                return piece_constants::pieces[set][queenId];
            else if (m_materialMask.kings() & mask)
                return piece_constants::pieces[set][kingId];
        }
    }
    return ChessPiece::None();
}

MutableMaterialProxy
Position::materialEditor(Set set, PieceType pType)
{
    return MutableMaterialProxy(&m_materialMask.m_set[static_cast<i8>(set)], &m_materialMask.m_material[toPieceId(pType)]);
}


template<Set us>
KingPinThreats Position::calcKingMask() const {
    constexpr Set op = opposing_set<us>();
    auto slidingMask = calcMaterialSlidingMasksBulk<op>();    
    Square kingSqr = static_cast<Square>(m_materialMask.kings<us>().lsbIndex());
    Square opKingSqr = static_cast<Square>(m_materialMask.kings<op>().lsbIndex());
    
    KingPinThreats ret;
    ret.evaluate<us>(kingSqr, *this, slidingMask);
    ret.calculateOpponentOpenAngles<op>(opKingSqr, *this);
    return ret;
}

template KingPinThreats Position::calcKingMask<Set::WHITE>() const;
template KingPinThreats Position::calcKingMask<Set::BLACK>() const;

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
    u64 combMat = m_materialMask.combine().read();

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

template<Set us, bool captures>
Bitboard Position::calcAvailableMovesPawnBulk(const KingPinThreats& kingMask) const {
    const size_t usIndx = static_cast<size_t>(us);
    const Bitboard usMat = m_materialMask.combine<us>();
    const Bitboard opMat = m_materialMask.combine<opposing_set<us>()>();
    const Bitboard unoccupied(~(usMat | opMat));
    const Bitboard piecebb = m_materialMask.pawns<us>();

    Bitboard mvsbb;
    mvsbb = piecebb.shiftNorthRelative<us>();
    Bitboard doublePush = mvsbb & pawn_constants::baseRank[usIndx] & unoccupied;
    mvsbb |= doublePush.shiftNorthRelative<us>();

    mvsbb &= unoccupied;

    mvsbb |= (opMat | m_enpassantState.readBitboard()) & calcThreatenedSquaresPawnBulk<us>();

    if (kingMask.isChecked()) {
        Bitboard checksMask(kingMask.checks());
        auto otherMask = squareMaskTable[(u32)m_enpassantState.readTarget()];
        if (checksMask & otherMask) {
            checksMask |= m_enpassantState.readBitboard();
        }
        mvsbb &= checksMask;
    }

    if constexpr (captures == true)
        mvsbb &= opMat;

    return mvsbb;
}

template Bitboard Position::calcAvailableMovesPawnBulk<Set::WHITE, true>(const KingPinThreats& kingMask) const;
template Bitboard Position::calcAvailableMovesPawnBulk<Set::WHITE, false>(const KingPinThreats& kingMask) const;
template Bitboard Position::calcAvailableMovesPawnBulk<Set::BLACK, true>(const KingPinThreats& kingMask) const;
template Bitboard Position::calcAvailableMovesPawnBulk<Set::BLACK, false>(const KingPinThreats& kingMask) const;

template<Set us>
Bitboard
Position::calcThreatenedSquaresPawnBulk() const
{
    const size_t usIndx = static_cast<size_t>(us);
    Bitboard piecebb = m_materialMask.pawns<us>();

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

template<Set us, bool captures, Set op>
Bitboard
Position::calcAvailableMovesKing(byte castlingRights) const
{
    bool constexpr includeMaterial = false;
    bool constexpr pierceKing = true;
    Bitboard threatened = calcThreatenedSquares<op, includeMaterial, pierceKing>();
    Bitboard moves = calcThreatenedSquaresKing<us>();
    // remove any squares blocked by our own pieces.
    moves &= ~m_materialMask.combine<us>();
    moves &= ~threatened;
    if ((threatened & m_materialMask.kings<us>()).empty())
        moves |= Castling((byte)us, castlingRights, threatened.read());
    if constexpr (captures == true)
        moves &= m_materialMask.combine<op>();
    return moves;
}

template Bitboard Position::calcAvailableMovesKing<Set::WHITE, true, Set::BLACK>(byte) const;
template Bitboard Position::calcAvailableMovesKing<Set::WHITE, false, Set::BLACK>(byte) const;
template Bitboard Position::calcAvailableMovesKing<Set::BLACK, true, Set::WHITE>(byte) const;
template Bitboard Position::calcAvailableMovesKing<Set::BLACK, false, Set::WHITE>(byte) const;

template<Set us>
Bitboard
Position::calcThreatenedSquaresKing() const
{
    Bitboard moves;
    const auto kingbb = m_materialMask.kings<us>();
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

    return moves;
}

template Bitboard Position::calcThreatenedSquaresKing<Set::WHITE>() const;
template Bitboard Position::calcThreatenedSquaresKing<Set::BLACK>() const;

template<Set us, bool captures, u8 pieceId>
Bitboard
Position::calcAvailableMovesBishopBulk(const KingPinThreats& kingMask) const
{
    const Bitboard materialbb = readMaterial().combine<us>();
    Bitboard moves = calcThreatenedSquaresBishopBulk<us, pieceId>();

    if (kingMask.isChecked())
        moves &= kingMask.checks();
    else
        moves ^= (materialbb & moves);

    if constexpr (captures == true)
        moves &= readMaterial().combine<opposing_set<us>()>();

    return moves;
}

template Bitboard Position::calcAvailableMovesBishopBulk<Set::WHITE, true, bishopId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesBishopBulk<Set::BLACK, true, bishopId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesBishopBulk<Set::WHITE, true, queenId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesBishopBulk<Set::BLACK, true, queenId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesBishopBulk<Set::WHITE, false, bishopId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesBishopBulk<Set::BLACK, false, bishopId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesBishopBulk<Set::WHITE, false, queenId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesBishopBulk<Set::BLACK, false, queenId>(const KingPinThreats&) const;

template<Set us, bool captures, u8 pieceId>
Bitboard
Position::calcAvailableMovesRookBulk(const KingPinThreats& kingMask) const
{
    const Bitboard materialbb = readMaterial().combine<us>();

    Bitboard moves = calcThreatenedSquaresRookBulk<us, pieceId>();

    if (kingMask.isChecked())
        moves &= kingMask.checks();
    else
        moves ^= (materialbb & moves);

    if constexpr (captures == true)
        moves &= readMaterial().combine<opposing_set<us>()>();

    return moves;
}

template Bitboard Position::calcAvailableMovesRookBulk<Set::WHITE, true, rookId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesRookBulk<Set::BLACK, true, rookId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesRookBulk<Set::WHITE, true, queenId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesRookBulk<Set::BLACK, true, queenId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesRookBulk<Set::WHITE, false, rookId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesRookBulk<Set::BLACK, false, rookId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesRookBulk<Set::WHITE, false, queenId>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesRookBulk<Set::BLACK, false, queenId>(const KingPinThreats&) const;

template<Set us, bool captures>
Bitboard
Position::calcAvailableMovesQueenBulk(const KingPinThreats& kingMask) const
{
    Bitboard moves = 0;
    moves |= calcAvailableMovesBishopBulk<us, captures, queenId>(kingMask);
    moves |= calcAvailableMovesRookBulk<us, captures, queenId>(kingMask);
    return moves;
}

template Bitboard Position::calcAvailableMovesQueenBulk<Set::WHITE, true>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesQueenBulk<Set::WHITE, false>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesQueenBulk<Set::BLACK, true>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesQueenBulk<Set::BLACK, false>(const KingPinThreats&) const;

template<Set us, bool captures>
Bitboard
Position::calcAvailableMovesKnightBulk(const KingPinThreats& kingMask) const
{
    auto moves = calcThreatenedSquaresKnightBulk<us>();
    const Bitboard ourMaterial = readMaterial().combine<us>();

    moves &= ~ourMaterial;

    if (kingMask.isChecked())
        return moves & kingMask.checks();

    if constexpr (captures == true)
        return moves & readMaterial().combine<opposing_set<us>()>();

    return moves;
}

template Bitboard Position::calcAvailableMovesKnightBulk<Set::WHITE, true>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesKnightBulk<Set::BLACK, true>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesKnightBulk<Set::WHITE, false>(const KingPinThreats&) const;
template Bitboard Position::calcAvailableMovesKnightBulk<Set::BLACK, false>(const KingPinThreats&) const;

template<Set us>
Bitboard
Position::calcThreatenedSquaresKnightBulk() const
{
    Bitboard result = 0;
    const Bitboard knights = m_materialMask.knights<us>();
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

    return result;  //& ~ourMaterial;
}

template Bitboard Position::calcThreatenedSquaresKnightBulk<Set::WHITE>() const;
template Bitboard Position::calcThreatenedSquaresKnightBulk<Set::BLACK>() const;

template<Set us>
Bitboard Position::calcThreatenedDiagonals() const
{
    const auto bounds = board_constants::boundsRelativeMasks[(size_t)us];
    Bitboard diagonalPieces = readMaterial().queens<us>() | readMaterial().bishops<us>();

    Bitboard moves = 0;
    moves |= internalCalculateThreatBulk<us, northeast>(bounds[north] | bounds[east], diagonalPieces);
    moves |= internalCalculateThreatBulk<us, southeast>(bounds[south] | bounds[east], diagonalPieces);
    moves |= internalCalculateThreatBulk<us, southwest>(bounds[south] | bounds[west], diagonalPieces);
    moves |= internalCalculateThreatBulk<us, northwest>(bounds[north] | bounds[west], diagonalPieces);

    return moves;
}

template Bitboard Position::calcThreatenedDiagonals<Set::WHITE>() const;
template Bitboard Position::calcThreatenedDiagonals<Set::BLACK>() const;

template<Set us>
Bitboard Position::calcThreatenedOrthogonals() const
{
    const auto bounds = board_constants::boundsRelativeMasks[(size_t)us];
    Bitboard orthogonalPieces = readMaterial().queens<us>() | readMaterial().rooks<us>();

    Bitboard moves = 0;
    moves |= internalCalculateThreatBulk<us, north>(bounds[north], orthogonalPieces);
    moves |= internalCalculateThreatBulk<us, east>(bounds[east], orthogonalPieces);
    moves |= internalCalculateThreatBulk<us, south>(bounds[south], orthogonalPieces);
    moves |= internalCalculateThreatBulk<us, west>(bounds[west], orthogonalPieces);

    return moves;
}

template Bitboard Position::calcThreatenedOrthogonals<Set::WHITE>() const;
template Bitboard Position::calcThreatenedOrthogonals<Set::BLACK>() const;

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
Position::isolatePiece(u8 pieceId, Notation source, Bitboard movesbb, const KingPinThreats& kingMask) const
{
    switch (pieceId) {
    case pawnId:
        return internalIsolatePawn<us>(source, movesbb, kingMask);
    case bishopId:
        return internalIsolateBishop<us>(source, movesbb, kingMask);
    case rookId:
        return internalIsolateRook<us>(source, movesbb, kingMask);
    case knightId:
        return internalIsolateKnightMoves<us>(source, movesbb, kingMask);
    case queenId: {
        auto [moves, captures] = internalIsolateBishop<us>(source, movesbb, kingMask, queenId);
        auto [rookMoves, rookCaptures] = internalIsolateRook<us>(source, movesbb, kingMask, queenId);
        return { moves | rookMoves, captures | rookCaptures };
    }
    default:
        FATAL_ASSERT(false) << "Not implemented";
    }

    return { 0, 0 };
}

template std::tuple<Bitboard, Bitboard> Position::isolatePiece<Set::WHITE>(u8, Notation, Bitboard, const KingPinThreats&) const;
template std::tuple<Bitboard, Bitboard> Position::isolatePiece<Set::BLACK>(u8, Notation, Bitboard, const KingPinThreats&) const;

template<Set us>
std::tuple<Bitboard, Bitboard>
Position::internalIsolatePawn(Notation source, Bitboard movesbb, const KingPinThreats& pinThreats) const
{
    const size_t usIndx = static_cast<size_t>(us);

    Bitboard opMatCombined = readMaterial().combine<opposing_set<us>()>() | m_enpassantState.readBitboard();
    Bitboard srcMask = Bitboard(squareMaskTable[source.index()]);

    const Bitboard pinned = pinThreats.pinned(srcMask);

    // special case for when there is a enpassant available.
    if (m_enpassantState) {
        // Bitboard enPassantTarget(squareMaskTable[(int)m_enpassantState.readTarget()]);
        Bitboard potentialPin(pinThreats.readEnPassantMask() & srcMask);
        if (potentialPin) {
            opMatCombined ^= m_enpassantState.readBitboard();
        }
    }

    Bitboard threatns;
    if ((srcMask & board_constants::boundsRelativeMasks[usIndx][west]).empty())
        threatns |= srcMask.shiftNorthWestRelative<us>();
    if ((srcMask & board_constants::boundsRelativeMasks[usIndx][east]).empty())
        threatns |= srcMask.shiftNorthEastRelative<us>();

    Bitboard isolatedbb = srcMask.shiftNorthRelative<us>();
    Bitboard unoccupied = ~(readMaterial().combine<us>() | opMatCombined);
    Bitboard doublePush = isolatedbb & pawn_constants::baseRank[usIndx] & unoccupied;
    isolatedbb |= doublePush.shiftNorthRelative<us>();
    isolatedbb &= unoccupied;
    if (srcMask & pinned) {
        isolatedbb &= pinned;
        threatns &= pinned;
    }
    return { movesbb & isolatedbb, movesbb & opMatCombined & threatns };
}

template std::tuple<Bitboard, Bitboard> Position::internalIsolatePawn<Set::WHITE>(Notation, Bitboard,
    const KingPinThreats&) const;
template std::tuple<Bitboard, Bitboard> Position::internalIsolatePawn<Set::BLACK>(Notation, Bitboard,
    const KingPinThreats&) const;

template<Set us>
std::tuple<Bitboard, Bitboard>
Position::internalIsolateKnightMoves(Notation source, Bitboard movesbb, const KingPinThreats& kingMask) const
{
    Bitboard opMatCombined = readMaterial().combine<opposing_set<us>()>();

    // figure out if piece is pinned
    u64 srcMask = squareMaskTable[source.index()];
    const Bitboard pinned = kingMask.pinned(srcMask);
    if (pinned.empty() == false) {
        movesbb &= pinned;
        // return {movesbb & ~opMatCombined, movesbb & opMatCombined};
    }

    if (readMaterial().knights<us>().count() <= 1)
        return { movesbb & ~opMatCombined, movesbb & opMatCombined };

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
    Bitboard ourMaterial = readMaterial().combine<us>();
    isolatedbb &= ~ourMaterial;

    return { movesbb & isolatedbb & ~opMatCombined, movesbb & isolatedbb & opMatCombined };
}

template<Set us>
std::tuple<Bitboard, Bitboard>
Position::internalIsolateBishop(Notation source, Bitboard movesbb, const KingPinThreats& kingMask, i8 pieceIndex) const
{
    const Bitboard opMatCombined = readMaterial().combine<opposing_set<us>()>();
    // figure out if piece is pinned
    u64 srcMask = squareMaskTable[source.index()];
    const Bitboard pinned = kingMask.pinned(srcMask);
    if (pinned.empty() == false) {
        movesbb &= pinned;
    }

    if (readMaterial().read<us>(pieceIndex).count() <= 1) {
        return { movesbb & ~opMatCombined, movesbb & opMatCombined };
    }

    if (pieceIndex == bishopId) {
        // isolate using dark & light squares
        Bitboard material = readMaterial().bishops<us>();
        Bitboard darkSquaredMaterial = material & board_constants::darkSquares;
        Bitboard lightSquaredMaterial = material & board_constants::lightSquares;
        if (srcMask & darkSquaredMaterial && darkSquaredMaterial.count() == 1)
            return { movesbb & board_constants::darkSquares & ~opMatCombined,
                    movesbb & board_constants::darkSquares & opMatCombined };
        else if (srcMask & lightSquaredMaterial && lightSquaredMaterial.count() == 1)
            return { movesbb & board_constants::lightSquares & ~opMatCombined,
                    movesbb & board_constants::lightSquares & opMatCombined };
    }

    const auto bounds = board_constants::boundsRelativeMasks[(size_t)us];
    const Bitboard usMaterial = readMaterial().combine<us>();

    Bitboard moves = 0;
    moves |= internalCalculateThreat<us, northeast, bishopId>(bounds[north] | bounds[east], srcMask, usMaterial, opMatCombined);
    moves |= internalCalculateThreat<us, southeast, bishopId>(bounds[south] | bounds[east], srcMask, usMaterial, opMatCombined);
    moves |= internalCalculateThreat<us, southwest, bishopId>(bounds[south] | bounds[west], srcMask, usMaterial, opMatCombined);
    moves |= internalCalculateThreat<us, northwest, bishopId>(bounds[north] | bounds[west], srcMask, usMaterial, opMatCombined);
    movesbb &= moves;

    return { movesbb & ~opMatCombined, movesbb & opMatCombined };
}

template std::tuple<Bitboard, Bitboard> Position::internalIsolateBishop<Set::WHITE>(Notation, Bitboard, const KingPinThreats&,
    i8) const;
template std::tuple<Bitboard, Bitboard> Position::internalIsolateBishop<Set::BLACK>(Notation, Bitboard, const KingPinThreats&,
    i8) const;

template<Set us>
std::tuple<Bitboard, Bitboard>
Position::internalIsolateRook(Notation source, Bitboard movesbb, const KingPinThreats& kingMask, i8 pieceIndx) const
{
    const Bitboard opThreatenedPieces = readMaterial().combine<opposing_set<us>()>() & movesbb;
    const Bitboard opMatCombined = readMaterial().combine<opposing_set<us>()>();

    u64 srcMask = squareMaskTable[source.index()];
    Bitboard pinned = kingMask.pinned(srcMask);
    if (pinned.empty() == false) {
        movesbb &= pinned;
        // return {movesbb & ~opMatCombined, movesbb & opThreatenedPieces};
    }

    if (readMaterial().read<us>(pieceIndx).count() <= 1)
        return { movesbb & ~opThreatenedPieces, movesbb & opThreatenedPieces };

    Bitboard mask(board_constants::fileMasks[source.file] | board_constants::rankMasks[source.rank]);
    Bitboard pieceMask = readMaterial().read<us>(pieceIndx) & mask;
    movesbb &= mask;

    Bitboard otherRooks = pieceMask & ~squareMaskTable[source.index()];
    Bitboard includeMask = squareMaskTable[source.index()];
    Bitboard usMaterial = readMaterial().combine<us>();
    // remove self from us material
    usMaterial &= ~squareMaskTable[source.index()];

    const auto bounds = board_constants::boundsRelativeMasks[(size_t)us];
    if (pieceIndx == rookId) {
        if (otherRooks & board_constants::fileMasks[source.file])  // on same file
        {
            while (!(includeMask & board_constants::rank7Mask) && !(includeMask.shiftNorth() & usMaterial))
                includeMask |= includeMask.shiftNorth();

            while (!(includeMask & board_constants::rank0Mask) && !(includeMask.shiftSouth() & usMaterial))
                includeMask |= includeMask.shiftSouth();

            Bitboard includedrank = movesbb & board_constants::rankMasks[source.rank];
            movesbb &= includeMask;
            movesbb |= includedrank;
        }
        else if (otherRooks & board_constants::rankMasks[source.rank]) {  // on same rank
            while (!(includeMask & board_constants::fileaMask) && !(includeMask.shiftWest() & usMaterial))
                includeMask |= includeMask.shiftWest();

            while (!(includeMask & board_constants::filehMask) && !(includeMask.shiftEast() & usMaterial))
                includeMask |= includeMask.shiftEast();

            Bitboard includedfile = movesbb & board_constants::fileMasks[source.file];
            movesbb &= includeMask;
            movesbb |= includedfile;
        }
    }
    Bitboard thisSrcMask = squareMaskTable[source.index()];

    Bitboard moves = 0;
    moves |= internalCalculateThreat<us, north, rookId>(bounds[north], thisSrcMask, usMaterial, opMatCombined);
    moves |= internalCalculateThreat<us, east, rookId>(bounds[east], thisSrcMask, usMaterial, opMatCombined);
    moves |= internalCalculateThreat<us, south, rookId>(bounds[south], thisSrcMask, usMaterial, opMatCombined);
    moves |= internalCalculateThreat<us, west, rookId>(bounds[west], thisSrcMask, usMaterial, opMatCombined);
    movesbb &= moves;

    return { movesbb & ~opThreatenedPieces, movesbb & opThreatenedPieces };
}

template std::tuple<Bitboard, Bitboard> Position::internalIsolateRook<Set::WHITE>(Notation, Bitboard, const KingPinThreats&,
    i8) const;
template std::tuple<Bitboard, Bitboard> Position::internalIsolateRook<Set::BLACK>(Notation, Bitboard, const KingPinThreats&,
    i8) const;

i32
Position::diffWestEast(Notation a, Notation b) const
{
    i32 a_flattened = mod_by_eight(a.index());
    i32 b_flattened = mod_by_eight(b.index());
    return b_flattened - a_flattened;
}