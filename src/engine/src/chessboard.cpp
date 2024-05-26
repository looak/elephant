#include "chessboard.h"
#include "bitboard_constants.h"
#include "defines.h"
#include "hash_zorbist.h"
#include "intrinsics.hpp"
#include "log.h"
#include "move.h"
#include "move_generator.hpp"

#include <future>
#include <thread>
#include <vector>

Chessboard::Chessboard() :
    m_hash(0),
    m_isWhiteTurn(true),
    m_moveCount(1),
    m_plyCount(0)
{
    m_kings[0].first = ChessPiece::None();
    m_kings[0].second = Notation();
    m_kings[1].first = ChessPiece::None();
    m_kings[1].second = Notation();
}

Chessboard::Chessboard(const Chessboard& other) :
    m_hash(other.readHash()),
    m_isWhiteTurn(other.m_isWhiteTurn),
    m_moveCount(other.m_moveCount),
    m_plyCount(other.m_plyCount)
{
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

        ranks[boardItr.rank()] << '[' << boardItr.get().toString() << ']';
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
    m_hash = 0;
    m_kings[0].first = ChessPiece::None();
    m_kings[0].second = Notation();
    m_kings[1].first = ChessPiece::None();
    m_kings[1].second = Notation();
    m_position.Clear();
    m_plyCount = 0;
    m_isWhiteTurn = true;
    m_moveCount = 1;
}

bool
Chessboard::PlacePiece(ChessPiece piece, Notation target, bool overwrite)
{
    Square trgSqr = target.toSquare();
    auto tsqrPiece = m_position.readPieceAt(trgSqr);
    if (tsqrPiece != ChessPiece::None()) {
        if (overwrite == true) {
            m_position.ClearPiece(tsqrPiece, trgSqr);
            m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, tsqrPiece, target);
        }
        else
            return false;  // already a piece on this square
    }

    if (piece.getType() == PieceType::KING) {
        m_kings[piece.set()].first = piece;
        m_kings[piece.set()].second = Notation(target);
    }

    m_position.PlacePiece(piece, trgSqr);

    m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, piece, target);
    return true;
}

