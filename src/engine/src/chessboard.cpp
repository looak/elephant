#include "chessboard.h"
#include "bitboard_constants.h"
#include "hash_zorbist.h"
#include "intrinsics.hpp"
#include "log.h"
#include "move.h"

#include <future>
#include <thread>
#include <vector>

ChessboardTile::ChessboardTile() :
    m_position(InvalidNotation),
    m_piece(ChessPiece())
{
}

ChessboardTile::ChessboardTile(Notation&& notation) :
    m_position(std::move(notation))
{
}

bool
ChessboardTile::operator==(const ChessboardTile& rhs) const
{
    bool result = m_position == rhs.m_position;
    result |= m_piece == rhs.m_piece;
    return result;
}

ChessboardTile&
ChessboardTile::operator=(const ChessboardTile& rhs)
{
    m_position = Notation(rhs.m_position);
    m_piece = rhs.m_piece;

    return *this;
}

Chessboard::Chessboard() :
    m_hash(0)
{
    for (byte r = 0; r < 8; r++) {
        for (byte f = 0; f < 8; f++) {
            Notation pos(r, f);
            m_tiles[pos.index()].editPosition() = std::move(pos);
            m_tiles[pos.index()].editPiece() = ChessPiece();
        }
    }

    m_kings[0].first = ChessPiece();
    m_kings[0].second = Notation();
    m_kings[1].first = ChessPiece();
    m_kings[1].second = Notation();

#ifdef EG_DEBUGGING
    // verify tiles & tiles named have the same position
    LOG_ERROR_EXPR(m_tilesNamed.A1.m_position == Notation(0, 0));
    LOG_ERROR_EXPR(m_tilesNamed.A2.m_position == Notation(0, 1));
    LOG_ERROR_EXPR(m_tilesNamed.A3.m_position == Notation(0, 2));
    LOG_ERROR_EXPR(m_tilesNamed.A4.m_position == Notation(0, 3));
    LOG_ERROR_EXPR(m_tilesNamed.A5.m_position == Notation(0, 4));
    LOG_ERROR_EXPR(m_tilesNamed.A6.m_position == Notation(0, 5));
    LOG_ERROR_EXPR(m_tilesNamed.A7.m_position == Notation(0, 6));
    LOG_ERROR_EXPR(m_tilesNamed.A8.m_position == Notation(0, 7));

    LOG_ERROR_EXPR(m_tilesNamed.B1.m_position == Notation(1, 0));
    LOG_ERROR_EXPR(m_tilesNamed.B2.m_position == Notation(1, 1));
    LOG_ERROR_EXPR(m_tilesNamed.B3.m_position == Notation(1, 2));
    LOG_ERROR_EXPR(m_tilesNamed.B4.m_position == Notation(1, 3));
    LOG_ERROR_EXPR(m_tilesNamed.B5.m_position == Notation(1, 4));
    LOG_ERROR_EXPR(m_tilesNamed.B6.m_position == Notation(1, 5));
    LOG_ERROR_EXPR(m_tilesNamed.B7.m_position == Notation(1, 6));
    LOG_ERROR_EXPR(m_tilesNamed.B8.m_position == Notation(1, 7));

    LOG_ERROR_EXPR(m_tilesNamed.C1.m_position == Notation(2, 0));
    LOG_ERROR_EXPR(m_tilesNamed.C2.m_position == Notation(2, 1));
    LOG_ERROR_EXPR(m_tilesNamed.C3.m_position == Notation(2, 2));
    LOG_ERROR_EXPR(m_tilesNamed.C4.m_position == Notation(2, 3));
    LOG_ERROR_EXPR(m_tilesNamed.C5.m_position == Notation(2, 4));
    LOG_ERROR_EXPR(m_tilesNamed.C6.m_position == Notation(2, 5));
    LOG_ERROR_EXPR(m_tilesNamed.C7.m_position == Notation(2, 6));
    LOG_ERROR_EXPR(m_tilesNamed.C8.m_position == Notation(2, 7));

    LOG_ERROR_EXPR(m_tilesNamed.D1.m_position == Notation(3, 0));
    LOG_ERROR_EXPR(m_tilesNamed.D2.m_position == Notation(3, 1));
    LOG_ERROR_EXPR(m_tilesNamed.D3.m_position == Notation(3, 2));
    LOG_ERROR_EXPR(m_tilesNamed.D4.m_position == Notation(3, 3));
    LOG_ERROR_EXPR(m_tilesNamed.D5.m_position == Notation(3, 4));
    LOG_ERROR_EXPR(m_tilesNamed.D6.m_position == Notation(3, 5));
    LOG_ERROR_EXPR(m_tilesNamed.D7.m_position == Notation(3, 6));
    LOG_ERROR_EXPR(m_tilesNamed.D8.m_position == Notation(3, 7));

    LOG_ERROR_EXPR(m_tilesNamed.E1.m_position == Notation(4, 0));
    LOG_ERROR_EXPR(m_tilesNamed.E2.m_position == Notation(4, 1));
    LOG_ERROR_EXPR(m_tilesNamed.E3.m_position == Notation(4, 2));
    LOG_ERROR_EXPR(m_tilesNamed.E4.m_position == Notation(4, 3));
    LOG_ERROR_EXPR(m_tilesNamed.E5.m_position == Notation(4, 4));
    LOG_ERROR_EXPR(m_tilesNamed.E6.m_position == Notation(4, 5));
    LOG_ERROR_EXPR(m_tilesNamed.E7.m_position == Notation(4, 6));
    LOG_ERROR_EXPR(m_tilesNamed.E8.m_position == Notation(4, 7));

    LOG_ERROR_EXPR(m_tilesNamed.F1.m_position == Notation(5, 0));
    LOG_ERROR_EXPR(m_tilesNamed.F2.m_position == Notation(5, 1));
    LOG_ERROR_EXPR(m_tilesNamed.F3.m_position == Notation(5, 2));
    LOG_ERROR_EXPR(m_tilesNamed.F4.m_position == Notation(5, 3));
    LOG_ERROR_EXPR(m_tilesNamed.F5.m_position == Notation(5, 4));
    LOG_ERROR_EXPR(m_tilesNamed.F6.m_position == Notation(5, 5));
    LOG_ERROR_EXPR(m_tilesNamed.F7.m_position == Notation(5, 6));
    LOG_ERROR_EXPR(m_tilesNamed.F8.m_position == Notation(5, 7));

    LOG_ERROR_EXPR(m_tilesNamed.G1.m_position == Notation(6, 0));
    LOG_ERROR_EXPR(m_tilesNamed.G2.m_position == Notation(6, 1));
    LOG_ERROR_EXPR(m_tilesNamed.G3.m_position == Notation(6, 2));
    LOG_ERROR_EXPR(m_tilesNamed.G4.m_position == Notation(6, 3));
    LOG_ERROR_EXPR(m_tilesNamed.G5.m_position == Notation(6, 4));
    LOG_ERROR_EXPR(m_tilesNamed.G6.m_position == Notation(6, 5));
    LOG_ERROR_EXPR(m_tilesNamed.G7.m_position == Notation(6, 6));
    LOG_ERROR_EXPR(m_tilesNamed.G8.m_position == Notation(6, 7));

    LOG_ERROR_EXPR(m_tilesNamed.H1.m_position == Notation(7, 0));
    LOG_ERROR_EXPR(m_tilesNamed.H2.m_position == Notation(7, 1));
    LOG_ERROR_EXPR(m_tilesNamed.H3.m_position == Notation(7, 2));
    LOG_ERROR_EXPR(m_tilesNamed.H4.m_position == Notation(7, 3));
    LOG_ERROR_EXPR(m_tilesNamed.H5.m_position == Notation(7, 4));
    LOG_ERROR_EXPR(m_tilesNamed.H6.m_position == Notation(7, 5));
    LOG_ERROR_EXPR(m_tilesNamed.H7.m_position == Notation(7, 6));
    LOG_ERROR_EXPR(m_tilesNamed.H8.m_position == Notation(7, 7));
#endif
}

