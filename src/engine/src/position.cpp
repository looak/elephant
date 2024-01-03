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
Position::empty() const
{
    return m_material[0].combine().empty() && m_material[1].combine().empty();
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

ChessPiece
Position::readPieceAt(Square sqr) const
{
    for (byte set = 0; set < 2; ++set) {
        for (byte pieceId = 0; pieceId < 6; ++pieceId) {
            if (m_material[set].material[pieceId][sqr])
                return ChessPiece(set, pieceId);
        }
    }
    return ChessPiece::None();
}

template<Set us>
KingPinThreats
Position::calcKingMask() const
{
    constexpr Set op = opposing_set<us>();
    auto slidingMask = calcMaterialSlidingMasksBulk<op>();
    auto king = ChessPiece(us, PieceType::KING);
    auto kingSqr = m_material[(size_t)us].material[kingId].lsbIndex();
    auto ret = calcKingMask(king, Notation(kingSqr), slidingMask);

    auto opKingSqr = m_material[(size_t)op].kings().lsbIndex();
    ret.calculateOpponentOpenAngles(op, Notation(opKingSqr), *this);

    return ret;
}

template KingPinThreats Position::calcKingMask<Set::WHITE>() const;
template KingPinThreats Position::calcKingMask<Set::BLACK>() const;

KingPinThreats
Position::calcKingMask(ChessPiece king, Notation source, const SlidingMaterialMasks& opponentSlidingMask) const
{
    KingPinThreats ret;
    ret.evaluate(king.getSet(), source, *this, opponentSlidingMask);
    return ret;
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
    u64 combMat = readMaterial<Set::WHITE>().combine().read() | readMaterial<Set::BLACK>().combine().read();

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

Bitboard
Position::SlidingMaterialCombined(byte set) const
{
    return (m_material[set].material[bishopId] | m_material[set].material[rookId] | m_material[set].material[queenId]);
}

template<Set us>
Bitboard
Position::calcAvailableMovesPawnBulk(const KingPinThreats& kingMask, bool captures) const
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

    if (kingMask.isChecked()) {
        Bitboard checksMask(kingMask.checks());
        auto otherMask = squareMaskTable[(u32)m_enpassantState.readTarget()];
        if (checksMask & otherMask) {
            checksMask |= m_enpassantState.readBitboard();
        }
        mvsbb &= checksMask;
    }

    if (captures == true)
        mvsbb &= opMat;

    return mvsbb;
}

template Bitboard Position::calcAvailableMovesPawnBulk<Set::WHITE>(const KingPinThreats& kingMask, bool captures) const;
template Bitboard Position::calcAvailableMovesPawnBulk<Set::BLACK>(const KingPinThreats& kingMask, bool captures) const;

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
Position::calcAvailableMovesKing(byte castlingRights, bool captures) const
{
    bool constexpr includeMaterial = false;
    bool constexpr pierceKing = true;
    Bitboard treatened = calcThreatenedSquares<op, includeMaterial, pierceKing>();
    Bitboard moves = calcThreatenedSquaresKing<us>();
    moves &= ~treatened;
    moves |= Castling((byte)us, castlingRights, treatened.read());
    if (captures == true)
        moves &= readMaterial<op>().combine();
    return moves;
}

template Bitboard Position::calcAvailableMovesKing<Set::WHITE, Set::BLACK>(byte, bool) const;
template Bitboard Position::calcAvailableMovesKing<Set::BLACK, Set::WHITE>(byte, bool) const;

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
Position::calcAvailableMovesBishopBulk(const KingPinThreats& kingMask, bool captures) const
{
    const Bitboard materialbb = readMaterial<us>().combine();
    Bitboard moves = calcThreatenedSquaresBishopBulk<us, pieceId>();

    if (kingMask.isChecked())
        moves &= kingMask.checks();
    else
        moves ^= (materialbb & moves);

    if (captures == true)
        moves &= readMaterial<opposing_set<us>()>().combine();

    return moves;
}

template Bitboard Position::calcAvailableMovesBishopBulk<Set::WHITE, bishopId>(const KingPinThreats&, bool) const;
template Bitboard Position::calcAvailableMovesBishopBulk<Set::BLACK, bishopId>(const KingPinThreats&, bool) const;
template Bitboard Position::calcAvailableMovesBishopBulk<Set::WHITE, queenId>(const KingPinThreats&, bool) const;
template Bitboard Position::calcAvailableMovesBishopBulk<Set::BLACK, queenId>(const KingPinThreats&, bool) const;

template<Set us, u8 pieceId>
Bitboard
Position::calcAvailableMovesRookBulk(const KingPinThreats& kingMask, bool captures) const
{
    const Bitboard materialbb = readMaterial<us>().combine();

    Bitboard moves = calcThreatenedSquaresRookBulk<us, pieceId>();

    if (kingMask.isChecked())
        moves &= kingMask.checks();
    else
        moves ^= (materialbb & moves);

    if (captures == true)
        moves &= readMaterial<opposing_set<us>()>().combine();

    return moves;
}

template Bitboard Position::calcAvailableMovesRookBulk<Set::WHITE, rookId>(const KingPinThreats&, bool) const;
template Bitboard Position::calcAvailableMovesRookBulk<Set::BLACK, rookId>(const KingPinThreats&, bool) const;
template Bitboard Position::calcAvailableMovesRookBulk<Set::WHITE, queenId>(const KingPinThreats&, bool) const;
template Bitboard Position::calcAvailableMovesRookBulk<Set::BLACK, queenId>(const KingPinThreats&, bool) const;

template<Set us>
Bitboard
Position::calcAvailableMovesQueenBulk(const KingPinThreats& kingMask, bool captures) const
{
    Bitboard moves = 0;
    moves |= calcAvailableMovesBishopBulk<us, queenId>(kingMask, captures);
    moves |= calcAvailableMovesRookBulk<us, queenId>(kingMask, captures);
    return moves;
}

template Bitboard Position::calcAvailableMovesQueenBulk<Set::WHITE>(const KingPinThreats&, bool) const;
template Bitboard Position::calcAvailableMovesQueenBulk<Set::BLACK>(const KingPinThreats&, bool) const;

template<Set us>
Bitboard
Position::calcAvailableAttacksPawnBulk(const KingPinThreats&) const
{
    constexpr Set op = opposing_set<us>();

    Bitboard threats = calcThreatenedSquaresPawnBulk<us>();
    Bitboard opMaterial = readMaterial<op>().combine();

    return threats & opMaterial;
}

template Bitboard Position::calcAvailableAttacksPawnBulk<Set::WHITE>(const KingPinThreats& kingMask) const;
template Bitboard Position::calcAvailableAttacksPawnBulk<Set::BLACK>(const KingPinThreats& kingMask) const;

template<Set us>
Bitboard
Position::calcAvailableMovesKnightBulk(const KingPinThreats& kingMask, bool captures) const
{
    auto moves = calcThreatenedSquaresKnightBulk<us>();

    if (kingMask.isChecked())
        return moves & kingMask.checks();

    if (captures == true)
        return moves & readMaterial<opposing_set<us>()>().combine();

    return moves;
}

template Bitboard Position::calcAvailableMovesKnightBulk<Set::WHITE>(const KingPinThreats& kingMask, bool captures) const;
template Bitboard Position::calcAvailableMovesKnightBulk<Set::BLACK>(const KingPinThreats& kingMask, bool captures) const;

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
            return {moves | rookMoves, captures | rookCaptures};
        }
        default:
            FATAL_ASSERT(false) << "Not implemented";
    }

    return {0, 0};
}