template<bool validation>
MoveUndoUnit
Chessboard::MakeMove(const PackedMove move)
{
    MoveUndoUnit undoState;
    undoState.move = move;
    undoState.hash = m_hash;

    auto piece = m_position.readPieceAt(move.sourceSqr());
    undoState.movedPiece = piece;

    auto materialEditor = m_position.materialEditor(piece.getSet(), piece.getType());
    const Square targetSqr = move.targetSqr();

    // cache captureTarget in seperate variable since we might be capturing enpassant
    Square captureTarget = targetSqr;

    // storing enpassant target square so we can unmake this.
    undoState.enPassantState.write(m_position.readEnPassant().read());
    undoState.castlingState.write(m_position.readCastling().read());

    switch (piece.getType()) {
    case PieceType::PAWN:
        // updating pieceTarget since if we're capturing enpassant the target will be on a
        // different square.
        std::tie(captureTarget, piece) = InternalHandlePawnMove(move, piece.getSet(), materialEditor, undoState);
        m_plyCount = 0;  // reset ply count on pawn move
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

    // since capture will reset this to 0, we need to increment it here.
    m_plyCount++;

    if (move.isCapture())
        InternalHandleCapture(move, captureTarget, undoState);

    // should happen after capture since enpassant logic relies on that order.
    InternalMakeMove(piece, move.sourceSqr(), move.targetSqr(), materialEditor);

    // unless something goes wrong, updating the black to move hash should remove it when it's time for white to move,
    // or add it when it's time for black to move.
    m_hash = ZorbistHash::Instance().HashBlackToMove(m_hash);

    // flip the bool and if we're back at white turn we assume we just made a black turn and hence we increment the move count.
    m_isWhiteTurn = !m_isWhiteTurn;
    m_moveCount += (short)m_isWhiteTurn;

    return undoState;
}

template MoveUndoUnit Chessboard::MakeMove<true>(PackedMove);
template MoveUndoUnit Chessboard::MakeMove<false>(PackedMove);

bool
Chessboard::UnmakeMove(const MoveUndoUnit& undoState)
{
    const Square srcSqr = undoState.move.sourceSqr();
    const Square trgSqr = undoState.move.targetSqr();
    //const ChessPiece movedPiece = m_position.readPieceAt((Square)trgSqr);
    const ChessPiece movedPiece = undoState.movedPiece;

    // idea here is to store the piece to place back on the board in this variable,
    // if we're dealing with a promotion this will be a pawn of the correct set.
    // otherwise it will be the same piece as movedPiece.
    const ChessPiece promotedPiece =
        undoState.move.isPromotion() ? ChessPiece(movedPiece.getSet(), PieceType::PAWN) : movedPiece;

    // unmake move, currently we are tracking the piece and board state in two
    // different places, here in our tiles and in the position.
    m_position.PlacePiece(promotedPiece, srcSqr);
    m_position.ClearPiece(movedPiece, trgSqr);

    if (undoState.move.isCapture()) {
        if (undoState.move.isEnPassant()) {
            m_position.PlacePiece(undoState.capturedPiece, undoState.enPassantState.readTarget());
        }
        else {
            m_position.PlacePiece(undoState.capturedPiece, trgSqr);
        }
    }
    else if (undoState.move.isCastling()) {
        // we're unmaking a castling move, we need to move the rook back to it's original position.
        // simply done by moving the rook with internal move from notations generated with the king move.
        // king would have been moved back by regular undo move code.
        Notation rookSource;
        Notation rookTarget;
        Notation target(trgSqr);
        if (target.file == file_c)  // queen side
        {
            rookSource = Notation(file_a, target.rank);
            rookTarget = Notation(file_d, target.rank);
        }
        else  // king side
        {
            rookSource = Notation(file_h, target.rank);
            rookTarget = Notation(file_f, target.rank);
        }
        ChessPiece rook(movedPiece.getSet(), PieceType::ROOK);
        auto editor = m_position.materialEditor(movedPiece.getSet(), PieceType::ROOK);
        InternalMakeMove(rook, rookTarget.toSquare(), rookSource.toSquare(), editor);
    }

    m_position.editEnPassant().write(undoState.enPassantState.read());  // restore enpassant state
    m_position.editCastling().write(undoState.castlingState.read());    // restore castling state

    m_hash = undoState.hash;  // this should be calculated and not just overwritten?
    m_moveCount -= (short)m_isWhiteTurn;
    m_isWhiteTurn = !m_isWhiteTurn;

    return true;
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

std::tuple<Square, ChessPiece>
Chessboard::InternalHandlePawnMove(const PackedMove move, Set set, MutableMaterialProxy& materialEditor, MoveUndoUnit& undoState)
{
    Square pieceTarget = move.targetSqr();
    const ChessPiece src(set, PieceType::PAWN);

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
        const ChessPiece promote(set, static_cast<PieceType>(move.readPromoteToPieceType()));
        undoState.movedPiece = promote; // store promotion piece in undo state as moved piece

        m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, src, move.sourceSqr());
        m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, promote, move.sourceSqr());

        // updating the piece on the source tile since we're doing this pre-move.
        // internal move will handle the actual move of the piece, but what piece it is doesn't
        // really mater at that point.
        materialEditor[move.sourceSqr()] = false;
        MutableMaterialProxy matPromoteEditor = m_position.materialEditor(set, static_cast<PieceType>(move.readPromoteToPieceType()));
        matPromoteEditor[move.sourceSqr()] = true;
        materialEditor = matPromoteEditor;
        return std::make_tuple(pieceTarget, promote);
    }

    return std::make_tuple(pieceTarget, src);
}

bool
Chessboard::InternalHandleKingMove(const PackedMove move, Set set, Square& targetRook, Square& rookMove, MoveUndoUnit& undoUnit)
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
            targetRook = Notation(0, targetRank).toSquare();
            rookMove = Notation(3, targetRank).toSquare();
            castling = true;
        }
        else if (targetSquare.file == 6)  // we are in g file.
        {
            targetRook = Notation(7, targetRank).toSquare();
            rookMove = Notation(5, targetRank).toSquare();
            castling = true;
        }
    }

    // clear castling state from hash
    m_hash = ZorbistHash::Instance().HashCastling(m_hash, castlingState);

    // update castling state
    undoUnit.castlingState.write(castlingState);
    casltingMask &= castlingState;
    castlingState ^= casltingMask;
    m_position.editCastling().write(castlingState);

    // apply new castling state to hash
    m_hash = ZorbistHash::Instance().HashCastling(m_hash, castlingState);
    return castling;
}

void Chessboard::InternalHandleRookMove(const ChessPiece piece, const PackedMove move, Square targetRook, Square rookMove, MoveUndoUnit& undoState) {
    if (piece.getType() == PieceType::KING && targetRook != Square::NullSQ) {
        ChessPiece rook(piece.getSet(), PieceType::ROOK);
        auto editor = m_position.materialEditor(piece.getSet(), PieceType::ROOK);
        InternalMakeMove(rook, targetRook, rookMove, editor);
    }
    else {
        InternalHandleRookMovedOrCaptured(move.sourceSqr(), undoState);
    }
}

