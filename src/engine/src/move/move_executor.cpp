#include <move/move_executor.hpp>
#include <position/hash_zorbist.hpp>

#include <game_context.h>
#include <chessboard.h>

MoveExecutor::MoveExecutor(PositionProxy<PositionEditPolicy> position, GameState& gameState, GameHistory& gameHistory) :
    m_position(position),
    m_gameStateRef(gameState),
    m_gameHistoryRef(gameHistory)
{}

template<bool validation>
void MoveExecutor::makeMove(const PackedMove move)
{
    MoveUndoUnit& undoUnit = m_gameHistoryRef.moveUndoUnits.emplace_back();
    undoUnit.move = move;
    undoUnit.hash = m_position.hash();
    undoUnit.plyCount = m_gameStateRef.plyCount;

    ChessPiece movingPiece = m_position.pieceAt(move.sourceSqr());
    undoUnit.movedPiece = movingPiece;
    
    if constexpr (validation) {
        if (!movingPiece.isValid()) {
            LOG_ERROR() << "Trying to make a move with an invalid piece at square " << Notation(move.sourceSqr()).toString();            
            return;  // early exit if the piece is invalid
        }
    }

    MutableMaterialProxy materialEditor = m_position.materialEditor(movingPiece.getSet(), movingPiece.getType());
    const Square targetSqr = move.targetSqr();

    // cache captureTarget in seperate variable since we might be capturing enpassant
    Square captureTarget = targetSqr;

    // store enpassant target square and castling state in undo unit
    undoUnit.enPassantState.write(m_position.enPassant().read());
    undoUnit.castlingState.write(m_position.castling().read());

    switch (movingPiece.getType()) {
    case PieceType::PAWN:
        // updating pieceTarget since if we're capturing enpassant the target will be on a
        // different square.
        std::tie(captureTarget, movingPiece) = internalHandlePawnMove(move, movingPiece.getSet(), materialEditor, undoUnit);
        m_gameStateRef.plyCount = 0;  // reset ply count on pawn move
        break;

    case PieceType::KING:
        [[fallthrough]];
    case PieceType::ROOK:
        internalHandleKingRookMove(movingPiece, move, undoUnit);
        [[fallthrough]];

    default:
        // update hash, start by removing old en passant if there was one.
        if (m_position.enPassant() == true)
            m_position.hash() = zorbist::updateEnPassantHash(m_position.hash(), m_position.enPassant().readSquare());

        // reset enpassant cached values
        m_position.enPassant().clear();
    }

    // since capture will reset this to 0, we need to increment it here.
    m_gameStateRef.plyCount++;
    m_gameHistoryRef.age++;

    if (move.isCapture())
        internalHandleCapture(move, captureTarget, undoUnit);

    // should happen after capture since enpassant logic relies on that order.
    internalMakeMove(movingPiece, move.sourceSqr(), move.targetSqr(), materialEditor);

    // unless something goes wrong, updating the black to move hash should remove it when it's time for white to move,
    // or add it when it's time for black to move.
    m_position.hash() = zorbist::updateBlackToMoveHash(m_position.hash());

    // flip the bool and if we're back at white turn we assume we just made a black turn and hence we increment the move count.
    m_gameStateRef.whiteToMove = !m_gameStateRef.whiteToMove;
    m_gameStateRef.moveCount += (short)m_gameStateRef.whiteToMove;
}

template void MoveExecutor::makeMove<true>(const PackedMove);
template void MoveExecutor::makeMove<false>(const PackedMove);


void MoveExecutor::internalUpdateEnPassant(Notation source, Notation target)
{
    // update hash, start by removing old en passant if there was one.
    if (m_position.enPassant() == true)
        m_position.hash() = zorbist::updateEnPassantHash(m_position.hash(), m_position.enPassant().readSquare());

    // reset enpassant cached values before updating en passant
    m_position.enPassant().clear();

    signed char dif = source.rank - target.rank;
    if (abs(dif) == 2)  // we made a enpassant move
    {
        dif = (signed char)((float)dif * .5f);
        auto sqr = Notation(source.file, source.rank - dif);
        m_position.enPassant().writeSquare(sqr.toSquare());
        m_position.hash() = zorbist::updateEnPassantHash(m_position.hash(), sqr.toSquare());
    }
}

