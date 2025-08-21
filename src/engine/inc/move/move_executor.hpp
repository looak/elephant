#pragma once

#include <position/position_accessors.hpp>
#include <move/move.hpp>

struct GameHistory;
struct GameState;

class MoveExecutor {
public:
    MoveExecutor(PositionProxy<PositionEditPolicy> position, GameState& gameState, GameHistory& gameHistory);

    template<bool validation = false>
    void makeMove(const PackedMove move);

private:
    MoveUndoUnit internalMakeMove(const std::string& moveString);
    void internalMakeMove(ChessPiece piece, Square source, Square target, MutableMaterialProxy materialEditor);

    std::tuple<Square, ChessPiece> internalHandlePawnMove(const PackedMove move, Set set, MutableMaterialProxy& materialEditor, MoveUndoUnit& undoState);
    void internalHandleRookMove(const ChessPiece piece, const PackedMove move, Square targetRook, Square rookMove, MoveUndoUnit& undoState);
    void internalHandleRookMovedOrCaptured(Square rookSquare, MoveUndoUnit& undoState);
    void internalUpdateCastlingState(byte mask, MoveUndoUnit& undoState);    
    bool internalHandleKingMove(const PackedMove move, Set set, Square& targetRook, Square& rookMove, MoveUndoUnit& undoState);
    void internalHandleKingRookMove(const ChessPiece piece, const PackedMove move, MoveUndoUnit& undoState);
    void internalHandleCapture(const PackedMove move, const Square pieceTarget, MoveUndoUnit& undoState);
    void internalUpdateEnPassant(Square source, Square target);

    PositionProxy<PositionEditPolicy> m_position;

    GameState& m_gameStateRef;
    GameHistory& m_gameHistoryRef;
};