template std::tuple<Bitboard, Bitboard> Position::isolatePiece<Set::WHITE>(u8, Notation, Bitboard, const KingPinThreats&) const;
template std::tuple<Bitboard, Bitboard> Position::isolatePiece<Set::BLACK>(u8, Notation, Bitboard, const KingPinThreats&) const;

template<Set us>
std::tuple<Bitboard, Bitboard>
Position::internalIsolatePawn(Notation source, Bitboard movesbb, const KingPinThreats& pinThreats) const
{
    const size_t usIndx = static_cast<size_t>(us);
    const size_t opIndx = static_cast<size_t>(opposing_set<us>());

    Bitboard opMatCombined = readMaterial<opposing_set<us>()>().combine() | m_enpassantState.readBitboard();
    Bitboard srcMask = Bitboard(squareMaskTable[source.index()]);

    // const Bitboard checks = pinThreats.checks();
    const Bitboard pins = pinThreats.pins();

    // special case for when there is a enpassant available.
    if (m_enpassantState) {
        Bitboard enPassantTarget(squareMaskTable[(int)m_enpassantState.readTarget()]);
        Bitboard potentialPin(pinThreats.readOpenAngles()[0] & enPassantTarget &
                              board_constants::enPassantRankRelative[opIndx]);
        if (potentialPin) {
            opMatCombined ^= m_enpassantState.readBitboard();
        }
    }

    Bitboard threatns;
    if ((srcMask & board_constants::boundsRelativeMasks[usIndx][west]).empty())
        threatns |= srcMask.shiftNorthWestRelative<us>();
    if ((srcMask & board_constants::boundsRelativeMasks[usIndx][east]).empty())
        threatns |= srcMask.shiftNorthEastRelative<us>();

    // there is probably a smarter way to do this with all my bitboards,
    // I just don't know how. Idea is to remove all the if statemetns.
    if (srcMask & pins) {
        movesbb &= pins;
        return {movesbb & ~opMatCombined & ~threatns, movesbb & opMatCombined & threatns};
    }

    if (readMaterial<us>()[pawnId].count() <= 1) {
        return {movesbb & ~opMatCombined, movesbb & opMatCombined};
    }

    Bitboard isolatedbb = srcMask.shiftNorthRelative<us>();
    Bitboard unoccupied = ~(readMaterial<us>().combine() | opMatCombined);
    Bitboard doublePush = isolatedbb & pawn_constants::baseRank[usIndx] & unoccupied;
    isolatedbb |= doublePush.shiftNorthRelative<us>();
    isolatedbb &= unoccupied;

    return {movesbb & isolatedbb, (opMatCombined & threatns)};
}