Chessboard::Chessboard(const Chessboard& other) :
    m_hash(other.readHash())
{
    auto otherItr = other.begin();
    auto thisItr = this->begin();
    while (otherItr != other.end()) {
        *thisItr = *otherItr;
        otherItr++;
        thisItr++;
    }

    m_kings[0].first = other.m_kings[0].first;
    m_kings[0].second = Notation(other.m_kings[0].second);
    m_kings[1].first = other.m_kings[1].first;
    m_kings[1].second = Notation(other.m_kings[1].second);

    m_position = other.m_position;
}

std::string
Chessboard::toString() const
{
    auto boardItr = begin();
    std::array<std::stringstream, 8> ranks;
    byte prevRank = -1;
    do  // build each row
    {
        if (prevRank != boardItr.rank()) {
            ranks[boardItr.rank()] << "\n" << (int)(boardItr.rank() + 1) << "  ";
        }

        ranks[boardItr.rank()] << '[' << (*boardItr).readPiece().toString() << ']';
        prevRank = boardItr.rank();
        ++boardItr;

    } while (boardItr != end());

    std::stringstream boardstream;
    auto rankItr = ranks.rbegin();
    while (rankItr != ranks.rend())  // rebuild the board
    {
        boardstream << (*rankItr).str();
        rankItr++;
    }

    boardstream << "\n    A  B  C  D  E  F  G  H\n";

    boardstream << "castling state: " << m_position.readCastling().toString();
    boardstream << "\nen passant: " << m_position.readEnPassant().toString();
    boardstream << "\nhash: 0x" << m_hash << "\n";

    return boardstream.str();
}
void
Chessboard::Clear()
{
    for (auto& tile : m_tiles) {
        tile.editPiece() = ChessPiece();
    }

    m_hash = 0;
    m_kings[0].first = ChessPiece();
    m_kings[0].second = Notation();
    m_kings[1].first = ChessPiece();
    m_kings[1].second = Notation();
    m_position.Clear();
}

bool
Chessboard::PlacePiece(ChessPiece piece, Notation target, bool overwrite)
{
    // if (!Bitboard::IsValidSquare(target))
    //     return false;

    const auto& tsqrPiece = m_tiles[target.index()].readPiece();
    if (tsqrPiece != ChessPiece()) {
        if (overwrite == true)
            m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, tsqrPiece, target);
        else
            return false;  // already a piece on this square
    }

    if (piece.getType() == PieceType::KING) {
        m_kings[piece.set()].first = piece;
        m_kings[piece.set()].second = Notation(target);
    }

    m_tiles[target.index()].editPiece() = piece;
    m_position.PlacePiece(piece, target);

    m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, piece, target);
    return true;
}

template<bool validation>
MoveUndoUnit
Chessboard::MakeMove(const PackedMove move, ChessPiece pieceToPromoteTo)
{
    MoveUndoUnit undoState;
    undoState.move = move;
    undoState.hash = m_hash;

    const auto& piece = m_tiles[move.source()].readPiece();
    Square targetSqr = move.targetSqr();
    // cache captureTarget in seperate variable since we might be capturing enpassant
    Square captureTarget = targetSqr;

    // storing enpassant target square so we can unmake this.
    undoState.enPassantState.write(m_position.readEnPassant().read());

    switch (piece.getType()) {
        case PieceType::PAWN:
            // updating pieceTarget since if we're capturing enpassant the target will be on a
            // different square.
            captureTarget = InternalHandlePawnMove(move, undoState);
            break;

        case PieceType::KING:
            [[fallthrough]];
        case PieceType::ROOK:
            InternalHandleKingRookMove(piece, move, undoState);
            [[fallthrough]];

        default:
            // update hash, start by removing old en passant if there was one.
            if (m_position.readEnPassant() == true)
                m_hash = ZorbistHash::Instance().HashEnPassant(m_hash, m_position.readEnPassant().readSquare());

            // reset enpassant cached values
            m_position.editEnPassant().clear();
    }

    InternalHandleCapture(move, captureTarget, undoState);

    // do move
    InternalMakeMove(move.sourceSqr(), move.targetSqr());

    // check if we checkmated or checked the op set after we have made the move
    // Set opSet = ChessPiece::FlipSet(move.Piece.getSet());
    // if (isChecked(opSet)) {
    //     move.Flags |= MoveFlag::Check;

    //     if (isCheckmated(opSet)) {
    //         move.Flags = (MoveFlag)(move.Flags & ~MoveFlag::Check);
    //         move.Flags |= MoveFlag::Checkmate;
    //     }
    // }

    return undoState;
}