void Chessboard::InternalUpdateCastlingState(byte mask, MoveUndoUnit& undoState) {
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
Chessboard::InternalHandleRookMovedOrCaptured(Notation rookSquare, MoveUndoUnit& undoState)
{
    byte mask = 0;
    // 0x01 == K, 0x02 == Q, 0x04 == k, 0x08 == q
    switch (rookSquare.index()) {
    case 63:  // H8 Black King Side Rook
        mask |= 0x04;
        InternalUpdateCastlingState(mask, undoState);
        break;
    case 56:  // A8 Black Queen Side Rook
        mask |= 0x08;
        InternalUpdateCastlingState(mask, undoState);
        break;
    case 7:  // H1 White King Side Rook
        mask |= 0x01;
        InternalUpdateCastlingState(mask, undoState);
        break;
    case 0:  // A1 White Queen Side Rook
        mask |= 0x02;
        InternalUpdateCastlingState(mask, undoState);
        break;
    }
}

void
Chessboard::InternalHandleKingRookMove(const ChessPiece piece, const PackedMove move, MoveUndoUnit& undoState)
{
    Square targetRook = Square::NullSQ;
    Square rookMove = Square::NullSQ;

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
Chessboard::InternalMakeMove(ChessPiece piece, Square source, Square target, MutableMaterialProxy materialEditor)
{
    materialEditor[source] = false;
    materialEditor[target] = true;

    // update hash
    m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, piece, target);
    m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, piece, source);
}

MoveUndoUnit
Chessboard::InternalMakeMove(const std::string& moveString)
{
    Move parsedMove = Move::fromPGN(moveString, m_isWhiteTurn);
    Set toMove = m_isWhiteTurn ? Set::WHITE : Set::BLACK;

    if (parsedMove.isAmbiguous() == true) {
        Bitboard pieceBB = m_position.readMaterial().read(toMove, parsedMove.Piece.index());

        if (pieceBB.count() == 1) {
            parsedMove.SourceSquare = Notation(pieceBB.lsbIndex());
        }
        else  // need to figure out which piece is being moved
        {
            MoveGenerator moveGen(m_position, toMove, parsedMove.Piece.getType());
            moveGen.generate();
            moveGen.forEachMove([&](const PrioratizedMove& move) {
                // this might be good enough for now, but if we have multiple pieces that can move to the
                // same square, there is unambiguious information in the parsed move which we need to use.
                if (move.move.target() == parsedMove.TargetSquare.index()) {
                    parsedMove.SourceSquare = Notation(move.move.source());
                }
                });
        }
    }

    PackedMove move = parsedMove.readPackedMove();
    auto undo = MakeMove<false>(move);

    return undo;
}

void
Chessboard::InternalHandleCapture(const PackedMove move, const Square pieceTarget, MoveUndoUnit& undoState)
{
    // handle capture
    auto capturedPiece = m_position.readPieceAt(pieceTarget);

    if (capturedPiece != ChessPiece::None()) {
        m_plyCount = 0;

        // store captured piece in undo state
        undoState.capturedPiece = capturedPiece;

        // handle castling rights in case piece is a rook.
        if (capturedPiece.getType() == PieceType::ROOK)
            InternalHandleRookMovedOrCaptured(move.targetSqr(), undoState);

        // remove captured piece from board.
        m_position.ClearPiece(capturedPiece, pieceTarget);

        // remove piece from hash
        m_hash = ZorbistHash::Instance().HashPiecePlacement(m_hash, capturedPiece, pieceTarget);
        return;
    }

    FATAL_ASSERT(move.isCapture()); // move claims it was a capture but there was no piece at target?
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

SlidingMaterialMasks
Chessboard::readSlidingMaterialMask(Set set) const
{
    if (set == Set::BLACK)
        return m_position.calcMaterialSlidingMasksBulk<Set::BLACK>();

    return m_position.calcMaterialSlidingMasksBulk<Set::WHITE>();
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

void Chessboard::setToPlay(Set set)
{
    m_isWhiteTurn = set == Set::WHITE;
    if (set == Set::BLACK)
        m_hash = ZorbistHash::Instance().HashBlackToMove(m_hash);
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
    // endgame already should just return 1.f

    i32 boardMaterialCombinedValue = 0;
    for (u8 index = 0; index < 5; ++index) {
        boardMaterialCombinedValue += ChessPieceDef::Value(index) * m_position.readMaterial().read<Set::WHITE>(index).count();
        boardMaterialCombinedValue += ChessPieceDef::Value(index) * m_position.readMaterial().read<Set::BLACK>(index).count();
    }

    // reflecting on the move counter, this can at most add .5 to the coeficient since I want piece value to be
    // the dominating factor in the endgame coeficient.
    const short maxCount = 100;
    const float maxCountCoeficient = 0.5f;
    // 100 moves into the game should give us a coeficient of 0.5
    // 50 moves into the game should give us a coeficient of 0.25
    float countCoeficient = ((float)m_moveCount / (float)maxCount) / 2.f;
    countCoeficient = std::min(countCoeficient, maxCountCoeficient);

    float coeficient = 1.f - ((float)boardMaterialCombinedValue / (float)defaultPosValueOfMaterial);
    coeficient += countCoeficient;
    return std::min(coeficient, 1.f);
}