template std::tuple<Bitboard, Bitboard> Position::internalIsolatePawn<Set::WHITE>(Notation, Bitboard,
                                                                                  const KingPinThreats&) const;
template std::tuple<Bitboard, Bitboard> Position::internalIsolatePawn<Set::BLACK>(Notation, Bitboard,
                                                                                  const KingPinThreats&) const;

template<Set us>
std::tuple<Bitboard, Bitboard>
Position::internalIsolateKnightMoves(Notation source, Bitboard movesbb, const KingPinThreats& kingMask) const
{
    Bitboard opMatCombined = readMaterial<opposing_set<us>()>().combine();

    // figure out if piece is pinned
    u64 srcMask = squareMaskTable[source.index()];
    bool pinned = srcMask & kingMask.pins();
    if (pinned == true) {
        movesbb &= kingMask.pins();
        return {movesbb & ~opMatCombined, movesbb & opMatCombined};
    }

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

    return {movesbb & isolatedbb & ~opMatCombined, movesbb & isolatedbb & opMatCombined};
}

template<Set us>
std::tuple<Bitboard, Bitboard>
Position::internalIsolateBishop(Notation source, Bitboard movesbb, const KingPinThreats& kingMask, i8 pieceIndex) const
{
    const Bitboard opMatCombined = readMaterial<opposing_set<us>()>().combine();
    // figure out if piece is pinned
    u64 srcMask = squareMaskTable[source.index()];
    bool pinned = srcMask & kingMask.pins();
    if (pinned == true) {
        movesbb &= kingMask.pins();
        return {movesbb & ~opMatCombined, movesbb & opMatCombined};
    }

    if (readMaterial<us>()[pieceIndex].count() <= 1) {
        return {movesbb & ~opMatCombined, movesbb & opMatCombined};
    }

    // isolate using dark & light squares
    Bitboard material = readMaterial<us>()[pieceIndex];
    Bitboard darkSquaredMaterial = material & board_constants::darkSquares;
    Bitboard lightSquaredMaterial = material & board_constants::lightSquares;
    if (srcMask & darkSquaredMaterial && darkSquaredMaterial.count() == 1)
        return {movesbb & board_constants::darkSquares & ~opMatCombined,
                movesbb & board_constants::darkSquares & opMatCombined};
    else if (srcMask & lightSquaredMaterial && lightSquaredMaterial.count() == 1)
        return {movesbb & board_constants::lightSquares & ~opMatCombined,
                movesbb & board_constants::lightSquares & opMatCombined};

    const auto bounds = board_constants::boundsRelativeMasks[(size_t)us];
    const Bitboard usMaterial = readMaterial<us>().combine();

    Bitboard moves = 0;
    moves |= internalCalculateThreat<us, northeast, bishopId>(bounds[north] | bounds[east], srcMask, usMaterial, opMatCombined);
    moves |= internalCalculateThreat<us, southeast, bishopId>(bounds[south] | bounds[east], srcMask, usMaterial, opMatCombined);
    moves |= internalCalculateThreat<us, southwest, bishopId>(bounds[south] | bounds[west], srcMask, usMaterial, opMatCombined);
    moves |= internalCalculateThreat<us, northwest, bishopId>(bounds[north] | bounds[west], srcMask, usMaterial, opMatCombined);
    movesbb &= moves;

    return {movesbb & ~opMatCombined, movesbb & opMatCombined};
}