template MoveUndoUnit Chessboard::MakeMove<true>(PackedMove, ChessPiece);
template MoveUndoUnit Chessboard::MakeMove<false>(PackedMove, ChessPiece);

bool
Chessboard::UnmakeMove(const MoveUndoUnit& undoState)
{
    const i32 srcSqr = undoState.move.source();
    const i32 trgSqr = undoState.move.target();
    const ChessPiece movedPiece = m_tiles[trgSqr].readPiece();

    // idea here is to store the piece to place back on the board in this variable,
    // if we're dealing with a promotion this will be a pawn of the correct set.
    // otherwise it will be the same piece as movedPiece.
    const ChessPiece promotedPiece =
        undoState.move.isPromotion() ? ChessPiece(movedPiece.getSet(), PieceType::PAWN) : movedPiece;

    // unmake move, currently we are tracking the piece and board state in two
    // different places, here in our tiles and in the position.
    m_tiles[srcSqr].editPiece() = promotedPiece;
    m_tiles[trgSqr].editPiece() = ChessPiece::None();
    m_position.PlacePiece(promotedPiece, Notation(srcSqr));
    m_position.ClearPiece(movedPiece, Notation(trgSqr));

    if (undoState.move.isCapture()) {
        if (undoState.move.isEnPassant()) {
            i32 epPieceSqr = static_cast<i32>(undoState.enPassantState.readTarget());
            m_position.PlacePiece(undoState.capturedPiece, Notation(epPieceSqr));
            m_tiles[epPieceSqr].editPiece() = undoState.capturedPiece;
        }
        else {
            m_position.PlacePiece(undoState.capturedPiece, Notation(trgSqr));
            m_tiles[trgSqr].editPiece() = undoState.capturedPiece;
        }
    }

    m_position.editEnPassant().write(undoState.enPassantState.read());  // restore enpassant state
    m_position.editCastling().write(undoState.castlingState.read());    // restore castling state

    m_hash = undoState.hash;

    return true;
}

Move
Chessboard::DeserializeMoveFromPGN(const std::string& pgn, bool isWhiteMove) const
{
    // Move mv = Move::fromPGN(pgn, isWhiteMove);

    // if (mv.SourceSquare.isValid())
    //     return mv;

    // u64 targetMask = UINT64_C(1) << mv.TargetSquare.index();

    // const auto notations = m_material[mv.Piece.set()].buildPlacementsOfPiece(mv.Piece);
    // std::vector<Notation> possibleSources;

    // // currently this won't care if a move is legal or not.
    // for (const auto& notation : notations) {
    //     auto moveMask = m_bitboard.calcAvailableMoves(notation, mv.Piece, 0, {});
    //     if (moveMask & targetMask)
    //         possibleSources.push_back(notation);
    // }

    // if (possibleSources.size() == 1) {
    //     mv.SourceSquare = possibleSources[0];
    //     mv.setAmbiguous(false);
    //     mv.setInvalid(false);
    //     return mv;
    // }
    // else {
    //     // during deserialization there was additional information in the pgn to disambiguate the
    //     // move
    //     if (mv.SourceSquare.file == 9) {  // we are looking for a piece on given rank
    //         for (const auto& notation : possibleSources) {
    //             if (notation.rank == mv.SourceSquare.rank) {
    //                 mv.SourceSquare = notation;
    //                 mv.setAmbiguous(false);
    //                 mv.setInvalid(false);
    //                 return mv;
    //             }
    //         }
    //     }
    //     else if (mv.SourceSquare.rank == 9) {  // we are looking for a piece on given file
    //         for (const auto& notation : possibleSources) {
    //             if (notation.file == mv.SourceSquare.file) {
    //                 mv.SourceSquare = notation;
    //                 mv.setAmbiguous(false);
    //                 mv.setInvalid(false);
    //                 return mv;
    //             }
    //         }
    //     }
    // }

    return Move::Invalid();
}

// todo needs unit tests
std::string
Chessboard::SerializeMoveToPGN(const Move& move) const
{
    std::string pgn = "";
    // if (move.Piece.isPawn()) {
    //     if (move.isCapture()) {
    //         pgn += Notation::fileToChar(move.SourceSquare);
    //         pgn += "x";
    //     }

    //     pgn += Notation::toString(move.TargetSquare);
    //     if (move.isPromotion()) {
    //         pgn += "=";
    //         pgn += move.PromoteToPiece.toString();
    //     }
    // }
    // else if (move.isCastling()) {
    //     if (move.TargetSquare.file == 6)
    //         pgn += "O-O";
    //     else
    //         pgn += "O-O-O";
    // }
    // else {
    //     pgn += std::toupper(move.Piece.toString());
    //     u64 curMask = UINT64_C(1) << move.TargetSquare.index();

    //     // do we need to be more specific?
    //     auto notations = m_material[move.Piece.set()].buildPlacementsOfPiece(move.Piece);
    //     if (notations.size() > 1) {
    //         // remove self from list.
    //         std::erase_if(notations, [&](const Notation& n) { return n == move.SourceSquare; });

    //         for (const auto& pos : notations) {
    //             u64 moveMask = m_bitboard.calcAvailableMoves(pos, move.Piece, 0, {});
    //             if (moveMask & curMask) {
    //                 // this assumes there are only two pieces of the same type on the board
    //                 // which will break if we ever support chess960 or pawns are promoted.
    //                 if (pos.file == move.SourceSquare.file)
    //                     pgn += Notation::rankToChar(pos);
    //                 else if (pos.rank == move.SourceSquare.rank)
    //                     pgn += Notation::fileToChar(pos);
    //                 else
    //                     pgn += pos.toString();
    //             }
    //         }
    //     }

    //     if (m_tiles[move.TargetSquare.index()].readPiece().isValid())
    //         pgn += "x";
    //     pgn += move.TargetSquare.toString();
    // }

    return pgn;
}

