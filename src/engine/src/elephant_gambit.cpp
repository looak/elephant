#include "elephant_gambit.h"
#include "chessboard.h"
#include <serializing/fen_parser.hpp>

namespace chess {
    void ClearBoard(Chessboard& board) {
        board.editPosition().clear();
        board.editState().plyCount = 0;
        board.editState().moveCount = 1;
        board.editState().whiteToMove = true;
    }

    void NewGame(Chessboard& board) {
        ClearBoard(board);
        FENParser::deserialize(c_startPositionFen.c_str(), board);
    }
} // namespace chess

