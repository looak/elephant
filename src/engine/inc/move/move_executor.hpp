#pragma once

#include <position/position_accessors.hpp>
#include <move/move.hpp>

struct GameHistory;
struct GameState;

class MoveExecutor {
public:
    MoveExecutor(PositionProxy<PositionEditPolicy> position);

    template<bool validation = false>
    void makeMove(const PackedMove move, GameState& gameState, GameHistory& history);

private:
    MoveUndoUnit internalMakeMove(const std::string& moveString);

    std::tuple<Square, ChessPiece> internalHandlePawnMove(const PackedMove move, Set set, MutableMaterialProxy& materialEditor, MoveUndoUnit& undoState);
    void internalHandleRookMove(const ChessPiece piece, const PackedMove move, Square targetRook, Square rookMove, MoveUndoUnit& undoState);
    void internalHandleRookMovedOrCaptured(Notation rookSquare, MoveUndoUnit& undoState);
    void internalUpdateCastlingState(byte mask, MoveUndoUnit& undoState);

    /**
     * internal helper function for handling the movement of a king chess piece.
     *
     * @param move The move being made.
     * @param set The set of the king being moved.
     * @param targetRook The position of the rook that will be involved in the castle move (if any).
     * @param rookMove The position that the rook will move to during the castle move (if any).
     * @return True if the move is a castle move, false otherwise. */
    bool internalHandleKingMove(const PackedMove move, Set set, Square& targetRook, Square& rookMove, MoveUndoUnit& undoState);
    void internalHandleKingRookMove(const ChessPiece piece, const PackedMove move, MoveUndoUnit& undoState);
    void internalHandleCapture(const PackedMove move, const Square pieceTarget, MoveUndoUnit& undoState);

    void internalUpdateEnPassant(Notation source, Notation target);
    void internalMakeMove(ChessPiece piece, Square source, Square target, MutableMaterialProxy materialEditor);

    PositionProxy<PositionEditPolicy> m_position;
};