template std::tuple<Bitboard, Bitboard> Position::internalIsolateBishop<Set::WHITE>(Notation, Bitboard, const KingPinThreats&,
                                                                                    i8) const;
template std::tuple<Bitboard, Bitboard> Position::internalIsolateBishop<Set::BLACK>(Notation, Bitboard, const KingPinThreats&,
                                                                                    i8) const;

template<Set us>
std::tuple<Bitboard, Bitboard>
Position::internalIsolateRook(Notation source, Bitboard movesbb, const KingPinThreats& kingMask, i8 pieceIndx) const
{
    Bitboard opThreatenedPieces = readMaterial<opposing_set<us>()>().combine() & movesbb;
    const Bitboard opMatCombined = readMaterial<opposing_set<us>()>().combine();

    u64 srcMask = squareMaskTable[source.index()];
    bool pinned = srcMask & kingMask.pins();
    if (pinned == true) {
        movesbb &= kingMask.pins();
        return {movesbb & ~opMatCombined, movesbb & opMatCombined};
    }

    if (readMaterial<us>()[pieceIndx].count() <= 1)
        return {movesbb & ~opThreatenedPieces, opThreatenedPieces};

    Bitboard mask(board_constants::fileMasks[source.file] | board_constants::rankMasks[source.rank]);
    Bitboard pieceMask = readMaterial<us>()[pieceIndx] & mask;
    movesbb &= mask;
    // if (pieceMask.count() == 1) {
    //     // rooks don't intersect so no need to isolate further.
    //     return {movesbb & ~opThreatenedPieces, movesbb & opThreatenedPieces};
    // }

    Bitboard otherRooks = pieceMask & ~squareMaskTable[source.index()];
    Bitboard includeMask = squareMaskTable[source.index()];
    Bitboard usMaterial = readMaterial<us>().combine();
    // remove self from us material
    usMaterial &= ~squareMaskTable[source.index()];

    const auto bounds = board_constants::boundsRelativeMasks[(size_t)us];

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
    else {
        Bitboard moves = 0;
        moves |= internalCalculateThreat<us, north, rookId>(bounds[north], pieceMask, usMaterial, opMatCombined);
        moves |= internalCalculateThreat<us, east, rookId>(bounds[east], pieceMask, usMaterial, opMatCombined);
        moves |= internalCalculateThreat<us, south, rookId>(bounds[south], pieceMask, usMaterial, opMatCombined);
        moves |= internalCalculateThreat<us, west, rookId>(bounds[west], pieceMask, usMaterial, opMatCombined);
        movesbb &= moves;
    }

    return {movesbb & ~opThreatenedPieces, movesbb & opThreatenedPieces};
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