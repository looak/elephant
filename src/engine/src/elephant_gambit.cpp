#include <elephant_gambit.hpp>
#include <core/chessboard.hpp>
#include <io/fen_parser.hpp>

namespace chess {
    void ClearBoard(Chessboard& board) {
        board.editPosition().clear();
        board.editState().plyCount = 0;
        board.editState().moveCount = 1;
        board.editState().whiteToMove = true;
    }

    void NewGame(Chessboard& board) {
        ClearBoard(board);
        io::fen_parser::deserialize(c_startPositionFen.c_str(), board);
    }
} // namespace chess