bool
Chessboard::InternalUpdateEnPassant(Notation source, Notation target)
{
    signed char dif = source.rank - target.rank;
    if (abs(dif) == 2)  // we made a enpassant move
    {
        dif = (signed char)((float)dif * .5f);
        auto sqr = Notation(source.file, source.rank - dif);
        m_position.editEnPassant().writeSquare(sqr.toSquare());
        m_hash = ZorbistHash::Instance().HashEnPassant(m_hash, sqr);
        return true;
    }

    return false;
}

Square
Chessboard::InternalHandlePawnMove(const PackedMove move, MoveUndoUnit& undoState)
{
    Square pieceTarget = move.targetSqr();

    // compare target square with en passant, if this is true we're capturing enpassant
    if (pieceTarget == m_position.readEnPassant().readSquare()) {
        // changing piece target to the position of the pawn being capturing enpassant
        pieceTarget = m_position.readEnPassant().readTarget();

        // could validate that there is a pawn on the target square.
    }

    // update hash, start by removing old en passant if there was one.
    if (m_position.readEnPassant() == true)
        m_hash = ZorbistHash::Instance().HashEnPassant(m_hash, m_position.readEnPassant().readSquare());

    // reset enpassant cached values
    m_position.editEnPassant().clear();

    InternalUpdateEnPassant(move.sourceSqr(), move.targetSqr());

    if (move.isPromotion()) {
        // ensure promotion piece is same set as piece we're moving. There is a bug in string
        // parsing of piece which assumses capitalized string is white, but that doesn't work for
        // promotions
        auto& srcTile = m_tiles[move.source()];
        const ChessPiece src = srcTile.readPiece();
        const ChessPiece promote(src.getSet(), static_cast<PieceType>(move.readPromoteToPieceType()));

        m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, src, move.sourceSqr());
        m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, promote, move.sourceSqr());

        // updating the piece on the source tile since we're doing this pre-move.
        // internal move will handle the actual move of the piece, but what piece it is doesn't
        // really mater at that point.
        srcTile.editPiece() = promote;

        m_position.ClearPiece(src, move.sourceSqr());
        m_position.PlacePiece(promote, move.sourceSqr());
    }

    return pieceTarget;
}

bool
Chessboard::InternalHandleKingMove(const PackedMove move, Set set, Notation& targetRook, Notation& rookMove,
                                   MoveUndoUnit& undoUnit)
{
    const u8 setIndx = (u8)set;
    bool castling = false;
    byte casltingMask = 3 << (2 * setIndx);
    byte castlingState = m_position.readCastling().read();
    Notation targetSquare(move.targetSqr());
    if (castlingState & casltingMask) {
        byte targetRank = 7 * setIndx;
        if (targetSquare.file == 2)  // we are in c file.
        {
            targetRook = Notation(0, targetRank);
            rookMove = Notation(3, targetRank);
            castling = true;
        }
        else if (targetSquare.file == 6)  // we are in g file.
        {
            targetRook = Notation(7, targetRank);
            rookMove = Notation(5, targetRank);
            castling = true;
        }
    }
    m_hash = ZorbistHash::Instance().HashCastling(m_hash, castlingState);
    undoUnit.castlingState.write(castlingState);
    casltingMask &= castlingState;
    castlingState ^= casltingMask;
    m_hash = ZorbistHash::Instance().HashCastling(m_hash, castlingState);
    m_position.editCastling().write(castlingState);

    return castling;
}

void
Chessboard::InternalHandleRookMove(const ChessPiece piece, const PackedMove move, Notation targetRook, Notation rookMove,
                                   MoveUndoUnit& undoState)
{
    if (piece.getType() == PieceType::KING && targetRook != Notation()) {
        InternalMakeMove(targetRook, rookMove);
    }
    else {
        InternalHandleRookMovedOrCaptured(move, move.sourceSqr(), undoState);
    }
}

void
Chessboard::InternalUpdateCastlingState(const PackedMove move, byte mask, MoveUndoUnit& undoState)
{
    byte castlingState = m_position.readCastling().read();
    m_hash = ZorbistHash::Instance().HashCastling(m_hash, castlingState);
    // in a situation where rook captures rook from original positions we don't need to store
    // we don't want to overwrite the original prev written while doing our move castling state.
    // this code has changed slightly since the comment above was written in case we run into a bug.
    if (undoState.castlingState.hasNone())
        undoState.castlingState.write(castlingState);
    mask &= castlingState;
    castlingState ^= mask;
    m_hash = ZorbistHash::Instance().HashCastling(m_hash, castlingState);
    m_position.editCastling().write(castlingState);
}

void
Chessboard::InternalHandleRookMovedOrCaptured(const PackedMove move, Notation rookSquare, MoveUndoUnit& undoState)
{
    byte mask = 0;
    // 0x01 == K, 0x02 == Q, 0x04 == k, 0x08 == q
    switch (rookSquare.index()) {
        case 63:  // H8 Black King Side Rook
            mask |= 0x04;
            InternalUpdateCastlingState(move, mask, undoState);
            break;
        case 56:  // A8 Black Queen Side Rook
            mask |= 0x08;
            InternalUpdateCastlingState(move, mask, undoState);
            break;
        case 7:  // H1 White King Side Rook
            mask |= 0x01;
            InternalUpdateCastlingState(move, mask, undoState);
            break;
        case 0:  // A1 White Queen Side Rook
            mask |= 0x02;
            InternalUpdateCastlingState(move, mask, undoState);
            break;
    }
}

void
Chessboard::InternalHandleKingRookMove(const ChessPiece piece, const PackedMove move, MoveUndoUnit& undoState)
{
    Notation targetRook, rookMove;
    switch (piece.getType()) {
        case PieceType::KING:
            if (InternalHandleKingMove(move, piece.getSet(), targetRook, rookMove, undoState) == false)
                break;

            [[fallthrough]];

        case PieceType::ROOK:
            InternalHandleRookMove(piece, move, targetRook, rookMove, undoState);
            [[fallthrough]];

        default:
            return;
    }
}

