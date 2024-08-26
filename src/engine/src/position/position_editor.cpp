#include <position/position_editor.hpp>

void PositionEditor::clear() {
    m_position.m_materialMask.clear();
    m_position.m_enpassantState = {};
    m_position.m_castlingState = {};
}

bool PositionEditor::empty() {
    return m_position.m_materialMask.empty();
}

bool PositionEditor::placePiece(Piece piece, Square square) {

}

bool PositionEditor::removePiece(Square square) {

}

MoveUndoUnit PositionEditor::makeMove(PackedMove move) {

}

void PositionEditor::unmakeMove(const MoveUndoUnit& undoUnit) {

}


EnPassantStateInfo& PositionEditor::editEnPassantState() {
    return m_position.m_enpassantState;
}

void PositionEditor::updateEnPassantState(EnPassantStateInfo& info) {
    m_position.m_enpassantState = info;
}

CastlingStateInfo& PositionEditor::editCastlingState() {
    return m_position.m_castlingState;
}

void PositionEditor::updateCastlingState(CastlingStateInfo& info) {
    m_position.m_castlingState = info;
}
