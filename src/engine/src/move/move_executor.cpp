#include <move/move_executor.hpp>

#include <core/chessboard.hpp>
#include <core/game_context.hpp>
#include <core/square_notation.hpp>
#include <math/cast.hpp>
#include <position/hash_zobrist.hpp>


template<bool validation>
void MoveExecutor::makeMove(const PackedMove move, MoveUndoUnit& undoUnit, u16& plyCount)
{
    if constexpr (validation) {
        if (move.isNull()) {
            LOG_ERROR("Trying to make a null move.");
            return;  // early exit if the move is null
        }
    }
    
    undoUnit.move = move;
    undoUnit.hash = m_position.hash();
    undoUnit.plyCount = plyCount;

    ChessPiece movingPiece = m_position.pieceAt(move.sourceSqr());
    undoUnit.movedPiece = movingPiece;
    
    if constexpr (validation) {
        if (!movingPiece.isValid()) {
            LOG_ERROR("Trying to make a move with an invalid piece at square {}", SquareNotation(move.sourceSqr()).toString());            
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
        plyCount = 0;  // reset ply count on pawn move
        break;

    case PieceType::KING:
        [[fallthrough]];
    case PieceType::ROOK:
        internalHandleKingRookMove(movingPiece, move, undoUnit);
        [[fallthrough]];

    default:
        // reset enpassant cached values
        m_position.enPassant().clear();
    }

    plyCount++;  // increment ply count on non-capture move
    
    if (move.isCapture())
    {
        internalHandleCapture(move, captureTarget, undoUnit);
        plyCount = 0;  // reset ply count on capture move
    }   

    // should happen after capture since enpassant logic relies on that order.
    internalMakeMove(movingPiece, move.sourceSqr(), move.targetSqr(), materialEditor);

    // unless something goes wrong, updating the black to move hash should remove it when it's time for white to move,
    // or add it when it's time for black to move.
    m_position.hash() = zobrist::updateBlackToMoveHash(m_position.hash());
}

template void MoveExecutor::makeMove<true>(const PackedMove, MoveUndoUnit&, u16&);
template void MoveExecutor::makeMove<false>(const PackedMove, MoveUndoUnit&, u16&);

void MoveExecutor::internalUpdateEnPassant(Square source, Square target)
{
    // reset enpassant cached values before updating en passant
    m_position.enPassant().clear();

    i16 dif = toRank(source) - toRank(target);
    if (abs(dif) == 2)  { // we made a enpassant move, calculate the enpassant square made available for pawn capture.
        dif = dif / 2;

        Square epSqr = Square::NullSQ;
        if (dif < 0)
            epSqr = shiftNorth(source);
        else
            epSqr = shiftSouth(source);
        // Square sqr = SquareNotation(toFile(source), toRank(source) - checked_cast<u8>(dif)).toSquare();
        m_position.enPassant().writeSquare(epSqr);
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

        m_position.hash() = zobrist::updatePieceHash(m_position.hash(), src, move.sourceSqr());
        m_position.hash() = zobrist::updatePieceHash(m_position.hash(), promote, move.sourceSqr());

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
    byte casltingMask = checked_cast<byte>(3 << (2 * setIndx));
    byte castlingState = m_position.castling().read();
    SquareNotation targetSquare(move.targetSqr());
    if (castlingState & casltingMask) {
        byte targetRank = 7 * setIndx;
        if (targetSquare.file() == 2)  // we are in c file.
        {
            targetRook = SquareNotation(0, targetRank).toSquare();
            rookMove = SquareNotation(3, targetRank).toSquare();
            castling = true;
        }
        else if (targetSquare.file() == 6)  // we are in g file.
        {
            targetRook = SquareNotation(7, targetRank).toSquare();
            rookMove = SquareNotation(5, targetRank).toSquare();
            castling = true;
        }
    }

    // update castling state
    undoUnit.castlingState.write(castlingState);
    casltingMask &= castlingState;
    castlingState ^= casltingMask;
    m_position.castling().write(castlingState);

    return castling;
}

void MoveExecutor::internalHandleRookMove(const ChessPiece piece, const PackedMove move, Square targetRook, Square rookMove, MoveUndoUnit& undoState) {
    // rook is being moved by king castling, the target square for the rook is figured out previously.
    if (piece.getType() == PieceType::KING && targetRook != Square::NullSQ) {
        ChessPiece rook(piece.getSet(), PieceType::ROOK); // construct rook piece for hashing purposes inside internalMakeMove
        auto editor = m_position.materialEditor(piece.getSet(), PieceType::ROOK);
        internalMakeMove(rook, targetRook, rookMove, editor);
    }
    else {
        internalHandleRookMovedOrCaptured(move.sourceSqr(), undoState);
    }
}

void MoveExecutor::internalUpdateCastlingState(byte mask, MoveUndoUnit& undoState) {
    byte castlingState = m_position.castling().read();
    // in a situation where rook captures rook from original positions we don't need to store
    // we don't want to overwrite the original prev written while doing our move castling state.
    // this code has changed slightly since the comment above was written in case we run into a bug.
    if (undoState.castlingState.hasNone())
        undoState.castlingState.write(castlingState);
    mask &= castlingState;
    castlingState ^= mask;    
    m_position.castling().write(castlingState);
}

void MoveExecutor::internalHandleRookMovedOrCaptured(Square rookSquare, MoveUndoUnit& undoState)
{
    byte mask = 0;
    // 0x01 == K, 0x02 == Q, 0x04 == k, 0x08 == q
    switch (rookSquare) {
    case Square::H8:  // H8 Black King Side Rook
        mask |= 0x04;
        internalUpdateCastlingState(mask, undoState);
        break;
    case Square::A8:  // A8 Black Queen Side Rook
        mask |= 0x08;
        internalUpdateCastlingState(mask, undoState);
        break;
    case Square::H1:  // H1 White King Side Rook
        mask |= 0x01;
        internalUpdateCastlingState(mask, undoState);
        break;
    case Square::A1:  // A1 White Queen Side Rook
        mask |= 0x02;
        internalUpdateCastlingState(mask, undoState);
        break;
    default:
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
    m_position.hash() = zobrist::updatePieceHash(m_position.hash(), piece, target);
    m_position.hash() = zobrist::updatePieceHash(m_position.hash(), piece, source);
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
        m_position.hash() = zobrist::updatePieceHash(m_position.hash(), capturedPiece, pieceTarget);
        return;
    }

    ASSERT(move.isCapture()); // move claims it was a capture but there was no piece at target?
}

bool MoveExecutor::unmakeMove(const MoveUndoUnit& undoState)
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

    // unmake move
    m_position.placePiece(promotedPiece, srcSqr);
    m_position.clearPiece(trgSqr);

    if (undoState.move.isCapture()) {
        if (undoState.move.isEnPassant()) {
            m_position.placePiece(undoState.capturedPiece, undoState.enPassantState.readTarget());
        }
        else {
            m_position.placePiece(undoState.capturedPiece, trgSqr);
        }
    }
    else if (undoState.move.isCastling()) {
        // we're unmaking a castling move, we need to move the rook back to it's original position.
        // simply done by moving the rook with internal move from notations generated with the king move.
        // king would have been moved back by regular undo move code.
        SquareNotation rookSource;
        SquareNotation rookTarget;
        SquareNotation target(trgSqr);
        if (target.file() == coordinates::file_c)  // queen side
        {
            rookSource = SquareNotation(coordinates::file_a, target.rank());
            rookTarget = SquareNotation(coordinates::file_d, target.rank());
        }
        else  // king side
        {
            rookSource = SquareNotation(coordinates::file_h, target.rank());
            rookTarget = SquareNotation(coordinates::file_f, target.rank());
        }
        ChessPiece rook(movedPiece.getSet(), PieceType::ROOK);
        auto editor = m_position.materialEditor(movedPiece.getSet(), PieceType::ROOK);
        internalMakeMove(rook, rookTarget.toSquare(), rookSource.toSquare(), editor);
    }

    m_position.enPassant().write(undoState.enPassantState.read());  // restore enpassant state
    m_position.castling().write(undoState.castlingState.read());    // restore castling state

    m_position.hash() = undoState.hash; // overwrite hash to previous state.

    return true;
}