void
Chessboard::InternalMakeMove(Notation source, Notation target)
{
    ChessPiece piece = m_tiles[source.index()].editPiece();

    m_tiles[source.index()].editPiece() = ChessPiece();  // clear old square.
    m_tiles[target.index()].editPiece() = piece;

    m_position.ClearPiece(piece, source);
    m_position.PlacePiece(piece, target);

    // update hash
    m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, piece, target);
    m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, piece, source);
}

bool
Chessboard::VerifyMove(const Move& move) const
{
    // if (!Bitboard::IsValidSquare(move.SourceSquare) || !Bitboard::IsValidSquare(move.TargetSquare))
    //     return false;

    // const auto& piece = m_tiles[move.SourceSquare.index()].readPiece();
    // if (piece == ChessPiece())
    //     return false;

    // u64 threatenedMask = calculateThreatenedMask(ChessPiece::FlipSet(piece.getSet()));

    // if (m_position.IsValidMove(move.SourceSquare, piece, move.TargetSquare, m_castlingState, m_enPassant, threatenedMask) ==
    //     false)
    //     return false;

    return true;
}

ChessPiece
Chessboard::readPieceAt(Notation notation) const
{
    return m_tiles[notation.index()].readPiece();
}

// bool
// Chessboard::MakeNullMove(Move& move)
// {
//     // storing enpassant target square so we can unmake this.
//     if (Notation::Validate(m_enPassant)) {
//         m_hash = ZorbistHash::Instance().HashEnPassant(m_hash, m_enPassant);
//         move.EnPassantTargetSquare = m_enPassantTarget;
//     }

//     m_enPassant = Notation();
//     m_enPassantTarget = Notation();

//     return true;
// }
// bool
// Chessboard::UnmakeNullMove(const Move& move)
// {
//     if (Notation::Validate(m_enPassant))
//         m_hash = ZorbistHash::Instance().HashEnPassant(m_hash, m_enPassant);

//     m_enPassant = Notation();
//     m_enPassantTarget = Notation();

//     if (move.EnPassantTargetSquare.isValid()) {
//         byte offset = move.Piece.getSet() == Set::WHITE ? 1 : -1;
//         m_enPassant = Notation(move.EnPassantTargetSquare.file, move.EnPassantTargetSquare.rank + offset);
//         m_enPassantTarget = move.EnPassantTargetSquare;
//         m_hash = ZorbistHash::Instance().HashEnPassant(m_hash, m_enPassant);
//     }

//     return true;
// }

// bool
// Chessboard::UnmakeMove(const Move& move)
// {
//     if (Notation::Validate(m_enPassant))
//         m_hash = ZorbistHash::Instance().HashEnPassant(m_hash, m_enPassant);

//     m_enPassant = Notation();
//     m_enPassantTarget = Notation();

//     ChessPiece pieceToRmv = move.Piece;
//     ChessPiece pieceToAdd = move.Piece;

//     if (move.isPromotion())
//         pieceToRmv = move.PromoteToPiece;

//     if (move.PrevCastlingState != 0) {
//         auto& castlingState = m_position.editCastling();
//         m_hash = ZorbistHash::Instance().HashCastling(m_hash, castlingState.read());
//         m_hash = ZorbistHash::Instance().HashCastling(m_hash, move.PrevCastlingState);
//         castlingState.write(move.PrevCastlingState);
//     }

//     if (move.isCastling()) {  // move back rook to it's origin since the rest of this code will deal
//                               // with the king.
//         ChessPiece rook(move.Piece.getSet(), PieceType::ROOK);
//         Notation rookOrigin;
//         Notation rookPlacement;
//         if (move.TargetSquare.file == 2)  // queen side
//         {
//             rookOrigin = Notation(file_a, move.SourceSquare.rank);
//             rookPlacement = Notation(file_d, move.SourceSquare.rank);
//         }
//         else  // king side
//         {
//             rookOrigin = Notation(file_h, move.SourceSquare.rank);
//             rookPlacement = Notation(file_f, move.SourceSquare.rank);
//         }
//         // move rook
//         InternalUnmakeMove(rookOrigin, rookPlacement, rook, rook);
//     }

//     if (move.EnPassantTargetSquare.isValid()) {
//         byte offset = move.Piece.getSet() == Set::WHITE ? 1 : -1;
//         m_enPassant = Notation(move.EnPassantTargetSquare.file, move.EnPassantTargetSquare.rank + offset);
//         m_enPassantTarget = move.EnPassantTargetSquare;
//         m_hash = ZorbistHash::Instance().HashEnPassant(m_hash, m_enPassant);
//     }

//     // move pieces back to where they were before we re add any possible captured piecs.
//     InternalUnmakeMove(move.SourceSquare, move.TargetSquare, pieceToRmv, pieceToAdd);

//     if (move.Piece.getType() == PieceType::KING)
//         m_kings[(size_t)move.Piece.getSet()].second = move.SourceSquare;

//     if (move.CapturedPiece.isValid()) {
//         if (move.isEnPassant()) {
//             byte offset = move.Piece.getSet() == Set::WHITE ? -1 : 1;
//             m_enPassantTarget = Notation(move.TargetSquare.file, move.TargetSquare.rank + offset);
//             m_enPassant = move.TargetSquare;

//             m_tiles[m_enPassantTarget.index()].editPiece() = move.CapturedPiece;
//             m_position.PlacePiece(move.CapturedPiece, m_enPassantTarget);

//             m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, move.CapturedPiece, m_enPassantTarget);
//             m_hash = ZorbistHash::Instance().HashEnPassant(m_hash, m_enPassant);
//         }
//         else {
//             m_tiles[move.TargetSquare.index()].editPiece() = move.CapturedPiece;
//             m_position.PlacePiece(move.CapturedPiece, move.TargetSquare);

//             m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, move.CapturedPiece, move.TargetSquare);
//         }
//     }

//     return true;
// }

// void
// Chessboard::InternalUnmakeMove(Notation source, Notation target, ChessPiece pieceToRmv, ChessPiece pieceToAdd)
// {
//     auto& sourceTile = m_tiles[source.index()];
//     auto& targetTile = m_tiles[target.index()];
//     // unmake chessboard
//     sourceTile.editPiece() = pieceToAdd;
//     targetTile.editPiece() = ChessPiece();

