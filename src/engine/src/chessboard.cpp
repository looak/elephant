#include "chessboard.h"
#include "bitboard_constants.hpp"
#include "defines.hpp"
#include <position/hash_zorbist.hpp>
#include "intrinsics.hpp"
#include "log.h"
#include <move/move.hpp>
#include <move_generation/move_generator.hpp>

#include <future>
#include <thread>
#include <vector>

Chessboard::Chessboard()
{}

Chessboard::Chessboard(const Chessboard& other) :
    m_gameState(other.m_gameState),
    m_position(other.m_position)
{
}

std::string
Chessboard::toString() const
{
    auto positionReader = m_position.read();   


    auto boardItr = positionReader.begin();
    auto endItr = positionReader.end();
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

    } while (boardItr != endItr);

    std::stringstream boardstream;
    auto rankItr = ranks.rbegin();
    while (rankItr != ranks.rend())  // rebuild the board
    {
        boardstream << (*rankItr).str();
        rankItr++;
    }

    boardstream << "\n    A  B  C  D  E  F  G  H\n";

    
    boardstream << "castling state: " << positionReader.castling().toString();
    boardstream << "\nen passant: " << positionReader.enPassant().toString();
    boardstream << "\nhash: 0x" << positionReader.hash() << "\n";

    return boardstream.str();
}

// bool
// Chessboard::UnmakeMove(const MoveUndoUnit& undoState)
// {
//     const Square srcSqr = undoState.move.sourceSqr();
//     const Square trgSqr = undoState.move.targetSqr();
//     //const ChessPiece movedPiece = m_position.readPieceAt((Square)trgSqr);
//     const ChessPiece movedPiece = undoState.movedPiece;

//     // idea here is to store the piece to place back on the board in this variable,
//     // if we're dealing with a promotion this will be a pawn of the correct set.
//     // otherwise it will be the same piece as movedPiece.
//     const ChessPiece promotedPiece =
//         undoState.move.isPromotion() ? ChessPiece(movedPiece.getSet(), PieceType::PAWN) : movedPiece;

//     // unmake move, currently we are tracking the piece and board state in two
//     // different places, here in our tiles and in the position.
//     m_position.PlacePiece(promotedPiece, srcSqr);
//     m_position.ClearPiece(movedPiece, trgSqr);

//     if (undoState.move.isCapture()) {
//         if (undoState.move.isEnPassant()) {
//             m_position.PlacePiece(undoState.capturedPiece, undoState.enPassantState.readTarget());
//         }
//         else {
//             m_position.PlacePiece(undoState.capturedPiece, trgSqr);
//         }
//     }
//     else if (undoState.move.isCastling()) {
//         // we're unmaking a castling move, we need to move the rook back to it's original position.
//         // simply done by moving the rook with internal move from notations generated with the king move.
//         // king would have been moved back by regular undo move code.
//         Notation rookSource;
//         Notation rookTarget;
//         Notation target(trgSqr);
//         if (target.file == file_c)  // queen side
//         {
//             rookSource = Notation(file_a, target.rank);
//             rookTarget = Notation(file_d, target.rank);
//         }
//         else  // king side
//         {
//             rookSource = Notation(file_h, target.rank);
//             rookTarget = Notation(file_f, target.rank);
//         }
//         ChessPiece rook(movedPiece.getSet(), PieceType::ROOK);
//         auto editor = m_position.materialEditor(movedPiece.getSet(), PieceType::ROOK);
//         InternalMakeMove(rook, rookTarget.toSquare(), rookSource.toSquare(), editor);
//     }

//     m_position.editEnPassant().write(undoState.enPassantState.read());  // restore enpassant state
//     m_position.editCastling().write(undoState.castlingState.read());    // restore castling state

//     m_hash = undoState.hash;  // this should be calculated and not just overwritten?
//     m_moveCount -= (short)m_isWhiteTurn;
//     m_isWhiteTurn = !m_isWhiteTurn;
//     m_plyCount = undoState.plyCount;
//     m_age--;

//     return true;
// }

// MoveUndoUnit
// Chessboard::InternalMakeMove(const std::string& moveString)
// {
//     Move parsedMove = Move::fromPGN(moveString, m_isWhiteTurn);
//     Set toMove = m_isWhiteTurn ? Set::WHITE : Set::BLACK;

//     if (parsedMove.isAmbiguous() == true) {
//         Bitboard pieceBB = m_position.readMaterial().read(toMove, parsedMove.Piece.index());

//         if (pieceBB.count() == 1) {
//             parsedMove.SourceSquare = Notation(pieceBB.lsbIndex());
//         }
//         else  // need to figure out which piece is being moved
//         {
//             MoveGenerator moveGen(m_position, toMove, parsedMove.Piece.getType());
//             moveGen.generate();
//             moveGen.forEachMove([&](const PrioratizedMove& move) {
//                 // this might be good enough for now, but if we have multiple pieces that can move to the
//                 // same square, there is unambiguious information in the parsed move which we need to use.
//                 if (move.move.target() == parsedMove.TargetSquare.index()) {
//                     parsedMove.SourceSquare = Notation(move.move.source());
//                 }
//                 });
//         }
//     }

//     PackedMove move = parsedMove.readPackedMove();
//     auto undo = MakeMove<false>(move);

//     return undo;
// }