std::tuple<Square, ChessPiece>
MoveExecutor::internalHandlePawnMove(const PackedMove move, Set set, MutableMaterialProxy& materialEditor, MoveUndoUnit& undoState)
{
    Square pieceTarget = move.targetSqr();
    const ChessPiece src(set, PieceType::PAWN);

    // compare target square with en passant, if this is true we're capturing enpassant
    if (pieceTarget == m_position.enPassant().readSquare()) {
        // changing piece target to the position of the pawn being capturing enpassant
        pieceTarget = m_position.enPassant().readTarget();

        // could validate that there is a pawn on the target square.
    }

    internalUpdateEnPassant(move.sourceSqr(), move.targetSqr());

    if (move.isPromotion()) {
        const ChessPiece promote(set, static_cast<PieceType>(move.readPromoteToPieceType()));
        undoState.movedPiece = promote; // store promotion piece in undo state as moved piece

        m_position.hash() = zorbist::updatePieceHash(m_position.hash(), src, move.sourceSqr());
        m_position.hash() = zorbist::updatePieceHash(m_position.hash(), promote, move.sourceSqr());

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

bool MoveExecutor::internalHandleKingMove(const PackedMove move, Set set, Square& targetRook, Square& rookMove, MoveUndoUnit& undoUnit)
{
    const u8 setIndx = (u8)set;
    bool castling = false;
    byte casltingMask = 3 << (2 * setIndx);
    byte castlingState = m_position.castling().read();
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
    m_position.hash() = zorbist::updateCastlingHash(m_position.hash(), castlingState);

    // update castling state
    undoUnit.castlingState.write(castlingState);
    casltingMask &= castlingState;
    castlingState ^= casltingMask;
    m_position.castling().write(castlingState);

    // apply new castling state to hash
    m_position.hash() = zorbist::updateCastlingHash(m_position.hash(), castlingState);
    return castling;
}

void MoveExecutor::internalHandleRookMove(const ChessPiece piece, const PackedMove move, Square targetRook, Square rookMove, MoveUndoUnit& undoState) {
    if (piece.getType() == PieceType::KING && targetRook != Square::NullSQ) {
        ChessPiece rook(piece.getSet(), PieceType::ROOK);
        auto editor = m_position.materialEditor(piece.getSet(), PieceType::ROOK);
        internalMakeMove(rook, targetRook, rookMove, editor);
    }
    else {
        internalHandleRookMovedOrCaptured(move.sourceSqr(), undoState);
    }
}

void MoveExecutor::internalUpdateCastlingState(byte mask, MoveUndoUnit& undoState) {
    byte castlingState = m_position.castling().read();
    m_position.hash() = zorbist::updateCastlingHash(m_position.hash(), castlingState);
    // in a situation where rook captures rook from original positions we don't need to store
    // we don't want to overwrite the original prev written while doing our move castling state.
    // this code has changed slightly since the comment above was written in case we run into a bug.
    if (undoState.castlingState.hasNone())
        undoState.castlingState.write(castlingState);
    mask &= castlingState;
    castlingState ^= mask;
    m_position.hash() = zorbist::updateCastlingHash(m_position.hash(), castlingState);
    m_position.castling().write(castlingState);
}

void MoveExecutor::internalHandleRookMovedOrCaptured(Notation rookSquare, MoveUndoUnit& undoState)
{
    byte mask = 0;
    // 0x01 == K, 0x02 == Q, 0x04 == k, 0x08 == q
    switch (rookSquare.index()) {
    case 63:  // H8 Black King Side Rook
        mask |= 0x04;
        internalUpdateCastlingState(mask, undoState);
        break;
    case 56:  // A8 Black Queen Side Rook
        mask |= 0x08;
        internalUpdateCastlingState(mask, undoState);
        break;
    case 7:  // H1 White King Side Rook
        mask |= 0x01;
        internalUpdateCastlingState(mask, undoState);
        break;
    case 0:  // A1 White Queen Side Rook
        mask |= 0x02;
        internalUpdateCastlingState(mask, undoState);
        break;
    }
}

void MoveExecutor::internalHandleKingRookMove(const ChessPiece piece, const PackedMove move, MoveUndoUnit& undoState)
{
    Square targetRook = Square::NullSQ;
    Square rookMove = Square::NullSQ;

    switch (piece.getType()) {
    case PieceType::KING:
        if (internalHandleKingMove(move, piece.getSet(), targetRook, rookMove, undoState) == false)
            break;

        [[fallthrough]];

    case PieceType::ROOK:
        internalHandleRookMove(piece, move, targetRook, rookMove, undoState);
        [[fallthrough]];

    default:
        return;
    }
}

void MoveExecutor::internalMakeMove(ChessPiece piece, Square source, Square target, MutableMaterialProxy materialEditor)
{
    materialEditor[source] = false;
    materialEditor[target] = true;

    // update hash
    m_position.hash() = zorbist::updatePieceHash(m_position.hash(), piece, target);
    m_position.hash() = zorbist::updatePieceHash(m_position.hash(), piece, source);
}

void MoveExecutor::internalHandleCapture(const PackedMove move, const Square pieceTarget, MoveUndoUnit& undoState)
{
    // handle capture
    auto capturedPiece = m_position.pieceAt(pieceTarget);

    if (capturedPiece != ChessPiece::None()) {
        //m_plyCount = 0;

        // store captured piece in undo state
        undoState.capturedPiece = capturedPiece;

        // handle castling rights in case piece is a rook.
        if (capturedPiece.getType() == PieceType::ROOK)
            internalHandleRookMovedOrCaptured(move.targetSqr(), undoState);

        // remove captured piece from board.
        m_position.clearPiece(pieceTarget);

        // remove piece from hash
        m_position.hash() = zorbist::updatePieceHash(m_position.hash(), capturedPiece, pieceTarget);
        return;
    }

    FATAL_ASSERT(move.isCapture()); // move claims it was a capture but there was no piece at target?
}