//     // unmake bitboard
//     m_position.ClearPiece(pieceToRmv, target);
//     m_position.PlacePiece(pieceToAdd, source);

//     // update hash
//     m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, pieceToRmv, target);
//     m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, pieceToAdd, source);
// }

// bool
// Chessboard::MakeMoveUnchecked(Move& move)
// {
//     const auto& piece = m_tiles[move.SourceSquare.index()].readPiece();
//     move.Flags = MoveFlag::Zero;
//     move.Piece = piece;

//     auto pieceTarget = Notation(move.TargetSquare);

//     // storing enpassant target square so we can unmake this.
//     move.EnPassantTargetSquare = m_enPassantTarget;

//     switch (piece.getType()) {
//         case PieceType::PAWN:
//             // updating pieceTarget since if we're capturing enpassant the target will be on a
//             // different square.
//             pieceTarget = InternalHandlePawnMove(move);
//             break;

//         case PieceType::KING:
//             [[fallthrough]];
//         case PieceType::ROOK:
//             InternalHandleKingRookMove(move);
//             [[fallthrough]];

//         default:
//             // reset enpassant cached values
//             if (Notation::Validate(m_enPassant))
//                 m_hash = ZorbistHash::Instance().HashEnPassant(m_hash, m_enPassant);

//             m_enPassant = Notation();
//             m_enPassantTarget = Notation();
//     }

//     InternalHandleCapture(move, pieceTarget);

//     // do move
//     InternalMakeMove(move.SourceSquare, move.TargetSquare);

//     return true;
// }

// bool
// Chessboard::MakeMove(Move& move)
// {
//     move.Flags = MoveFlag::Invalid;

//     // todo: enable if debug and test
//     if (!VerifyMove(move))
//         return false;

//     const auto& piece = m_tiles[move.SourceSquare.index()].readPiece();
//     move.Flags = MoveFlag::Zero;
//     move.Piece = piece;

//     auto pieceTarget = Notation(move.TargetSquare);

//     // storing enpassant target square so we can unmake this.
//     move.EnPassantTargetSquare = m_enPassantTarget;

//     switch (piece.getType()) {
//         case PieceType::PAWN:
//             // updating pieceTarget since if we're capturing enpassant the target will be on a
//             // different square.
//             pieceTarget = InternalHandlePawnMove(move);
//             break;

//         case PieceType::KING:
//             [[fallthrough]];
//         case PieceType::ROOK:
//             InternalHandleKingRookMove(move);
//             [[fallthrough]];

//         default:
//             // reset enpassant cached values
//             if (Notation::Validate(m_enPassant))
//                 m_hash = ZorbistHash::Instance().HashEnPassant(m_hash, m_enPassant);

//             m_enPassant = Notation();
//             m_enPassantTarget = Notation();
//     }

//     InternalHandleCapture(move, pieceTarget);

//     // do move
//     InternalMakeMove(move.SourceSquare, move.TargetSquare);

//     // check if we checkmated or checked the op set after we have made the move
//     Set opSet = ChessPiece::FlipSet(move.Piece.getSet());
//     if (isChecked(opSet)) {
//         move.Flags |= MoveFlag::Check;

//         if (isCheckmated(opSet)) {
//             move.Flags = (MoveFlag)(move.Flags & ~MoveFlag::Check);
//             move.Flags |= MoveFlag::Checkmate;
//         }
//     }
//     FATAL_ASSERT(move.Piece.isValid());

//     return true;
// }

void
Chessboard::InternalHandleCapture(const PackedMove move, const Notation pieceTarget, MoveUndoUnit& undoState)
{
    // handle capture
    auto capturedPiece = m_tiles[pieceTarget.index()].readPiece();

    if (capturedPiece != ChessPiece()) {
        // remove captured piece from board.
        m_tiles[pieceTarget.index()].editPiece() = ChessPiece();
        undoState.capturedPiece = capturedPiece;

        if (capturedPiece.getType() == PieceType::ROOK)
            InternalHandleRookMovedOrCaptured(move, move.targetSqr(), undoState);

        m_position.ClearPiece(capturedPiece, pieceTarget);

        // remove piece from hash
        m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, capturedPiece, pieceTarget);
    }
}

int
Chessboard::IsMoveCastling(const Move& move) const
{
    byte casltingMask = 3 << (2 * move.Piece.set());
    if (m_position.readCastling().read() & casltingMask) {
        if (move.TargetSquare.file == 2)
            return -1;  // castling queen side
        else if (move.TargetSquare.file == 6)
            return 1;  // castling king side
    }

    return 0;
}

bool
Chessboard::IsPromoting(const Move& move) const
{
    // we need to flip set for this calculation since it's the opposite side of the
    // board which is the promotion rank.
    byte promoteRankCheck = 7 * ChessPiece::FlipSet(move.Piece.set());
    if (move.TargetSquare.rank == promoteRankCheck)
        return true;

    return false;
}

std::tuple<int, KingMask>
Chessboard::calcualteCheckedCount(Set set) const
{
    std::tuple<int, KingMask> result = {0, KingMask()};

    KingMask mask = calcKingMask(set);
    std::get<1>(result) = mask;

    i32 chkCount = 0;
    for (int i = 0; i < 8; i++) {
        if (mask.checked[i] == true)
            chkCount++;
    }

    if (mask.knightOrPawnCheck == true)
        chkCount++;

    std::get<0>(result) = chkCount;
    return result;
}

bool
Chessboard::isChecked(Set set) const
{
    auto [count, mask] = calcualteCheckedCount(set);
    return count;
}

bool
Chessboard::isCheckmated(Set set) const
{
    if (isChecked(set)) {
        auto moves = GetAvailableMoves(set);
        if (moves.size() == 0)
            return true;
    }
    return false;
}

bool
Chessboard::isStalemated(Set set) const
{
    if (!isChecked(set)) {
        auto moves = GetAvailableMoves(set);
        if (moves.size() == 0)
            return true;
    }
    return false;
}

Notation
Chessboard::readKingPosition(Set set) const
{
    return m_kings[static_cast<u8>(set)].second;
}

template<bool useCache>
KingMask
Chessboard::calcKingMask(Set set) const
{
    u8 indx = static_cast<u8>(set);
    if (m_kings[indx].first == ChessPiece())
        return KingMask();

    if constexpr (useCache) {
        auto& [cached, mask] = m_cachedKingMask[indx];
        if (cached)
            return mask;

        auto slidingPair = readSlidingMaterialMask(ChessPiece::FlipSet(set));
        mask = m_position.calcKingMask(m_kings[indx].first, m_kings[indx].second, slidingPair);
        cached = true;
        return mask;
    }

    auto slidingPair = readSlidingMaterialMask(ChessPiece::FlipSet(set));
    auto mask = m_position.calcKingMask(m_kings[indx].first, m_kings[indx].second, slidingPair);
    return mask;
}

u64
Chessboard::calculateThreatenedMask(Set set) const
{
    u64 mask = ~universe;

    if (set == Set::WHITE)
        mask = m_position.calcThreatenedSquares<Set::WHITE, true>().read();
    else
        mask = m_position.calcThreatenedSquares<Set::BLACK, true>().read();

    return mask;
}

MaterialSlidingMask
Chessboard::readSlidingMaterialMask(Set set) const
{
    if (set == Set::BLACK)
        return m_position.calcMaterialSlidingMasksBulk<Set::BLACK>();

    return m_position.calcMaterialSlidingMasksBulk<Set::WHITE>();
}

std::vector<Move>
Chessboard::concurrentCalculateAvailableMovesForPiece(ChessPiece piece, u64 threatenedMask, KingMask kingMask,
                                                      KingMask checkedMask, bool captureMoves) const
{
    std::vector<Move> result;

    // u64 pieceBitboard = m_material[piece.set()].readPieceBitboard(piece.index());
    // while (pieceBitboard > 0) {
    //     u32 sqr = m_material[piece.set()].readNextPiece(pieceBitboard);
    //     auto moves = GetAvailableMoves(Notation(sqr), piece, threatenedMask, checkedMask, kingMask, captureMoves);
    //     result.insert(result.end(), moves.begin(), moves.end());
    // }

    return result;
}

std::vector<Move>
Chessboard::GetAvailableMoves(Set currentSet, bool captureMoves) const
{
    // Set opSet = ChessPiece::FlipSet(currentSet);
    // u64 threatenedMask = 0;

    // constexpr bool includeMaterial = false;
    // constexpr bool pierceKing = true;
    // if (opSet == Set::WHITE)
    //     threatenedMask = m_position.calcThreatenedSquares<Set::WHITE, includeMaterial, pierceKing>().read();
    // else
    //     threatenedMask = m_position.calcThreatenedSquares<Set::BLACK, includeMaterial, pierceKing>().read();

    // int chkCount = 0;
    // KingMask kingMask = calcKingMask(currentSet);
    // KingMask checkedMask = kingMask.checkedMask(chkCount);
    // kingMask ^= checkedMask;  // could also be called pinned mask?

    // KingMask pawnKingMask = kingMask;
    // KingMask pawnCheckedMask = checkedMask;

    // const unsigned char setModifier = currentSet == Set::WHITE ? 1 : 0;
    // bool kingRankCheck = m_kings[(int)currentSet].second.rank == (3 + setModifier);
    // if (m_enPassant.isValid() && kingRankCheck) {
    //     // remove en passant target from material
    //     ChessPiece pawn(opSet, PieceType::PAWN);
    //     m_position.ClearPiece(pawn, m_enPassantTarget);
    //     constexpr bool useCache = false;
    //     pawnKingMask = calcKingMask<useCache>(currentSet);
    //     chkCount = 0;
    //     pawnCheckedMask = pawnKingMask.checkedMask(chkCount);
    //     pawnKingMask ^= pawnCheckedMask;
    //     pawnKingMask.pawnMask = true;  // add a flag to indicate that this mask is for pawns
    //                                    // who think they're pinned but they aren't
    //     m_position.PlacePiece(pawn, m_enPassantTarget);
    // }

    std::vector<Move> result;
    // std::vector<std::future<std::vector<Move>>> moveFutures;

    // u32 pieceIndx = 1;
    // if (chkCount > 1)
    //     pieceIndx = (u32)PieceType::KING;

    // for (; pieceIndx < (size_t)PieceType::NR_OF_PIECES; ++pieceIndx) {
    //     ChessPiece currentPiece(currentSet, (PieceType)pieceIndx);
    //     // moveFutures.push_back(std::async(std::launch::async,
    //     // &Chessboard::concurrentCalculateAvailableMovesForPiece, this, currentPiece,
    //     // threatenedMask, kingMask, checked));
    //     if (pieceIndx == 1) {
    //         auto moves = concurrentCalculateAvailableMovesForPiece(currentPiece, threatenedMask, pawnKingMask,
    //         pawnCheckedMask,
    //                                                                captureMoves);
    //         result.insert(result.end(), moves.begin(), moves.end());
    //         continue;
    //     }
    //     auto moves =
    //         concurrentCalculateAvailableMovesForPiece(currentPiece, threatenedMask, kingMask, checkedMask, captureMoves);
    //     result.insert(result.end(), moves.begin(), moves.end());
    // }

    // for (auto& future : moveFutures)
    // {
    // 	auto moves = future.get();
    // 	result.insert(result.end(), moves.begin(), moves.end());
    // }

    return result;
}

bool
Chessboard::InternalIsMoveCheck(Move& move) const
{
    return false;
}

std::vector<Move>
Chessboard::GetAvailableMoves(Notation source, ChessPiece piece, u64 threatenedMask, KingMask checkedMask, KingMask kingMask,
                              bool captureMoves) const
{
    std::vector<Move> moveVector;
    moveVector.reserve(128);

    // #ifdef EG_DEBUGGING
    //     if (!Bitboard::IsValidSquare(source))
    //         return moveVector;

    //     if (piece == ChessPiece())
    //         return moveVector;
    // #endif

    //     bool checked = !checkedMask.zero();
    //     // castling not available when in check
    //     byte castlingState = checked ? 0 : m_castlingState;

    //     u64 movesbb =
    //         m_position.calcAvailableMoves(source, piece, castlingState, m_enPassant, threatenedMask, checkedMask, kingMask);
    //     MaterialMask opMaterialMask = m_position.GetMaterial(ChessPiece::FlipSet(piece.getSet()));
    //     u64 opMaterial = opMaterialMask.combine().read();

    //     if (captureMoves) {
    //         movesbb &= opMaterial;
    //     }

    //     if (movesbb == 0) {
    //         return moveVector;
    //     }

    //     // generate capture moves first, this helps searching down the line.
    //     u64 movesbbcopy = movesbb & opMaterial;
    //     while (movesbb != 0) {
    //         u64 itrBB = movesbbcopy;
    //         while (itrBB != 0) {
    //             byte target = (byte)intrinsics::lsbIndex(itrBB);
    //             itrBB = intrinsics::resetLsb(itrBB);

    //             Move move(source, Notation(target));
    //             move.Flags = MoveFlag::Zero;
    //             move.Piece = piece;

    //             if (opMaterial & squareMaskTable[target]) {
    //                 move.Flags |= MoveFlag::Capture;
    //                 move.CapturedPiece = m_tiles[target].readPiece();
    //             }

    //             if (piece.getType() == PieceType::PAWN) {
    //                 if (move.TargetSquare == m_enPassant) {
    //                     move.Flags |= MoveFlag::EnPassant;
    //                     move.Flags |= MoveFlag::Capture;
    //                 }
    //                 else if (IsPromoting(move)) {
    //                     move.Flags |= MoveFlag::Promotion;
    //                     move.PromoteToPiece = ChessPiece(piece.getSet(), PieceType::QUEEN);

    //                     Move rookPromote = Move(move);
    //                     rookPromote.PromoteToPiece = ChessPiece(piece.getSet(), PieceType::ROOK);

    //                     if (InternalIsMoveCheck(rookPromote))
    //                         rookPromote.Flags |= MoveFlag::Check;

    //                     Move bishopPromote = Move(move);
    //                     bishopPromote.PromoteToPiece = ChessPiece(piece.getSet(), PieceType::BISHOP);

    //                     if (InternalIsMoveCheck(bishopPromote))
    //                         bishopPromote.Flags |= MoveFlag::Check;

    //                     Move knightPromote = Move(move);
    //                     knightPromote.PromoteToPiece = ChessPiece(piece.getSet(), PieceType::KNIGHT);

    //                     if (InternalIsMoveCheck(knightPromote))
    //                         knightPromote.Flags |= MoveFlag::Check;

    //                     // when pushing back new elements to the vector our reference to move is moved
    //                     // and pointing at garbage. So we do this last.
    //                     moveVector.push_back(rookPromote);
    //                     moveVector.push_back(bishopPromote);
    //                     moveVector.push_back(knightPromote);
    //                 }
    //             }
    //             else if (move.Piece.getType() == PieceType::KING) {
    //                 int castlingDir = IsMoveCastling(move);
    //                 if (castlingDir != 0)
    //                     move.Flags |= MoveFlag::Castle;
    //             }

    //             if (InternalIsMoveCheck(move)) {
    //                 move.Flags |= MoveFlag::Check;
    //             }
    //             moveVector.push_back(move);
    //         }

    //         movesbb &= ~movesbbcopy;
    //         movesbbcopy = movesbb;
    //     }

    return moveVector;
}

bool
Chessboard::setEnPassant(Notation notation)
{
    if (notation.isValid() == false)
        return false;
    u64 newHash = m_hash;
    auto& ep = m_position.editEnPassant();
    if (ep)
        newHash = ZorbistHash::Instance().HashEnPassant(newHash, ep.readSquare());

    newHash = ZorbistHash::Instance().HashEnPassant(newHash, notation);
    ep.writeSquare(notation.toSquare());

    m_hash = newHash;
    return true;
}

bool
Chessboard::setCastlingState(u8 castlingState)
{
    auto& castlingStateRef = m_position.editCastling();
    m_hash = ZorbistHash::Instance().HashCastling(m_hash, castlingStateRef.read());
    m_hash = ZorbistHash::Instance().HashCastling(m_hash, castlingState);
    castlingStateRef.write(castlingState);
    return true;
}

const ChessboardTile&
Chessboard::readTile(Notation position) const
{
    return m_tiles[position.index()];
}

ChessboardTile&
Chessboard::editTile(Notation position)
{
    return m_tiles[position.index()];
}

const Notation s_beginPos = Notation::BuildPosition('a', 1);
const Notation s_endPos = Notation(0, 8);

Chessboard::Iterator
Chessboard::begin()
{
    return Chessboard::Iterator(*this, Notation(s_beginPos));
}

Chessboard::Iterator
Chessboard::end()
{
    return Chessboard::Iterator(*this, Notation(s_endPos));
}
Chessboard::ConstIterator
Chessboard::begin() const
{
    return Chessboard::ConstIterator(*this, Notation(s_beginPos));
}
Chessboard::ConstIterator
Chessboard::end() const
{
    return Chessboard::ConstIterator(*this, Notation(s_endPos));
}

float
Chessboard::calculateEndGameCoeficient() const
{
    static constexpr i32 defaultPosValueOfMaterial = ChessPieceDef::Value(0) * 16    // pawn
                                                     + ChessPieceDef::Value(1) * 4   // knight
                                                     + ChessPieceDef::Value(2) * 4   // bishop
                                                     + ChessPieceDef::Value(3) * 4   // rook
                                                     + ChessPieceDef::Value(4) * 2;  // queens

    // check if we have promoted a pawn because that will screw with this endgame coeficient
    // calculation. and probably, at the point we're looking for promotions, we're most likely in a
    // endgame already should just reutnr 1.f

    i32 boardMaterialCombinedValue = 0;
    for (u8 index = 0; index < 5; ++index) {
        boardMaterialCombinedValue += ChessPieceDef::Value(index) * m_position.readMaterial<Set::WHITE>()[index].count();
        boardMaterialCombinedValue += ChessPieceDef::Value(index) * m_position.readMaterial<Set::BLACK>()[index].count();
    }

    return 1.f - ((float)boardMaterialCombinedValue / (float)defaultPosValueOfMaterial);
}